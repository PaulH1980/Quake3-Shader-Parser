#include <sstream>
#include <cmath>
#include <string>

#include <QtCore/QProcess>
#include <QtCore/QDebug>

#include <Math/GenMath.h>
#include <Math/CameraRay.h>

#include <Common/Image.h>
#include <Common/ContainerTools.h>

#include <Render/OpenGLIncludes.h>

#include <Resource/ConcreteResources.hpp>
#include <Engine/RootObject.hpp>
#include <IO/FileSystem.hpp>
#include <Graphics/DebugRender.hpp>
#include <Engine/EngineContext.hpp>
#include <Command/CommandList.hpp>
#include <IO/EngineFileStream.hpp>
#include <Resource/ResourceManager.hpp>
#include <Command/CommandStack.hpp>
#include <Component/Camera.hpp>
#include <Graphics/View.hpp>
#include <Scene/Scene.hpp>
#include <Misc/Q3BuildGLSL.h>
#include <Misc/Q3BspFile.h>


namespace
{
    using namespace App;
    using namespace Misc;	
    using namespace Math;

    const Q3Face& GetMapFace(const Q3BspFile* q3bsp, int faceId)
    {
        return q3bsp->m_faceList[faceId];
    }

    VertexVector		GetFaceVertices(const Q3BspFile* q3bsp, int faceId)
    {
        const auto& face = GetMapFace(q3bsp, faceId);
        VertexVector result;
        for (int i = 0; i < face.m_numVerts; ++i)
        {
            //auto offset = q3bsp->m_meshVertexList[face.m_meshVert + i].m_vertexOffset;
            const auto& vertex = q3bsp->m_vertexList[face.m_vertex + i];
            result.push_back(vertex); 
        }
        return result;
    }
    
    VertexVector		GetFaceTriangleVertices(const Q3BspFile* q3bsp, int faceId )
    {
        const auto& face = GetMapFace(q3bsp, faceId);
        VertexVector result;
        for (int i = 0; i < face.m_numMeshVerts; ++i )
        {
            auto offset = q3bsp->m_meshVertexList[face.m_meshVert + i].m_vertexOffset;
            const auto& vertex = q3bsp->m_vertexList[face.m_vertex + offset];
            result.push_back(vertex);
        }
        return result;
    }

    Planef		GetFacePlane(const Q3BspFile* q3bsp, int faceId)
    {
        const auto& face = GetMapFace(q3bsp, faceId);
        auto offset1 = q3bsp->m_meshVertexList[face.m_meshVert + 0].m_vertexOffset;
        auto offset2 = q3bsp->m_meshVertexList[face.m_meshVert + 1].m_vertexOffset;
        auto offset3 = q3bsp->m_meshVertexList[face.m_meshVert + 2].m_vertexOffset;
        const auto& vertex1 = q3bsp->m_vertexList[face.m_vertex + offset1].m_worldCoord;
        const auto& vertex2 = q3bsp->m_vertexList[face.m_vertex + offset2].m_worldCoord;
        const auto& vertex3 = q3bsp->m_vertexList[face.m_vertex + offset3].m_worldCoord;

        return Planef(vertex1, vertex2, vertex3);
    }

    BBox3f		GetFaceBounds(const Q3BspFile* q3bsp, int faceId)
    {
        Math::BBox3f result;
        auto vertices = GetFaceVertices(q3bsp, faceId);
        for (const auto& vert : vertices)
            result.updateBounds(vert.m_worldCoord);
        return result;
    }

    Vector3f	GetFaceCenter(const Q3BspFile* q3bsp, int faceId)
    {
        auto vertices = GetFaceVertices(q3bsp, faceId);
        return GetPolyCenter(&vertices[0].m_worldCoord, static_cast<int>(vertices.size()), sizeof(vertices[0]));
    }

    bool AutoSpriteSort(const Q3BspFile* q3bsp, const Q3DrawInfo& dI, int faceId, VertexVector& buffer  )
    {
        auto &face = q3bsp->m_faceList[faceId];
        auto vl = GetFaceVertices(q3bsp, faceId);	
        
        BBox2f stBounds;
        for (auto & i : vl)
            stBounds.updateBounds(i.m_stCoord);

        if (!stBounds.isValid()) {
            App::AddConsoleMessage(q3bsp->getContext(), "AutoSprite no st coords", App::LOG_LEVEL_WARNING);
            return false;
        }
        //normalize uv coordinates
        Point3dVector stCoords;
        int vertexCount = static_cast<int>(vl.size());		
        for (int i = 0; i < vertexCount; ++i) {
            auto newStCoord = vl[i].m_stCoord - stBounds.getMin();			
            stCoords.push_back( newStCoord.toVector3() );
        }
        //sort uv coordinates & build indices ->starting at the smallest uv coord so it will
        //be rendered correctly in GLSL
        auto indices = SortVerticesCCW(stCoords.data(), vertexCount, Vector3f::UP);
        auto minIdx = 0;
        auto minLngth = Math::FLOAT_MAX;
        for (auto i = 0; i < indices.size(); ++i)
        {
            auto lengthSqr = stCoords[indices[i]].lengthSquared();
            if (lengthSqr < minLngth)
            {
                minIdx = i;
                minLngth = lengthSqr;
            }			
        }
        //update by indices
        VertexVector newVertices;
        for (int i = 0; i < vl.size(); ++i) {
            auto oldIdx = (minIdx + i) % vertexCount;
            auto newIdx = indices[oldIdx];
            newVertices.push_back(vl[newIdx]);
        }
        //build (degenerate) triangles
        const int egdeIndices[6] = { 2, 0, 1, 3, 0, 2 };
        for (int i = 0; i < 6; ++i) {
            buffer[dI.m_vertexStart + i]			  = newVertices[egdeIndices[i]];
            buffer[dI.m_vertexStart + i].m_worldCoord = dI.m_asCenter; //set vertex to autosprite center, 
        }
    

#if DEBUG
        qDebug() << "================\n";
        for (int i = 0; i < newVertices.size(); ++i) {

            qDebug() << newVertices[i].m_stCoord.toString().c_str();
        }
#endif


        return true;
    }

	//Test if this leaf node has any sky shaders visible
    bool	LeafHasSky(const Q3BspFile* q3bsp, int leafId )
    {
        auto& drawLeaf = q3bsp->m_drawLeafs[leafId];
        for (auto& dI : drawLeaf.m_drawInfoList)
        {
            auto shader = q3bsp->m_shaders[dI.m_shaderId];
            if (shader->hasSurfaceFlag(static_cast<int>(eQ3SurfaceParam::SURFACE_SKY)))
                return true;
        }
        return false;
    }


    bool	GetFaceAutoSprite(const Q3BspFile* q3bsp, int faceId, Q3DrawInfo& dI) 
    {
        dI.m_asCenter = GetFaceCenter(q3bsp, faceId);
        dI.m_asWidth  = GetFaceBounds(q3bsp, faceId).getSize().getMaxValue() * 0.5f;
        return true;
    }

    bool	GetFaceAutoSprite2(const Q3BspFile* q3bsp, int faceId, Q3DrawInfo& dI)
    {
        
        auto vertices = GetFaceVertices(q3bsp, faceId);
        
        //edges
        int e1 = 0;		
        int e2, e3, e4;
        float minDist = Math::FLOAT_MAX;	
        for (int i = 0; i < vertices.size(); ++i) 	//find minor axis
        {
            int j = (i + 1) & 0x3;
            auto distSqr = vertices[i].m_worldCoord.distanceSquared(vertices[j].m_worldCoord);
            if (distSqr < minDist) 
            {
                minDist = distSqr;
                e1 = i;
            }
        }
        e2 = (e1 + 1) & 0x3;
        e3 = (e1 + 2) & 0x3;
        e4 = (e3 + 1) & 0x3;

        dI.m_asVertexPos[0] = (vertices[e1].m_worldCoord + vertices[e2].m_worldCoord) * 0.5f;
        dI.m_asVertexPos[1] = (vertices[e3].m_worldCoord + vertices[e4].m_worldCoord) * 0.5f;
        dI.m_asCenter		 = (dI.m_asVertexPos[0] + dI.m_asVertexPos[1]) * 0.5f;
        dI.m_asWidth = sqrt(minDist) * 0.5f;
        dI.m_asHeight = dI.m_asVertexPos[0].distance( dI.m_asCenter );
        
        if ( (dI.m_asWidth < FLOAT_EPSILON) || (dI.m_asHeight < FLOAT_EPSILON ) )
        {
            App::AddConsoleMessage( q3bsp->getContext(), "AutoSprite2 zero area face", App::LOG_LEVEL_WARNING );
            return false;
        }
        dI.m_asMajorDir = (dI.m_asVertexPos[1] - dI.m_asVertexPos[0]).getNormalized();

        return true;
    }


    //bool		RayHitFace( const Math::ray)

    bool		GetRayHitFace(const Math::Vector3f& start, const Math::Vector3f& dir, 
        const Q3BspFile* q3bsp, int faceId, Math::Vector3f& hit)
    {
        auto vertices = GetFaceTriangleVertices(q3bsp, faceId);
        if (vertices.empty())
            return false;
        for (int i = 0; i < vertices.size() ; i += 3) 
        {
            bool hitFound = Math::IntersectsRay(start, dir, &vertices[i].m_worldCoord,
                3, hit, sizeof(Vertex));
            if (hitFound)
                return true;
        }
        return false;
    }

    std::vector<Q3Triangle>	GetTrianglesForLeaf(const Q3BspFile* q3bsp, int leafId, std::uint32_t sortflags )
    {
        TriangleList result;		
        const auto& q3leaf = q3bsp->m_drawLeafs[leafId];
        for (int i = 0; i < q3leaf.m_numFaces; ++i)
        {
            int offset			= q3leaf.m_firstFace + i;
            auto faceId			= q3bsp->m_leafFaceList[offset].m_faceIndex;
            const auto& face	= q3bsp->m_faceList[faceId];
            auto shader			= q3bsp->getShader(face.m_texIndex);
            if ( shader->hasAutoSprite() ) //ignore here gives unwanted results
                continue;
            auto tmpTriangles = q3bsp->triangulateFace(faceId);	
            result.insert(std::end(result), std::begin(tmpTriangles), std::end(tmpTriangles));
        }	
        //slow but will lets us see autosprites
        for (int i = 0; i < q3bsp->m_faceList.size(); ++i)
        {
            const auto& face = q3bsp->m_faceList[i];
            auto shader = q3bsp->getShader(face.m_texIndex);
            if (shader->hasAutoSprite()) 
            {
                auto faceCenter = GetFaceCenter(q3bsp, i);
                auto asId = q3bsp->leafNodeForPosition(faceCenter);
                if (asId == leafId)
                {
                    auto tmpTriangles = q3bsp->triangulateFace(i);
                    result.insert(std::end(result), std::begin(tmpTriangles), std::end(tmpTriangles));
                }
            }
        }

        std::sort(std::begin(result), std::end(result),
            [](const Q3Triangle& a, const Q3Triangle& b)
        {
            return a.m_shaderId < b.m_shaderId;
        });
        return result;		
    }


    VaoBuffer	CreateVAO(App::EngineContext* context, const Vertex* dataPtr, size_t numVerts)
    {
        using namespace Render;
        auto renderer = context->getSystem<OpenGLRenderer>();
        VaoBuffer vaoPtr = VaoBuffer( renderer->createVertexArrayObject() );
        vaoPtr->setInterleaved(true);

        auto _FlOAT = GetConstant("FLOAT");
        auto _UBYTE = GetConstant("UNSIGNED_BYTE");;
        auto _DRAWMODE = GetConstant("STATIC_DRAW");
        auto _VERTEXSIZE = sizeof(Vertex);
        auto _BUFFERSIZE = numVerts * _VERTEXSIZE;
        //create vertex array object
        VertexBuffer vBuffers[] = {
            VertexBuffer(dataPtr, _FlOAT, 3, _VERTEXSIZE, _DRAWMODE, 0,  0, 0, 0,  _BUFFERSIZE),
            VertexBuffer(dataPtr, _UBYTE, 4, _VERTEXSIZE, _DRAWMODE, 1,  1, 0, 12, _BUFFERSIZE),
            VertexBuffer(dataPtr, _FlOAT, 2, _VERTEXSIZE, _DRAWMODE, 2,  0, 0, 16, _BUFFERSIZE),	//tex coord
            VertexBuffer(dataPtr, _FlOAT, 2, _VERTEXSIZE, _DRAWMODE, 3,  0, 0, 24, _BUFFERSIZE),	//lightmap
            VertexBuffer(dataPtr, _UBYTE, 4, _VERTEXSIZE, _DRAWMODE, 4,  1, 0, 32, _BUFFERSIZE)
        };
        for (const auto& vb : vBuffers)
            vaoPtr->addVertexBuffer(vb);
        vaoPtr->initialize();
        return vaoPtr;
    }

    
	ViewPortVector UpdatePortalViews(const Misc::Q3BspFile* q3bsp, const ViewPortPtr& view, int clusterId )
	{
		ViewPortVector result;
		
		auto viewDepth = view->getRecursionDepth();
		if (viewDepth != 0)
			return result;
		
		const auto& commandList = q3bsp->getContext()->getSystem<CommandStack>()->getCommandList();
		auto entityList			= q3bsp->getEntitiesForCluster(clusterId);
		int culledPortals		= 0;
		int numPortals			= 0;
		for (auto& entity : entityList)
		{
			auto portal = std::dynamic_pointer_cast<Q3DrawPortal>(entity);
			//Update portal with this view
			if (portal && portal->updatePortal(view))
			{
				auto drawPortals = commandList.getVariable<int>("dbg_render_from_mirror") != 0;
				if (drawPortals && portal->isMirror())
				{
					auto camera = view->getCamera();
					camera->setRotation(portal->getPortalView()->getCamera()->getRotation());
					camera->setEye(portal->getPortalView()->getCamera()->getEye());
					camera->updateTransforms();
				}
				result.push_back(portal->getPortalView());
			}			
		}
		return result;
	}

      
    void DrawLeafs( const Misc::Q3BspFile* q3bsp, const DrawListPtr& drawList, std::uint32_t sortFlags )
    {
        if (drawList.empty())
            return;
        
        using namespace Misc;
        using namespace Render;
        using namespace App;
        using namespace Math;

        auto context = q3bsp->getContext();

        static auto& commandList	= context->getSystem<CommandStack>()->getCommandList();
        static auto resMan			= context->getSystem<ResourceManager>();			

    /*	auto DrawDebugLines = [context]( const std::vector<Vertex>& vList, const Q3DrawInfo& info, const Vector4ub& rgba)
        {
            static auto debugRender = context->getSystem<DebugRender>();
            auto start = info.m_vertexStart;
            auto end = start + info.m_vertexCount;
            for (; start < end; start += 3)
            {
                Vector3f points[3] = { vList[start + 0].m_worldCoord, vList[start + 1].m_worldCoord, vList[start + 2].m_worldCoord };
                debugRender->addClosedPolyLine(points, 3, rgba, rgba, DEBUG_DRAW_3D_DEPTH, 15 );
            }
        };*/

    /*	auto drawAlpha		= commandList.getVariable<int>( "r_drawAlphaFunc"	) != 0;
        auto drawVertDef	= commandList.getVariable<int>( "r_drawVertDeform"	) != 0;
        auto drawMultiPass	= commandList.getVariable<int>( "r_drawMultiPass"	) != 0;		
        auto drawTriangles  = commandList.getVariable<int>( "r_drawTriangles"   ) != 0;		*/

        for (const auto& drawInfo : drawList)
        {
            const auto& leaf = q3bsp->m_drawLeafs[drawInfo->m_leafId];
            auto shaderId = drawInfo->m_shaderId;
            auto& q3Shader = q3bsp->m_shaders[shaderId];
            if (!q3Shader->bind())
                continue;

            q3Shader->setDrawInfo( *drawInfo );

            leaf.m_vaoBuffer->bind();
            leaf.m_vaoBuffer->setDrawInformation( GL_TRIANGLES, drawInfo->m_vertexStart, drawInfo->m_vertexCount, false );
            leaf.m_vaoBuffer->draw();

            q3Shader->unBind();


            ////outline alpha tested triangles
            //if (drawAlpha && q3Shader->hasAlphaTest())
            //{
            //	Math::Vector4ub rgba(255, 0, 0, 255);
            //	DrawDebugLines(leaf.m_vertexList, *drawInfo, rgba);
            //}
            ////outline multipass shader
            //if (drawMultiPass && q3Shader->isMultiPass())
            //{
            //	Math::Vector4ub rgba(0, 255, 0, 255);
            //	DrawDebugLines(leaf.m_vertexList, *drawInfo, rgba);
            //}
            ////outline multipass shader
            //if (drawVertDef && q3Shader->hasVertexDeform())
            //{
            //	Math::Vector4ub rgba(0, 0, 255, 255);
            //	DrawDebugLines(leaf.m_vertexList, *drawInfo, rgba);
            //}
            ////outline all triangles
            //if (drawTriangles)
            //{
            //	Math::Vector4ub rgba(255, 255, 255, 255);
            //	DrawDebugLines(leaf.m_vertexList, *drawInfo, rgba);
            //}
        }
    }
}

namespace Misc
{
    /*
    *
    */
    inline MeshVertex convertToNativeVertex( const Q3Vertex& in )
    {
        Vertex result;
        result.m_worldCoord		= in.m_pos;
        result.m_stCoord		= in.m_texCoord1; //texture map
        result.m_uvCoord		= in.m_texCoord2; //lightmap
        result.m_vertexColor	= in.m_rgba;
        result.setNormal(in.m_normal);
        return result;
    }
    
  


    //////////////////////////////////////////////////////////////////////////
    // \Q3BspFile
    //////////////////////////////////////////////////////////////////////////
    Q3BspFile::Q3BspFile( App::EngineContext* context) 
        : App::RootObject( context )
        , m_loaded			(false)
        , m_numPolyFaces	(0) 
        , m_numPatches		(0)
        , m_numMeshFaces	(0)
        , m_numBillBoards	(0)
    {
    };


    Q3BspFile::~Q3BspFile()
    {
        clear();
    }

	bool Q3BspFile::isLoaded() const
	{
		return m_loaded;
	}

	bool Q3BspFile::loadFile(const String& fileName)
    {
        if ((m_fileName == fileName) && m_loaded)
            return true;
		
		clear();
        //do we have a valid q3 bsp file?
        if (!parseLumps( fileName )) {           
            return false;
        }

        //do a first pass to cache shaders
        parseShaderDirectory();
        
        //parse the entity string
        parseEntityString();
        
        //update bounds
        calculateBounds();
        
        //offset map so min values are at origin & max bounds are a power of two
        offsetMap( m_worldBounds.getMin() * -1.0f );	
               
        //load shaders&textures found in this map
        loadShaders();			

        //link entities together
        buildDrawClusters();

        //link entities together
        linkEntities();
            
        //build vertex buffers for draw leafs/clusters
        buildVAOForLeafs();

		//notify listeners
		m_context->getSystem<App::EventSystem>()->postEvent(EVENT_TYPE(MAP_INITIALIZED));

        m_loaded = true;
        return m_loaded;
    }


	bool Q3BspFile::MapExist(ContextPointer context, const String& mapName)
	{
		App::FileInputStream ifs(context);
		if (!ifs.open( (Q3GetMapPath() + mapName).c_str(), App::FILE_TYPE_BINARY)) {
			return false;
		}
		return true;
	}
	

	ClientStartingPos Q3BspFile::getRandomStartLocation() const
	{
		Math::EulerAnglef rotation;
		Math::Vector3f    position; 
		
		auto entities = getEntitiesByType<Q3InfoPlayerStart>();
		if (!entities.empty())
		{
			auto entityIdx = RandomValue(0, static_cast<int>(entities.size() - 1));
			auto entity = entities[entityIdx];
			
			position = entity->m_origin;
			rotation = Math::EulerAnglef(Math::ToRadians(90.0f), ToRadians(entity->m_angle - 90.0f), 0.0f );			
		}


		return std::make_pair( position, rotation );
	}

	void Q3BspFile::draw()
	{
        using namespace Math;
        using namespace Render;
        using namespace App;

        const auto& commandList = getContext()->getSystem<CommandStack>()->getCommandList();        
        auto view				= commandList.getVariable<IView*>("ActiveView");
		
        auto camera		= view->getCamera();
		auto viewDepth  = view->getRecursionDepth();
		auto camPos		= camera->getEye();
     	auto frustum	= camera->getFrustum();
		auto leafId		= leafNodeForPosition(camPos);
		auto clusterId	= m_drawLeafs[leafId].m_cluster;
	
		const auto& currentCluster = getVisibleClusters(clusterId);
		DrawListPtr solidContent, translucentContent, portalContent;
        bool skyVisible = !false;
        int culledSurfaces = 0;
        for (auto i : currentCluster.m_visibleLeafs)
        {
            auto& leaf = m_drawLeafs[i];
            if (leaf.m_hasSky)
                skyVisible = true;
            if (!frustum.boxInside(leaf.m_bounds))
                continue;
            for (auto& drawInfo : leaf.m_drawInfoList)
            {
                auto q3Shader		 = getShader(drawInfo.m_shaderId);
                auto& activeDrawList = q3Shader->isSolid() ? solidContent : translucentContent;
                activeDrawList.push_back(&drawInfo);
            }
        }
    
        glFrontFace(GL_CCW);

        { //sky
            bool drawSky = static_cast<bool>( commandList.getVariable<int>("r_drawSky") );
            if ((drawSky & skyVisible) && m_skyBox) {
                glDepthMask( false );
                glEnable( GL_BLEND );
                glDisable(GL_CULL_FACE);
                m_skyBox->draw();
            }
        }
        {	//solids
            
            bool drawSolid = static_cast<bool>(commandList.getVariable<int>("r_drawSolid"));
            if (drawSolid) {
                glEnable(GL_CULL_FACE);
                glEnable(GL_DEPTH_TEST);
                glDepthMask(true);
                glEnable(GL_BLEND);
                DrawLeafs(this, solidContent, 0);
            }
        }
        {	//transparant
            bool drawTrans = static_cast<bool>(commandList.getVariable<int>("r_drawTransparent"));
            if (drawTrans) 
            {
                glEnable(GL_CULL_FACE);
                glEnable(GL_BLEND);
                glEnable(GL_DEPTH_TEST);
                //glDepthMask(false);
                DrawLeafs(this, translucentContent, 0);
                glDisable(GL_BLEND);
            }
        }			
    }



    bool Q3BspFile::clear()
    {
        if (!m_loaded)
            return false;

        m_loaded	= false;
        m_fileName	= "";
		
    
        m_numPolyFaces	  = 0;
        m_numPatches	  = 0;
        m_numMeshFaces	  = 0;
        m_numBillBoards	  = 0;
        m_bytesPerVisCluster = 0;
        m_numVisClusters  = 0;
        m_lightmap	      = nullptr;
        m_skyBox		  = nullptr;

		for (auto shader : m_shaders)
			shader->unloadShader();

        m_worldBounds.clearBounds();
        m_faceTriangles.clear();
        m_facePatches.clear();
        m_drawLeafs.clear();
        m_shaders.clear();
        m_entityList.clear();
        m_shaderLUT.clear();  
        m_clusterList.clear();
        m_planeList.clear();
	
        m_fileHeader.clear();
        m_entityString.clear();
        m_visData.clear();
        m_textureList.clear();       
        m_nodeList.clear();      
        m_leafFaceList.clear();
        m_leafBrushList.clear();
        m_modelList.clear();
        m_brushList.clear();
        m_brushSidesList.clear();
        m_vertexList.clear();
        m_meshVertexList.clear();
        m_effectList.clear();
        m_faceList.clear();       
        m_lightVolumeList.clear();

        return true;
    }

    String Q3BspFile::toString() const
    {
        std::stringstream result;

        result << "========= Q3 Bsp Info =========\n";
        result << "Textures:      " << m_textureList.size()		<< "\n";
        result << "Planes:        " << m_planeList.size()		<< "\n";
        result << "Nodes:         " << m_nodeList.size()		<< "\n";
        result << "Leaf faces:    " << m_leafFaceList.size()	<< "\n";
        result << "Leaf brushes:  " << m_leafBrushList.size()	<< "\n";
        result << "Models:        " << m_modelList.size()		<< "\n";
        result << "Brushes:       " << m_brushList.size()		<< "\n";
        result << "Brush sides:   " << m_brushSidesList.size()	<< "\n";
        result << "Vertices:      " << m_vertexList.size()		<< "\n";
        result << "Mesh vertices: " << m_meshVertexList.size()	<< "\n";	     
        result << "Light volumes: " << m_lightVolumeList.size() << "\n";
        result << "Total faces:   " << m_faceList.size()		<< "\n";
        result << "->Poly faces:  " << m_numPolyFaces			<< "\n";
        result << "->Patch faces: " << m_numPatches				<< "\n";
        result << "->Mesh faces:  " << m_numMeshFaces			<< "\n";
        result << "->Billboards:  " << m_numBillBoards			<< "\n";

        result << "Draw tri's:    " << m_faceTriangles.size()   << "\n";
        result << "Draw patches:  " << m_facePatches.size()     << "\n";

        result << "Bounds min:    " << m_worldBounds.getMin().toString().c_str() << "\n";
        result << "Bounds max:    " << m_worldBounds.getMax().toString().c_str() << "\n";

        return result.str();
    }

    EntityList Q3BspFile::getEntitiesByName(const String& name, bool getAll /*= false */) const
    {
        EntityList result;
        if (name.empty())
            return result;
        for (auto& ent : m_entityList)
        {
            if ( ent->m_targetName == name )
            {
                result.push_back(ent);
                if (!getAll)
                    break;
            }
        }
        return result;
    }


    EntityList Q3BspFile::getEntitiesForCluster(int clusterId) const
    {
        EntityList result;
        if (clusterId == -1)
            return result;

        auto AddEntities = [this, &result](const Q3DrawCluster& cluster )
        {
            result.insert( result.end(), 
                cluster.m_staticEntities.begin(), cluster.m_staticEntities.end());
        };
        //add current cluster
        auto curCluster = m_clusterList.at(clusterId);
        //add entities from clusters visible from this cluster
        for (auto clId : curCluster.m_visibleClusters)
            AddEntities(m_clusterList.at(clId));

        return result;
    }

	Misc::Q3ShaderPtr Q3BspFile::getShader(int id) const
	{
#if DEBUG
		assert(id >= 0 && id < m_shaders.size());
#endif
		return m_shaders[id];
	}

	int Q3BspFile::leafNodeForPosition(const Math::Vector3f& position) const
	{
		int nodeIdx = 0;
		while (nodeIdx >= 0)
		{
			auto planeIdx = m_nodeList[nodeIdx].m_splitPlane;
			const auto& plane = m_planeList[planeIdx];
			auto result = plane.classifyPoint(position, FLOAT_EPSILON);
			if (Math::POINT_FRONT_PLANE == result)
				nodeIdx = m_nodeList[nodeIdx].m_children[0];
			else
				nodeIdx = m_nodeList[nodeIdx].m_children[1];
		}
		return ~nodeIdx;
	}

	int Q3BspFile::clusterForPosition(const Math::Vector3f& position) const
	{
		return m_drawLeafs[leafNodeForPosition(position)].m_cluster;
	}

	bool Q3BspFile::clusterVisible(int inputCluster, int clusterToTest) const
	{
		if (inputCluster == -1 || inputCluster == clusterToTest)
			return true;

		int idx = inputCluster * m_bytesPerVisCluster + (clusterToTest >> 3);
		auto visibleBit = m_visData[idx] & (1 << (clusterToTest & 0x7));
		return visibleBit != 0;
	}

	IntVector Q3BspFile::getFacesOfType(std::uint32_t surfaceFlags)
    {
        IntVector result;
        for ( int i = 0; i < m_faceList.size(); ++i )
        {
            auto shader = getShader(m_faceList[i].m_texIndex);
            if (shader->hasSurfaceFlag(surfaceFlags))
                result.push_back(i);
        }
        return result;
    }


    void Q3BspFile::calculateBounds()
    {
        m_worldBounds.clearBounds();
        for (auto& vert : m_vertexList)
            m_worldBounds.updateBounds(vert.m_worldCoord);		
    }

    void Q3BspFile::offsetMap(const Math::Vector3f& offset)
    {
        for (auto& vert : m_vertexList)
            vert.m_worldCoord += offset;

        for (auto& entity : m_entityList)
            entity->m_origin += offset;
        
        for (auto& model : m_modelList) 
        for (int i = 0; i < 3; ++i) {
            model.m_max[i] += offset[i];
            model.m_min[i] += offset[i];
        }

        for( auto& node : m_nodeList )
        for (auto i = 0; i < 3; ++i) {
            node.m_max[i] += offset[i];
            node.m_min[i] += offset[i];
        }

        for (auto& leaf : m_drawLeafs)      
            leaf.m_bounds += offset;

        for (auto& plane : m_planeList) {
            auto dist		 = -plane.m_normal.dot(offset);
            plane.m_distance = plane.m_distance + dist;
        }

        calculateBounds();
        auto axis = static_cast<int>(m_worldBounds.getSize().getMaxValue());
        axis = Math::IsPowerOfTwo(axis) ? axis : Math::NextPowerOfTwo(axis);
        m_worldBounds.setMin(Math::Vector3f(0.0f));
        m_worldBounds.setMax(Math::Vector3f(static_cast<float>(axis)));
    }

    void Q3BspFile::adjustLightmapCoords(TriangleList& triangles ) const
    {
        auto setShaderLambda = [this]( Q3Triangle& tri ) ->void 
        {
            //adjust light map id & uv coordinates
            if (tri.m_lightmapId >= 0) 
            {
				const float LM_SIZE = 128.0f;
				for (auto& v : tri.m_vertices) 
                {
                    float uCoord = (v.m_uvCoord[0] * LM_SIZE)  + (tri.m_lightmapId * LM_SIZE);
                    v.m_uvCoord[0] = uCoord / m_lightmap->m_texture->getTextureWidth();
                }
                tri.m_lightmapId = 0;		
            }
            assert(tri.m_shaderId >= 0);
        };       

        std::for_each(std::begin(triangles), std::end(triangles), setShaderLambda);   
    }

    


    IntVector Q3BspFile::getFacesForCluster(const Q3DrawCluster& input, bool noDuplicates /*= true */)
    {
        IntVector result;
        for (auto leafId : input.m_visibleLeafs)
        {
            auto& leaf = m_drawLeafs[leafId];
            auto start = leaf.m_firstFace;
            auto end = start + leaf.m_numFaces;
            while (start < end) {
                result.push_back(m_leafFaceList[start].m_faceIndex);
                start++;
            }
        }
        if (noDuplicates)
            Common::SortUnique(result, true);
        return result;
    }

	ViewPortVector Q3BspFile::getPortalViews(const ViewPortPtr& activeView)
	{
		auto camera = activeView->getCamera();	
		auto camPos = camera->getEye();	
		auto leafId = leafNodeForPosition(camPos);
		auto clusterId = m_drawLeafs[leafId].m_cluster;		
		return UpdatePortalViews(this, activeView, clusterId);
	}

	Q3DrawCluster& Q3BspFile::getVisibleClusters(int clusterId)
    {
        //create a new draw cluster if not yet found
        auto it = m_clusterList.find(clusterId);
        if (it != std::end(m_clusterList))
            return it->second;

        Q3DrawCluster newCluster;
        for (const auto& leaf : m_drawLeafs)
        {
            if (leaf.m_cluster != -1) 
            {
                if (clusterVisible(clusterId, leaf.m_cluster))
                {
                    newCluster.m_visibleClusters.push_back(leaf.m_cluster);
                    newCluster.m_visibleLeafs.push_back(leaf.m_leafId);                  
                }
            }
        }
        m_clusterList[clusterId] = newCluster;		
        return m_clusterList[clusterId];
    }

    void Q3BspFile::linkEntities()
    {
        using namespace Math;
        auto cStack = m_context->getSystem<App::CommandStack>();	
    
        //use 'target' field of current entity and link with 
        //entities that has the same 'targetname' field
        for (auto& ent : m_entityList) {
            auto targetEnts = getEntitiesByName( ent->m_target, true );
            for (auto& target : targetEnts)
                ent->m_linkedEntities.push_back( target->m_slotId );
        }
        
        

        //link portals
        auto MAX_DIST = cStack->getCommandList().getVariable<float>("r_maxDistSurfacePortal"); 
        auto portalFaces = getFacesOfType(SURFACE_PORTAL); //potential portal faces
        auto portalEnts  = getEntitiesByType<Q3MiscPortalSurface>();
    
        for (const auto& entity : portalEnts) {

            const static Vector3f directions[4] =
            {
                Vector3f::LEFT, Vector3f::RIGHT, Vector3f::FORWARD, 
                Vector3f::BACK/*, Vector3f::UP, Vector3f::DOWN*/
            };
            auto clusterId = clusterForPosition(entity->m_origin);
            if( clusterId == - 1)
                continue;         

            auto& cluster = getVisibleClusters(clusterId);
            auto faceList = getFacesForCluster(cluster);			
            auto minDistance = MAX_DIST;
            int portalFaceId = -1;

            //generate ray along each of the axes
            for (const auto& dir : directions )
            {
                for (auto faceId : faceList)
                {
                    Vector3f hit;
                    if (GetRayHitFace(entity->m_origin, dir, this, faceId, hit))
                    {
                        auto newDist = entity->m_origin.distanceSquared(hit);
                        if ( newDist < MAX_DIST)
                        {
                            portalFaceId = faceId;
                            minDistance = newDist;
                        }
                    }
                }
            }
            //found a face close enough to be a portal?
            if (portalFaceId != -1)
            {
                auto drawPortal				  = std::make_shared<Q3DrawPortal>( m_context );
                drawPortal->m_originalFace	  = portalFaceId;
				drawPortal->m_origin		  = entity->m_origin;
				drawPortal->m_portalTriangles = parsePolyFace(m_faceList[portalFaceId], portalFaceId); //get triangles for face
				drawPortal->m_portalPlane	  = GetFacePlane( this, portalFaceId );
                drawPortal->m_bounds		  = GetFaceBounds(this, portalFaceId).expanded( 0.1f );
				
				if (!entity->isMirror()) {
					drawPortal->m_portalCamera = std::dynamic_pointer_cast<Q3MiscPortalCamera>(m_entityList[entity->getPortalCameraIndex()]);
					assert(drawPortal->m_portalCamera);
				}
                //add portal to this cluster	
                cluster.m_staticEntities.push_back(drawPortal);
            }
        }
        
    }

    void Q3BspFile::buildDrawClusters()
    {
        for (auto drawLeaf : m_drawLeafs) {
            getVisibleClusters(drawLeaf.m_cluster);
        }
    }

    bool Q3BspFile::parseLumps(const String& fileName)
    {
        m_loaded = false;
        using FileInputStream = App::FileInputStream;
        FileInputStream ifs(m_context);
        if (!ifs.open( (Q3GetMapPath() + fileName ).c_str(), App::FILE_TYPE_BINARY)) {
            return false;
        }
        ifs.read(&m_fileHeader, 1);
        if (!m_fileHeader.valid()) {
           return false;
        }

        const auto& commandList = m_context->getSystem<App::CommandStack>()->getCommandList();		
        auto lightmapScale		= commandList.getVariable<float>("r_lightmapScale");

        //Entity string 
        Q3LumpInfo curEntry = m_fileHeader.m_DirEntries[ENTITY_LUMP];
        if (curEntry.m_Length)
        {
            m_entityString.resize(curEntry.m_Length);
            ifs.setOffset(curEntry.m_Offset);
            ifs.read(&m_entityString[0], static_cast<int>(m_entityString.size()));
        }
        //texture lump
        curEntry = m_fileHeader.m_DirEntries[TEXTURE_LUMP];
        if (curEntry.m_Length)
        {
            m_textureList.resize(curEntry.m_Length / sizeof(Q3ShaderInfo));
            ifs.setOffset(curEntry.m_Offset);
            ifs.read(&m_textureList[0], static_cast<int>(m_textureList.size()));
        }
        //plane data
        curEntry = m_fileHeader.m_DirEntries[PLANE_LUMP];
        if (curEntry.m_Length)
        {
            
            std::vector<Q3Plane>			planeList;			
            planeList.resize(curEntry.m_Length / sizeof(Q3Plane));
            ifs.setOffset(curEntry.m_Offset);
            ifs.read(&planeList[0], static_cast<int>(planeList.size()));
            for (const auto& pl : planeList)
                m_planeList.emplace_back(pl.m_normal[0], pl.m_normal[1], pl.m_normal[2], -pl.m_distance);
        }
        //node data
        curEntry = m_fileHeader.m_DirEntries[NODE_LUMP];
        if (curEntry.m_Length)
        {
            m_nodeList.resize(curEntry.m_Length / sizeof(Q3BspNode));
            ifs.setOffset(curEntry.m_Offset);
            ifs.read(&m_nodeList[0], static_cast<int>(m_nodeList.size()));
        }
        //leaf nodes
        curEntry = m_fileHeader.m_DirEntries[LEAF_LUMP];
        if (curEntry.m_Length)
        {
           
            std::vector<Q3BspLeaf> leafList;
            leafList.resize(curEntry.m_Length / sizeof(Q3BspLeaf));
            ifs.setOffset(curEntry.m_Offset);
            ifs.read(&leafList[0], static_cast<int>(leafList.size()));			
            for (auto i = 0; i < leafList.size(); ++i)				
                m_drawLeafs.emplace_back(  leafList[i], i );		
        }
        curEntry = m_fileHeader.m_DirEntries[LEAF_FACES_LUMP];
        if (curEntry.m_Length)
        {
            m_leafFaceList.resize(curEntry.m_Length / sizeof(Q3LeafFace));
            ifs.setOffset(curEntry.m_Offset);
            ifs.read(&m_leafFaceList[0], static_cast<int>(m_leafFaceList.size()));
        }
        curEntry = m_fileHeader.m_DirEntries[LEAF_BRUSHES_LUMP];
        if (curEntry.m_Length)
        {
            m_leafBrushList.resize(curEntry.m_Length / sizeof(Q3LeafBrush));
            ifs.setOffset(curEntry.m_Offset);
            ifs.read(&m_leafBrushList[0], static_cast<int>(m_leafBrushList.size()));
        }
        curEntry = m_fileHeader.m_DirEntries[MODELS_LUMP];
        if (curEntry.m_Length)
        {
            m_modelList.resize(curEntry.m_Length / sizeof(Q3Model));
            ifs.setOffset(curEntry.m_Offset);
            ifs.read(&m_modelList[0], static_cast<int>(m_modelList.size()));
        }
        curEntry = m_fileHeader.m_DirEntries[BRUSHES_LUMP];
        if (curEntry.m_Length)
        {
            m_brushList.resize(curEntry.m_Length / sizeof(Q3Brush));
            ifs.setOffset(curEntry.m_Offset);
            ifs.read(&m_brushList[0], static_cast<int>(m_brushList.size()));
        }
        curEntry = m_fileHeader.m_DirEntries[BRUSH_SIDES_LUMP];
        if (curEntry.m_Length)
        {
            m_brushSidesList.resize(curEntry.m_Length / sizeof(Q3BrushSide));
            ifs.setOffset(curEntry.m_Offset);
            ifs.read(&m_brushSidesList[0], static_cast<int>(m_brushSidesList.size()));
        }
        curEntry = m_fileHeader.m_DirEntries[VERTEX_LUMP];
        if (curEntry.m_Length)
        {
            std::vector<Q3Vertex> vList;
            vList.resize(curEntry.m_Length / sizeof(Q3Vertex));
            ifs.setOffset(curEntry.m_Offset);
            ifs.read(&vList[0], static_cast<int>(vList.size()));
            for (const auto& v : vList)
                m_vertexList.push_back(convertToNativeVertex(v));
        }
        curEntry = m_fileHeader.m_DirEntries[MESH_VERTEX_LUMP];
        if (curEntry.m_Length)
        {
            m_meshVertexList.resize(curEntry.m_Length / sizeof(Q3MeshVertices));
            ifs.setOffset(curEntry.m_Offset);
            ifs.read(&m_meshVertexList[0], static_cast<int>(m_meshVertexList.size()));
        }
        curEntry = m_fileHeader.m_DirEntries[EFFECTS_LUMP];
        if (curEntry.m_Length)
        {
            m_effectList.resize(curEntry.m_Length / sizeof(Q3Effect));
            ifs.setOffset(curEntry.m_Offset);
            ifs.read(&m_effectList[0], static_cast<int>(m_effectList.size()));
        }
        curEntry = m_fileHeader.m_DirEntries[FACES_LUMP];
        if (curEntry.m_Length)
        {
            m_faceList.resize(curEntry.m_Length / sizeof(Q3Face));
            ifs.setOffset(curEntry.m_Offset);
            ifs.read(&m_faceList[0], static_cast<int>(m_faceList.size()));
        }
        curEntry = m_fileHeader.m_DirEntries[LIGHTMAP_LUMP];
        if (curEntry.m_Length)
        {
            std::vector<Q3LightMap>			lightmapList;
            
            lightmapList.resize(curEntry.m_Length / sizeof(Q3LightMap));
            ifs.setOffset(curEntry.m_Offset);
            ifs.read(&lightmapList[0], static_cast<int>(lightmapList.size()));
            
            for (auto& lm : lightmapList)
                lm.rescale(lightmapScale);

            //convert lightmaps 
            loadLightMaps(lightmapList);

        }
        curEntry = m_fileHeader.m_DirEntries[LIGHTVOLS_LUMP];
        if (curEntry.m_Length)
        {
            m_lightVolumeList.resize(curEntry.m_Length / sizeof(Q3LightVolume));
            ifs.setOffset(curEntry.m_Offset);
            ifs.read(&m_lightVolumeList[0], static_cast<int>(m_lightVolumeList.size()));
        }
        curEntry = m_fileHeader.m_DirEntries[VIS_DATA_LUMP];
        if (curEntry.m_Length)
        {
            ifs.setOffset(curEntry.m_Offset);
            ifs.read(&m_numVisClusters, 1 );
            ifs.read(&m_bytesPerVisCluster, 1);
            m_visData.resize(m_numVisClusters * m_bytesPerVisCluster);
            ifs.read(&m_visData[0], static_cast<int>(m_visData.size()));
        }

        m_fileName  = fileName;
        m_loaded	= true;
        return m_loaded;
    }

    TriangleList Q3BspFile::triangulateFace(int faceId) const
    {
        TriangleList result;

        const auto& face = m_faceList[faceId];
        if (face.m_faceType == POLYFACE)
        {
            result = parsePolyFace(face, faceId);
            m_numPolyFaces++;
        }
        else if (face.m_faceType == PATCHFACE) {
            result = parsePatchFace(face, faceId);
            m_numPatches++;
        }
        else if (face.m_faceType == MESHFACE)
        {
            result = parseMeshFace(face, faceId);
            m_numMeshFaces++;
        }
        else if (face.m_faceType == BILLBOARD)
        {
            result = parseBillBoardFace(face, faceId);
            m_numBillBoards++;
        }
        else
            AddConsoleMessage(m_context, "Unknown face id " + std::to_string(face.m_faceType), App::LOG_LEVEL_WARNING);
        
        adjustLightmapCoords(result);
        return result;
    }

    

    bool Q3BspFile::parseEntityString()
    {
        auto parseEntity = [this](const std::string& entityString) -> std::map<String, String> 
        {
            using namespace Common;
            std::istringstream input(entityString);			
            //assign key-value pairs
            std::map<String, String> keyPairs;
            for (std::string line; std::getline(input, line);)
            {
                if (line.empty())
                    continue;

                line.erase(0, 1); 
                line.erase(line.size() - 1);		//eat first & last '"' characters
                
                auto key = line.substr(0, line.find_first_of('"'));
                auto value = line.substr(line.find_last_of('"') + 1);
                if (key.length() && value.length())
                    keyPairs[key] = value;
            }
            return keyPairs;			
        };
        
        std::string result( reinterpret_cast<const char*>( &m_entityString[0]), m_entityString.size() );					
        enum eEntityState
        {
            STATE_GLOBAL		= 0,
            STATE_PARSE_ENTITY	= 1
        };
        StringList splitEntities;
        String curEntity;
        auto curLine = 0;
        auto curState = STATE_GLOBAL;
        for (auto curChar : result)
        {
            if (curChar == '\n')
                curLine++;

            if (curChar == '{')
            {
                assert(STATE_GLOBAL == curState);
                curState = STATE_PARSE_ENTITY;
                curEntity = "";
            }
            else if (curChar == '}')
            {
                assert(STATE_PARSE_ENTITY == curState);
                curState = STATE_GLOBAL;
                splitEntities.push_back(curEntity);				
            }
            else if (STATE_PARSE_ENTITY == curState) {
                curEntity += curChar;
            }		
        }
        for (auto& splitEntity : splitEntities) 
        {
            auto keyPairs = parseEntity(splitEntity);
            auto entity   = EntityForClassName(keyPairs, getContext() );
            if (entity) {
                entity->m_slotId	= static_cast<int>(m_entityList.size());
                entity->m_keyValue	= keyPairs;
			    m_entityList.emplace_back(entity);
            }
            else
            {
                qDebug() << "!!!** Unkown Entity:" << keyPairs.at( "classname" ).c_str() << "**!!!";
                qDebug() << splitEntity.c_str() << "\n";				
            }		
        }
        return true;
    }

    bool Q3BspFile::parseShaderDirectory()
    {
        auto files = App::getFilesInFolder( Q3GetShaderPath().c_str(), { "*.shader" });				
        
        for (auto file : files) {
            
            Q3ParseShader parseShader( m_context, Q3GetShaderPath() + file.toStdString());
            if( parseShader.parseShaderFile() )
            {
                //add shaders to LUT 
                for( const auto& shader: parseShader.m_shaders )
                {
                    const auto& shaderName = shader->m_name;
                    if ( m_shaderLUT.find( shaderName ) != std::end(m_shaderLUT)) 
                    {
                        AddConsoleMessage( m_context, String( "Duplicate Shader: " ) + shaderName, App::LOG_LEVEL_WARNING );
                        continue;
                    }
                    m_shaderLUT[shaderName] = std::move(shader);
                }
            }
        }
        
        AddConsoleMessage( m_context, String( "Num shaders parsed: " ) + 
            std::to_string( m_shaderLUT.size() ), App::LOG_LEVEL_WARNING );
        
        return true;
    }


    bool Q3BspFile::loadShaders()
    {
        auto fallbackShader = Q3Shader::CreateFallBackShader(m_context); //create a fallback shader
        std::vector<Q3ShaderPtr> curMapShaders;	
        //iterate though shader list
        for( const auto& curTex : m_textureList )
        {
            String name = reinterpret_cast<const char*>(curTex.m_texName);			
            if( std::end( m_shaderLUT ) == m_shaderLUT.find(name) )  //not found in shader scripts, use regular shader
            {
                auto result = Q3Shader::CreateRegularShader(m_context, name);
                m_shaderLUT[name] = result ? result : fallbackShader;				
                curMapShaders.push_back(m_shaderLUT[name]);
                continue;
            }		
            curMapShaders.push_back(m_shaderLUT[name]);
        }

        {
            int numShadersLoaded = 0;
            int numGLSLGenerated = 0;
            int numGLSLErrors    = 0;
            
            for (auto it : curMapShaders ) //load textures & generate glsl code
            {
                it->beginLoad();
                if ( it->getStatus() == App::RESOURCE_LOADED ) 
                {
                    for (auto& stage : it->m_shaderStages) //set lightmap
                    {
                        if (stage.m_lightmap) {//add lightmap
                            stage.m_textureList.push_back(m_lightmap);
                        }
                    }
                    numShadersLoaded++;
                    if (it->generateGLSL()) 
                        numGLSLGenerated++;					
                    else {
                        numGLSLErrors++;
                        AddConsoleMessage(m_context, String("Error compiling shader: ") + it->m_name, App::LOG_LEVEL_WARNING);
                    }
                }			
                if (eQ3SurfaceParam::SURFACE_SKY & it->m_sufaceFlags)
                {
                    if (!m_skyBox)
                        m_skyBox = std::make_shared<Q3DrawSky>( m_context, it );
                }
            }		
            
            AddConsoleMessage( m_context, String( "Map shaders found: " )      +	std::to_string( curMapShaders.size() ) );
            AddConsoleMessage( m_context, String( "Map shaders loaded: " )     +	std::to_string( numShadersLoaded ) );
            AddConsoleMessage( m_context, String( "#Compiled shaders(GLSL): ") +	std::to_string( numGLSLGenerated ) );
            AddConsoleMessage( m_context, String( "#Error shaders(GLSL): ") +		std::to_string( numGLSLErrors ), App::LOG_LEVEL_WARNING);
        }   
        m_shaders = curMapShaders;
        return true;
    }

  

    bool Q3BspFile::loadLightMaps( const std::vector<Q3LightMap>& lightmaps )
    {
		//lambda to avoid code copy pasting
		auto SetWhiteLightmap = [this](const String& errorMsg)-> bool
		{
			const auto rMan = getContext()->getSystem<ResManager>();
			App::AddConsoleMessage(m_context, errorMsg, App::LOG_LEVEL_WARNING);
			m_lightmap = rMan->getResourceSafe<Texture>("DefaultWhiteTexture");
			return errorMsg.empty();
		};

	    int width = static_cast<int>(lightmaps.size()) * 128;		
		if (!width) 
			return SetWhiteLightmap("File contains no lightmaps");
		//resize new lightmap texture to a power of two
	    width = Math::IsPowerOfTwo(width) ? width : Math::NextPowerOfTwo( width );        
		Common::Image newImg;
        if (!newImg.loadFromMemory(nullptr, Common::IMAGE_FORMAT_RGB8_UI, width, 128))
      		return SetWhiteLightmap("Error creating light map atlas");
       
        newImg.m_data.assign(newImg.m_data.size(), (std::uint8_t)255);
        for (auto i = 0; i < static_cast<int>(lightmaps.size()); ++i)
        {
            const auto& lm = lightmaps[i];
            Common::Image lmImg;
			bool succes = true;
            succes &= lmImg.loadFromMemory(lm.m_lightmapData, Common::IMAGE_FORMAT_RGB8_UI, 128, 128);
            succes &= newImg.setSubImage(i * 128, 0, lmImg);
			if (!succes)
				return SetWhiteLightmap("Error inserting image into atlas");
		}
       
        m_lightmap = std::make_shared<App::Texture>(m_context);
        m_lightmap->m_params.m_clampMode_S = GL_CLAMP_TO_EDGE;
        m_lightmap->m_params.m_clampMode_T = GL_CLAMP_TO_EDGE;

		Common::MipMapPixelFilter filter;
		newImg.generateMipmaps(&filter);
        if (!m_lightmap->setFromImage(newImg))
   			return SetWhiteLightmap("Error creating gpu-texture for lightmap");
             
		return true;
    }


    void Q3BspFile::buildVAOForLeafs()
    {
        auto AddVertices = [this](TriangleList& triangeList, Q3DrawLeaf& leaf )
        {
            std::sort(std::begin(triangeList), std::end(triangeList),
                [](const Q3Triangle& a, const Q3Triangle& b)
            {
                return a.m_faceid < b.m_faceid;
            });

            auto addDrawInfo = [this]( Q3DrawInfo& di, int faceId ) -> bool
            {
                auto AUTO_SPRITE	= static_cast<int>(eQ3VertexDeformFunc::VD_AUTOSPRITE);
                auto AUTO_SPRITE2	= static_cast<int>(eQ3VertexDeformFunc::VD_AUTOSPRITE2);
                auto shader			= m_shaders[di.m_shaderId];
                auto& vertices		= m_drawLeafs[di.m_leafId].m_vertexList;
                
                if ( !shader->bind() )
                    return false;		

                bool succes = true;
                if (auto flags = shader->hasAutoSprite())
                {
                    if (this->m_faceList[faceId].m_numVerts != 4) 
                    {
                        App::AddConsoleMessage(this->getContext(), "Invalid autoSprite face", App::LOG_LEVEL_WARNING);
                        succes = false;
                    }
                    if (succes) 
                    {
                        if (AUTO_SPRITE & flags)
                            succes &= GetFaceAutoSprite(this, faceId, di);
                        if (AUTO_SPRITE2 & flags)
                            succes &= GetFaceAutoSprite2(this, faceId, di);
                        if (succes) //sort uv coords? 
                            succes &= AutoSpriteSort(this, di, faceId, vertices );
                    }
                }
                if (succes)
                    m_drawLeafs[di.m_leafId].m_drawInfoList.push_back(di);
                
                shader->unBind();
                return succes;
            };

            const auto& commandList = m_context->getSystem<App::CommandStack>()->getCommandList();
            auto dbgShowLeafs = commandList.getVariable<int>("dbg_show_clusters") != 0;
            
            auto& startTriangle	= triangeList[0];			
            auto shaderId		= startTriangle.m_shaderId;
            auto shader			= m_shaders[shaderId];
            auto leafId			= leaf.m_leafId;
            auto lightMapId		= 0;			
            auto startVert		= static_cast<int>(leaf.m_vertexList.size());
            auto vertCount		= 0;		
            auto isSkyShader    = shader->hasSurfaceFlag(static_cast<int>(eQ3SurfaceParam::SURFACE_SKY));
            //tag leaf as having sky
            leaf.m_hasSky |= isSkyShader;

            if (!isSkyShader) //ignore triangles with sky shaders
            {
                BBox3f	bounds;
                auto lastFaceId = startTriangle.m_faceid;
                auto curFaceId = lastFaceId;
                for (auto& triangle : triangeList)
                {
                    curFaceId = triangle.m_faceid;
                    if (shader->hasAutoSprite() && (lastFaceId != curFaceId)) //new face found
                    {
                        Q3DrawInfo info(shaderId, lightMapId, leafId, startVert, vertCount, bounds);
                        addDrawInfo(info, lastFaceId);

                        lastFaceId = curFaceId;
                        bounds.clearBounds();
                        startVert += vertCount;
                        vertCount = 0;
                    }
                    for (auto vert : triangle.m_vertices)
                    {
                        if( dbgShowLeafs && leaf.m_cluster != -1 )
                            vert.m_normalTangent.setW(leaf.m_cluster & 255 );					
                        leaf.m_vertexList.push_back(vert);
                        bounds.updateBounds(vert.m_worldCoord);
                        vertCount++;
                    }
                }
                if (vertCount)
                {
                    Q3DrawInfo info(shaderId, lightMapId, leafId, startVert, vertCount, bounds);
                    addDrawInfo(info, curFaceId);
                }
            }
        
            triangeList.clear();
            
        };
        


        for (auto& leaf : m_drawLeafs )
        {
            TriangleList triangeList = GetTrianglesForLeaf( this, leaf.m_leafId, 0 ); //triangles sorted by shader id
            if (triangeList.empty())
                continue;

            TriangleList faceList;
            auto lastShaderId = triangeList[0].m_shaderId;
            for (const auto& triangle : triangeList ) 
            {
                auto  curShaderId = triangle.m_shaderId;				
                if (lastShaderId != curShaderId)
                {
                    AddVertices( faceList, leaf ); //sort by triangles face id
                    lastShaderId = curShaderId;				
                }
                faceList.push_back(triangle);
            }
            if ( !faceList.empty() )
            {
                AddVertices(faceList, leaf);
            }	
            
            
            leaf.m_vaoBuffer = CreateVAO( m_context, leaf.m_vertexList.data(), leaf.m_vertexList.size());
        }				
    }

    

    TriangleList Q3BspFile::parsePolyFace(const Q3Face& face, int faceId) const
    {
        TriangleList result;
        Q3Triangle newTriangle(Math::Vector3f(face.m_normal), faceId, face.m_texIndex, face.m_lightmap);
  
        for( int i = 0; i < face.m_numMeshVerts; i += 3 )
        {
            auto offset_1 = m_meshVertexList[face.m_meshVert + i + 0].m_vertexOffset;
            auto offset_2 = m_meshVertexList[face.m_meshVert + i + 1].m_vertexOffset;
            auto offset_3 = m_meshVertexList[face.m_meshVert + i + 2].m_vertexOffset;
        
            newTriangle.m_vertices[2] = m_vertexList[face.m_vertex + offset_1];
            newTriangle.m_vertices[1] = m_vertexList[face.m_vertex + offset_2];
            newTriangle.m_vertices[0] = m_vertexList[face.m_vertex + offset_3];			
            newTriangle.calcNormal();			//calculate normal ourself
            newTriangle.m_faceType = POLYFACE;
            result.push_back(newTriangle);

        }
        return result;
    }

    TriangleList Q3BspFile::parsePatchFace(const Q3Face& face, int origFaceIdx) const
    {
        
        
        Q3Patch newPatch;
        newPatch.m_shaderId		= face.m_texIndex;
        newPatch.m_lightmapId	= face.m_lightmap;
        newPatch.m_width		= face.m_patchSize[0];
        newPatch.m_height		= face.m_patchSize[1];

        auto numPatchesX = (newPatch.m_width - 1) / 2;
        auto numPatchesY = (newPatch.m_height - 1) / 2;

        newPatch.m_patches.resize(numPatchesX * numPatchesY);

        for (auto y = 0; y < numPatchesY; ++y) {
            for (auto x = 0; x < numPatchesX; ++x) {
                auto* cPoint = newPatch.m_patches[y * numPatchesX + x].m_controls;
                for (auto row = 0; row < 3; ++row) {
                    for (auto point = 0; point < 3; ++point) {

                        int vIndex = face.m_vertex;
                        vIndex += y * 2 * newPatch.m_width + x * 2;
                        vIndex += row * newPatch.m_width + point;
                        cPoint[row * 3 + point] = m_vertexList[vIndex];
                    }
                }
                auto tmpTris = newPatch.m_patches[y * numPatchesX + x].tesselate( face, origFaceIdx );	
                newPatch.m_triangles.insert(newPatch.m_triangles.end(), tmpTris.begin(), tmpTris.end());
              //  newPatch.calcBounds();			
            }
        }	
        newPatch.calcBounds();
        newPatch.smoothPatchNormals();
        m_facePatches.push_back(newPatch);	
        return newPatch.m_triangles;    
    }
    
    TriangleList Q3BspFile::parseMeshFace(const Q3Face& face, int origFaceIdx) const
    {
        auto result = parsePolyFace(face, origFaceIdx);
        for (auto& triangle : result)
            triangle.m_faceType = MESHFACE;
        return result;
    }


    TriangleList Q3BspFile::parseBillBoardFace(const Q3Face& face, int origFaceIdx) const
    {
        TriangleList result;
        return result;	
    }
    


   

    Q3DrawLeaf::Q3DrawLeaf() 
        : Q3DrawLeaf(-1, -1, -1, -1, -1, -1 )
    {

    }

    Q3DrawLeaf::Q3DrawLeaf(int leaf, int cluster, int firstBrush, int numBrushes, int firstFace, int numFaces)
        : m_leafId(leaf)
        , m_cluster(cluster)
        , m_firstBrush(firstBrush)
        , m_numBrushes(numBrushes)
        , m_firstFace(firstFace)
        , m_numFaces(numFaces)
        , m_hasSky(false)
    {

    }

    Q3DrawLeaf::Q3DrawLeaf(const Q3BspLeaf& leafNode, int leafId) : m_leafId(leafId)
        , m_cluster(leafNode.m_visCluster)
        , m_firstBrush(leafNode.m_firstLeafBrush)
        , m_numBrushes(leafNode.m_numLeafBrushes)
        , m_firstFace(leafNode.m_firstLeafFace)
        , m_numFaces(leafNode.m_numLeafFaces)
        , m_hasSky(false)
    {
        m_bounds.setMin(Math::Vector3f(leafNode.m_min[0], leafNode.m_min[1], leafNode.m_min[2]));
        m_bounds.setMax(Math::Vector3f(leafNode.m_max[0], leafNode.m_max[1], leafNode.m_max[2]));
    }

    Q3DrawLeaf::~Q3DrawLeaf()
    {

    }
        
    Q3DrawSky::Q3DrawSky(App::EngineContext* context, Q3ShaderPtr& shader, float width, float height )
        : m_context( context )
        , m_shader( shader )
        , m_skyBoxWidth(width )
        , m_eyeHeight( height )
    {
        if (!createSkyBox())
            throw std::exception("Create SkyBox failed");
    }

    bool Q3DrawSky::createSkyBox()
    {
        using namespace Render;	

        const Vertex skyVertices[] =
        {
            //top
            {-m_skyBoxWidth, -m_skyBoxWidth,	m_skyBoxWidth - m_eyeHeight, 0.0f, 0.0f},
            {-m_skyBoxWidth,  m_skyBoxWidth,	m_skyBoxWidth - m_eyeHeight, 0.0f, 1.0f},
            { m_skyBoxWidth,  m_skyBoxWidth,	m_skyBoxWidth - m_eyeHeight, 1.0f, 1.0f},

            { -m_skyBoxWidth, -m_skyBoxWidth,	m_skyBoxWidth - m_eyeHeight, 0.0f, 0.0f},
            {  m_skyBoxWidth,  m_skyBoxWidth,	m_skyBoxWidth - m_eyeHeight, 1.0f, 1.0f},
            {  m_skyBoxWidth, -m_skyBoxWidth,	m_skyBoxWidth - m_eyeHeight, 1.0f, 0.0f},

            //left
            {-m_skyBoxWidth, -m_skyBoxWidth,	-m_skyBoxWidth - m_eyeHeight, 0.0f, 0.0f},
            {-m_skyBoxWidth,  m_skyBoxWidth,	-m_skyBoxWidth - m_eyeHeight, 0.0f, 1.0f},
            {-m_skyBoxWidth,  m_skyBoxWidth,	 m_skyBoxWidth - m_eyeHeight,  1.0f, 1.0f},

            {-m_skyBoxWidth, -m_skyBoxWidth,	-m_skyBoxWidth - m_eyeHeight, 0.0f, 0.0f},
            {-m_skyBoxWidth,  m_skyBoxWidth,	 m_skyBoxWidth - m_eyeHeight,  1.0f, 1.0f},
            {-m_skyBoxWidth,  -m_skyBoxWidth,	 m_skyBoxWidth - m_eyeHeight, 1.0f, 0.0f},
            //right
            {m_skyBoxWidth, -m_skyBoxWidth,		-m_skyBoxWidth - m_eyeHeight,  0.0f, 0.0f},
            {m_skyBoxWidth,  -m_skyBoxWidth,	 m_skyBoxWidth - m_eyeHeight, 0.0f, 1.0f},
            {m_skyBoxWidth,  m_skyBoxWidth,		 m_skyBoxWidth - m_eyeHeight,   1.0f, 1.0f},

            {m_skyBoxWidth, -m_skyBoxWidth,		-m_skyBoxWidth - m_eyeHeight, 0.0f, 0.0f},
            {m_skyBoxWidth,  m_skyBoxWidth,		 m_skyBoxWidth - m_eyeHeight,  1.0f, 1.0f},
            {m_skyBoxWidth,  m_skyBoxWidth,		-m_skyBoxWidth - m_eyeHeight, 1.0f, 0.0f},

            //front
            { -m_skyBoxWidth, -m_skyBoxWidth,	-m_skyBoxWidth - m_eyeHeight,  0.0f, 0.0f},
            { -m_skyBoxWidth, -m_skyBoxWidth,	 m_skyBoxWidth - m_eyeHeight,   0.0f, 1.0f },
            { m_skyBoxWidth,  -m_skyBoxWidth,	 m_skyBoxWidth - m_eyeHeight,   1.0f, 1.0f },

            { -m_skyBoxWidth, -m_skyBoxWidth,	-m_skyBoxWidth - m_eyeHeight,  0.0f, 0.0f },
            { m_skyBoxWidth,  -m_skyBoxWidth,	 m_skyBoxWidth - m_eyeHeight,  1.0f, 1.0f },
            { m_skyBoxWidth,  -m_skyBoxWidth,	-m_skyBoxWidth - m_eyeHeight,  1.0f, 0.0f },

            //back
            { -m_skyBoxWidth, m_skyBoxWidth,	-m_skyBoxWidth - m_eyeHeight,  0.0f, 0.0f},
            { m_skyBoxWidth, m_skyBoxWidth,		-m_skyBoxWidth - m_eyeHeight,   0.0f, 1.0f },
            { m_skyBoxWidth, m_skyBoxWidth,		 m_skyBoxWidth - m_eyeHeight,   1.0f, 1.0f },

            { -m_skyBoxWidth, m_skyBoxWidth,	-m_skyBoxWidth - m_eyeHeight,  0.0f, 0.0f },
            { m_skyBoxWidth,  m_skyBoxWidth,	 m_skyBoxWidth - m_eyeHeight,  1.0f, 1.0f },
            { -m_skyBoxWidth,  m_skyBoxWidth,	 m_skyBoxWidth - m_eyeHeight,  1.0f, 0.0f }
        };
    
        auto renderer = m_context->getSystem<OpenGLRenderer>();
        m_vaoBuffer = VaoBuffer(renderer->createVertexArrayObject());
        m_vaoBuffer->setInterleaved(true);

        auto _FlOAT		= GetConstant("FLOAT");
        auto _UBYTE		= GetConstant("UNSIGNED_BYTE");
        auto _DRAWMODE	= GetConstant("STATIC_DRAW");	
        auto _VERTEXSIZE	= sizeof(Vertex);
        auto _BUFFERSIZE	= 30 * _VERTEXSIZE;
        

        VertexBuffer vBuffers[] = {
            VertexBuffer(skyVertices, _FlOAT, 3, _VERTEXSIZE, _DRAWMODE, 0,  0, 0, 0,  _BUFFERSIZE),
            VertexBuffer(skyVertices, _UBYTE, 4, _VERTEXSIZE, _DRAWMODE, 1,  1, 0, 12, _BUFFERSIZE),
            VertexBuffer(skyVertices, _FlOAT, 2, _VERTEXSIZE, _DRAWMODE, 2,  0, 0, 16, _BUFFERSIZE),
            VertexBuffer(skyVertices, _FlOAT, 2, _VERTEXSIZE, _DRAWMODE, 3,  0, 0, 24, _BUFFERSIZE),
            VertexBuffer(skyVertices, _UBYTE, 4, _VERTEXSIZE, _DRAWMODE, 4,  1, 0, 32, _BUFFERSIZE)
        };
        for (const auto& vb : vBuffers)
            m_vaoBuffer->addVertexBuffer(vb);		
    
        return m_vaoBuffer->initialize();
    }

    void Q3DrawSky::draw()
    {
        using namespace Render;
        using namespace App;
        static auto resMan = m_context->getSystem<ResourceManager>();	
        
        ResourceWrapper<Q3Shader>			shader(m_shader);
        ResourceWrapper<VertexArrayObject>  vao(m_vaoBuffer);

		 shader->m_useSkyBox.setData(1);

		vao->setDrawInformation( GL_TRIANGLES, 0, 30, false );
		vao->draw();
    }

	bool Q3DrawCluster::containsLeaf(int leafIdx) const
	{
		return std::find(std::begin(m_visibleLeafs), std::end(m_visibleLeafs), leafIdx) != std::end(m_visibleLeafs);
	}

	Q3DrawPortal::Q3DrawPortal(ContextPointer context) 
		: Q3Entity(context)
	{

	}
	bool Q3DrawPortal::updatePortal(ViewPortPtr activeView)
	{
		auto activeCam  = activeView->getCamera();
		auto activeRot  = activeCam->getRotation();
		auto activeEye  = activeCam->getEye();
		auto camForward = activeCam->getCameraVector(App::eCamVector::CAM_FORWARD);
		auto frustum    = activeCam->getFrustum();
	
		if (m_portalPlane.m_normal.dot(camForward) > 0.0f)
			return false;
		if (!frustum.boxInside( m_bounds ) )
			return false; 		  
		
		//simple lambda for debugging of portals
		auto debugRender = m_context->getSystem<DebugRender>();
		auto addDebugPortal = [this, debugRender](Q3DrawPortal* portal, const Vector4ub& rgba) -> void
		{
			for (auto& tri : portal->m_portalTriangles)
			{
				const auto& vList = tri.m_vertices;
				Vector3f points[3] = { vList[0].m_worldCoord , vList[1].m_worldCoord, vList[2].m_worldCoord };
				debugRender->addClosedPolyLine(points, 3, rgba, rgba, DEBUG_DRAW_3D_DEPTH, 30);
			}
			Math::BBox3f bounds(portal->m_origin, 8);
			debugRender->addBounds(bounds, rgba, DEBUG_DRAW_3D_DEPTH, 30);
		};		
		
		auto portalView = getPortalView();
		portalView->setViewport(Common::ViewRect(activeCam->getWidth(), activeCam->getHeight()));
		portalView->setRecursionDepth( activeView->getRecursionDepth() + 1 );
		
		auto portalCam = portalView->getCamera();
		portalCam->setFov(activeCam->getFov());
		portalCam->setNear(activeCam->getNear());
		portalCam->setFar(activeCam->getFar());		

		if (isMirror())
		{
			auto mirrorRot = Reflect( activeRot, m_portalPlane);
			auto mirrorDist = m_portalPlane.distanceToPlane(activeEye) * -2.0f;
			auto mirrorPos = activeEye + m_portalPlane.getNormal() * mirrorDist;
			
			portalView->enableClipPlane(0, m_portalPlane);

			portalCam->setRotation(mirrorRot);
			portalCam->setEye(mirrorPos);
			portalCam->updateTransforms();
		
			auto color = Math::Vector4ub(0, 255, 0, 255);			
			debugRender->addBounds(Math::BBox3f(mirrorPos, 4), color, DEBUG_DRAW_3D, 30);
			//debugRender->addFrustum(portalCam->getFrustum(), color, DEBUG_DRAW_3D_DEPTH, 15);		
			addDebugPortal(this, color);
		}
		else //'real' portal
		{
			
			
			auto cameraYaw	 = ToDegrees(QuaternionToEulerAngle(activeRot).getYaw());
			auto camRotateTo = GetRotationTo( m_portalCamera->m_direction, AngleToDirection( cameraYaw ) );
			auto camRoll	 = AxisAngleToQuaternion(AxisAnglef(0.0f, 1.0f, 0.0f, ToRadians(m_portalCamera->m_portalCurRotY)));
			
			portalCam->setRotation( ( camRotateTo * camRoll * activeRot ).getNormalized()  );
			portalCam->setEye( m_portalCamera->m_origin );
			portalCam->updateTransforms();


			auto color = Vector4ub(0, 0, 255, 255);		
			debugRender->addBounds( BBox3f( m_portalCamera->m_origin, 4 ), color, DEBUG_DRAW_3D, 30 );
			addDebugPortal(this, color);
		}
	

		return true;
	}

	Misc::PortalView Q3DrawPortal::getPortalView() const
	{
		if (!m_portalView)
			m_portalView = std::make_shared<App::RenderToTextureView>(m_context);
		return m_portalView;
	}

	VaoBuffer Q3DrawPortal::getVAOBuffer() const
	{
		if (!m_vaoBuffer)
			m_vaoBuffer = std::make_shared<Render::OpenGLVertexArrayObject>(m_context->getSystem<Render::OpenGLRenderer>().get());
		return m_vaoBuffer;
	}

	bool Q3DrawPortal::isMirror() const
	{
		return (m_portalCamera == nullptr);
	}

 }//namespace misc
