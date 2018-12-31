#pragma once
#include <map>
#include <Resource/IResource.hpp>
#include <Engine/RootObject.hpp>
#include <App/AppTypeDefs.h>

#include <App/AppCommon.h>
#include <Graphics/RenderUniforms.h>
#include <Misc/Q3BspTypes.h>
#include <Misc/Q3BSPShader.h>
#include <Misc/Q3Entities.h>

namespace App
{
	class EngineContext;
	class Camera;	
	class IView;
}

namespace Render
{
	class VertexArrayObject;
}

namespace Misc
{
	struct Q3DrawInfo;
	struct Q3DrawCluster;	
	
	using PortalView	  = std::shared_ptr<App::IView>;
	using DrawList		  = std::vector<Q3DrawInfo>;	
	using DrawListPtr	  = std::vector<Q3DrawInfo*>;
	using EntityList	  = std::vector<Q3EntityPtr>;	
	using Q3ShaderList    = std::vector<Q3ShaderPtr>;
	using ShaderCache	  = std::map<String, Q3ShaderPtr>;
	using ClusterCache	  = std::map<int, Q3DrawCluster >;

	/*
		@brief: Quake III Leaf node
	*/
	struct Q3DrawLeaf
	{
		
		Q3DrawLeaf();

		explicit Q3DrawLeaf(const Q3BspLeaf& leafNode, int leafId );

		explicit Q3DrawLeaf( int leaf, int cluster, int firstBrush, 
							 int numBrushes, int firstFace, int numFaces );

		~Q3DrawLeaf();	
		
		int						m_leafId;
		int						m_cluster;

		int						m_firstBrush;
		int						m_numBrushes;

		int						m_firstFace;
		int						m_numFaces;
		bool					m_hasSky;

		Math::BBox3f			m_bounds;		
		VaoBuffer				m_vaoBuffer;
		DrawList				m_drawInfoList;
		VertexVector				m_vertexList;	

	private:	

	};






	/*
		@brief: Drawable sky information
	*/
	struct Q3DrawSky
	{
		Q3DrawSky(App::EngineContext* context, Q3ShaderPtr& shader,
			float width = 256, float eyeHeight = 0);

		virtual bool					createSkyBox();
		virtual void					draw();
		
		float							m_skyBoxWidth;
		float							m_eyeHeight;

		App::EngineContext*				m_context;
		Q3ShaderPtr						m_shader;
		VaoBuffer						m_vaoBuffer;
	};	

	/*
	@brief: Mirror/Portal surface
	*/
	class Q3DrawPortal : public Q3Entity
	{
	public:
		Q3DrawPortal( ContextPointer context );
		
	
		virtual bool						updatePortal( ViewPortPtr activeView );

		PortalView							getPortalView() const;
		VaoBuffer							getVAOBuffer() const;
		bool								isMirror() const;
	


		int									m_originalFace;			
		TriangleList						m_portalTriangles;
		Math::Planef						m_portalPlane;			//reflection plane for mirrored surfaces		
		Q3EntityPtr							m_portalCamera;			//the end-point of this portal
		mutable VaoBuffer					m_vaoBuffer;			//vao for this portal
		mutable ViewPortPtr					m_portalView;
		
	};

	/*
		@brief: Draw Cluster, contains all geometry potentially visilble for a given eye
	*/
	struct Q3DrawCluster
	{
		
		Q3DrawCluster() = default;

		bool				containsLeaf(int leafIdx) const;
		
		Math::BBox3f		m_clusterBounds;
		EntityList			m_staticEntities;
		IntVector			m_visibleClusters;
		IntVector			m_visibleLeafs;

	};
		
	/*
		@brief: Main Quake III map file
	*/
	class Q3BspFile : public App::RootObject
	{
	
	
	public:
		using DrawSkyPtr	= std::shared_ptr<Q3DrawSky>;		
		using DrawLeafNodes = std::vector<Q3DrawLeaf>;
		

		explicit Q3BspFile				( App::EngineContext* context );
		virtual ~Q3BspFile();


		bool							isLoaded() const;
		String							getMapName() const { return m_fileName; }

		/*
		*@brief: Opens a Quake III file 
		*/
		bool							loadFile(const String& mapName);
		static bool						MapExist( ContextPointer context, const String& mapName);
		
		/*
			@brief: places camera at one of the start locations
		*/
		//void							spawn();


		ClientStartingPos				getRandomStartLocation() const;


		


		/*
		* @brief: clears an opened file
		*/
		bool							clear();	
		
		/*
		* @brief: Draw map
		*/
		void							draw();		

		/*
		* @brief: Build triangle list from faces& patches
		*/
		TriangleList					triangulateFace(int faceId) const;
					
		/*
		 @brief: Return (all) surface faces that matches a surface flags
		*/
		IntVector						getFacesOfType( std::uint32_t surfaceFlags );
		
		/*
			@brief: Returns all entities from a specific type
		*/
		template<class T> 
		std::vector<std::shared_ptr<T>>	getEntitiesByType() const;

		/*
		* @brief: Get entities by name (TODO optimize --> SLOW )
		*/
		EntityList						getEntitiesByName( const String& name, bool getAll = false) const;

		/*
			@brief: Returns all the static entities for this cluster
		*/
		EntityList						getEntitiesForCluster(int clusterId) const;	
		
		/*
			@brief: Returns a shader by index
		*/
		Q3ShaderPtr						getShader(int id) const;
		
		/*
			@brief: Iterative method for traversing a bsp tree
		*/
		int								leafNodeForPosition(const Math::Vector3f& position) const;

		/*
			@brief : Directly returns the cluster for a position
		*/
		int								clusterForPosition(const Math::Vector3f& position) const;


		/*
			@brief: Extract pvs for current input clusters
		*/
		bool							clusterVisible(int inputCluster, int clusterToTest) const;


		IntVector						getFacesForCluster(const Q3DrawCluster& input, bool noDuplicates = true);


		ViewPortVector					getPortalViews(const ViewPortPtr& activeView);
		
		/*
			Returns all the visible leafs for this cluster
		*/
		Q3DrawCluster&					getVisibleClusters(int clusterId);

				
		/*
		*  @brief: Returns a description/stats of this map
		*/
		String							toString() const;


		Math::BBox3f					m_worldBounds;
		DrawLeafNodes					m_drawLeafs;	//all the leaf nodes that can be drawn	
		EntityList						m_entityList;	//entities found for this map
		Q3ShaderList					m_shaders;		//active & compiled shaders
		DrawSkyPtr						m_skyBox;		//skybox if any
		ShaderCache						m_shaderLUT;	//shader cache todo make this global?		
		TexturePtr						m_lightmap;		//single lightmap texture
		PlaneVector						m_planeList;
		ClusterCache					m_clusterList;				

		String							m_fileName;
		Q3Header						m_fileHeader;
		
		int								m_numVisClusters;
		int								m_bytesPerVisCluster;

		std::vector<std::uint8_t>				m_visData;
		std::vector<std::uint8_t>				m_entityString;		
		std::vector<Q3ShaderInfo>		m_textureList;
		std::vector<Q3BspNode>			m_nodeList;		
		std::vector<Q3LeafFace>			m_leafFaceList;
		std::vector<Q3LeafBrush>		m_leafBrushList;
		std::vector<Q3Model>			m_modelList;
		std::vector<Q3Brush>			m_brushList;
		std::vector<Q3BrushSide>		m_brushSidesList;
		std::vector<Q3MeshVertices>		m_meshVertexList;
		std::vector<Q3Effect>			m_effectList;
		std::vector<Q3Face>				m_faceList;		
		std::vector<Q3LightVolume>		m_lightVolumeList;
		std::vector<Vertex>				m_vertexList;		
		std::vector<Q3Triangle>			m_faceTriangles;
	
	private:

		///*
		//Returns all the visible leafs for this cluster
		//*/
		//const Q3DrawCluster&			getVisibleClusters(int clusterId) const {
		//	return getVisibleClusters(clusterId);
		//}
		//
		/*
			@brief: link all entities e.g. connect entity target with other entity's targetField
		*/
		void							linkEntities();

		/*
			@brief: build draw clusters
		*/
		void							buildDrawClusters();

		/*
		@brief: Build a vao-buffer for input leafs, also tries to merge shaders/triangles
		into a bigger group.
		*/
		void							buildVAOForLeafs();	
	
		/*
		 * @Update lightmap coordinates
		 */
		void							adjustLightmapCoords( TriangleList& triangles ) const;
		/*
		 * @brief: Triangulate poly face
		 */
		TriangleList					parsePolyFace( const Q3Face& face, int origFaceIdx ) const;
		/*
		* @brief: Parse curved face
		*/
		TriangleList					parsePatchFace( const Q3Face& face, int origFaceIdx) const;
		/*
		* @brief: Parse model face
		*/
		TriangleList					parseMeshFace(const Q3Face& face, int origFaceIdx) const;		
		/*
		* @brief: Parse billboard
		*/
		TriangleList					parseBillBoardFace(const Q3Face& face, int origFaceIdx) const;

		/*
		*	@brief: Offset map so that it's min bounds are at the origin(0,0,0)
		*/
		void							offsetMap( const Math::Vector3f& offset );
		
		/*
		*	@brief: parse Quake III lumps, returns false upon failure
		*/
		bool							parseLumps( const String& fileName );
		
		/*
			@brief: Calculate world bounds
		*/
		void							calculateBounds();
		
		/*
		* @brief: Parse the entity string
		*/
		bool							parseEntityString();

		/*
		* @brief: Cache shader directory e.g. loads all shaders, but doesn't load textures
		* or generate GLSL code
		*/
		bool							parseShaderDirectory();			

		/*
		 * @brief: Load&compile shaders found in this map, return
		 * the succesfully loaded shadrs
		 */
		bool							loadShaders();					

		/*
		 * @brief: Load the lightmaps from this map file & combine them 
		 * into single texture
		 */
		bool							loadLightMaps( const std::vector<Q3LightMap>& lightmaps );		


		bool							m_loaded;		
		mutable int						m_numPolyFaces;
		mutable int						m_numPatches;
		mutable int						m_numMeshFaces;
		mutable int						m_numBillBoards;

		mutable std::vector<Q3Patch>	m_facePatches;
		
	};

	//slow but steady
	template<class T>
	std::vector<std::shared_ptr<T>> Q3BspFile::getEntitiesByType() const
	{
		std::vector<std::shared_ptr<T>> result;
		for (auto& ent : m_entityList)
			if ( auto entCast = std::dynamic_pointer_cast<T>(ent))
				result.push_back(entCast);
		return result;
	}

}
