#pragma once

#include <vector>
#include <map>
#include <Math/AABB.h>
#include <App/AppTypeDefs.h>
#include <IO/FileSystem.hpp>
#include <Graphics/MeshVertex.h>

namespace Misc
{

	struct Q3Triangle;
	struct Q3Face;


	const int			ENTITY_BITS			= 17;
	const int			MAX_ENTITIES		= 1 << ENTITY_BITS;
	const int			MAX_PORTAL_SURFACES = 8;	//max portal surfaces per map
	const int				PORTAL_FACE_ID		= -666;
	
	
   
    using StringList	= std::vector<String>;

	using TriangleList			= std::vector<Q3Triangle>;
	using TriangleListPointer	= std::vector<Q3Triangle*>;
	using FaceListPointer		= std::vector<Q3Face*>;
	
    const String BASE_PATH			= "q3a/";
    const String MUSIC_PATH			= "music/";
    const String SHADER_PATH		= "scripts/";
    const String MAP_PATH			= "maps/";
    const String MODEL_PATH			= "models/";
    const String FALLBACK_SHADER	= "FallbackShader"; 
    
    inline String Q3BasePath()
    {
        return App::getDataPath() + BASE_PATH;
    }

    inline String Q3GetShaderPath()
    {
        return App::getDataPath() + BASE_PATH + SHADER_PATH;
    }

    inline String Q3GetMapPath()
    {
        return App::getDataPath() + BASE_PATH + MAP_PATH;
    }

    inline String Q3ModelPath()
    {
        return App::getDataPath() + BASE_PATH + MODEL_PATH;
    }


    enum class eQ3WaveFunc
    {
        NONE,
        SIN,
        TRIANGLE,
        SQUARE,
        SAWTOOTH,
        INV_SAWTOOTH,
        NOISE
    };

    enum class eQ3VertexDeformFunc
    {
        NONE				= 0x0,
        VD_AUTOSPRITE		= 0x01,
        VD_AUTOSPRITE2		= 0x02,
        VD_PROJECT_SHADOW	= 0x04,
        VD_WAVE				= 0x08,
        VD_MOVE				= 0x10, 
		VD_BULGE			= 0x20,
        VD_NORMAL			= 0x40,
        VD_TEXT0			= 0x80,
        VD_TEXT1			= 0x100
    };

    enum class eQ3AlphaFunc
    {
        NONE,
        GREATER_THAN0,
        LESS_THAN128,
        GEQUALS_THAN128
    };

    enum class eQTcGen
    {
        BASE,
        VECTOR,
        ENVIRONMENT,
        LIGHTMAP
    };

    enum class eQ3RgbGen
    {
        NONE,
        WAVE,
        IDENTITY,
        VERTEX,
        EXACTVERTEX,
        LIGHTNING_DIFFUSE,
        ALPHA_LIGHTING_SPEC,
        ALPHA_PORTAL,
        ENTITY
    };

    enum class eQ3TcMod
    {
        NONE,
        SCROLL,
        SCALE,
        TURB,
        TRANSFORM,
        STRETCH,
        ROTATE
    };

    enum eQ3SurfaceParam
    {
        SURFACE_NONE = 0x0,
        SURFACE_NODAMAGE = 0x1,// never give falling damage
        SURFACE_SLICK = 0x2,// effects game physics
        SURFACE_SKY = 0x4,// lighting from environment map
        SURFACE_LADDER = 0x8,
        SURFACE_NOIMPACT = 0x10,// don't make missile explosions
        SURFACE_NOMARKS = 0x20,// don't leave missile marks
        SURFACE_FLESH = 0x40,// make flesh sounds and effects
        SURFACE_NODRAW = 0x80,// don't generate a draws surface at all
        SURFACE_HINT = 0x100,// make a primary bsp splitter
        SURFACE_SKIP = 0x200,// completely ignore, allowing non-closed brushes
        SURFACE_NOLIGHTMAP = 0x400,// surface doesn't need a light map
        SURFACE_POINTLIGHT = 0x800,// generate lighting info at vertexes
        SURFACE_METALSTEPS = 0x1000,// clanking footsteps
        SURFACE_NOSTEPS = 0x2000,// no footstep sounds
        SURFACE_NONSOLID = 0x4000,// don't collide against curves with this set
        SURFACE_LIGHTFILTER = 0x8000,// act as a light filter during q3map -light
        SURFACE_ALPHASHADOW = 0x10000,// do per-pixel light shadow casting in q3map
        SURFACE_NODLIGHT = 0x20000,// don't d-light even if solid (solid lava, skies)
        SURFACE_DUST = 0x40000,// leave a dust trail when walking on this surface
        SURFACE_PORTAL = 0x80000
    };

    enum eQ3SurfaceContents
    {
        CONTENT_NONE = 0x00,
        CONTENT_SOLID = 0x01,		// an eye is never valid in a solid
        CONTENT_LAVA = 0x08,
        CONTENT_SLIME = 0x10,
        CONTENT_WATER = 0x20,
        CONTENT_FOG = 0x40,
        CONTENT_NOTTEAM1 = 0x0080,
        CONTENT_NOTTEAM2 = 0x0100,
        CONTENT_NOBOTCLIP = 0x0200,
        CONTENT_AREAPORTAL = 0x8000,
        CONTENT_PLAYERCLIP = 0x10000,
        CONTENT_MONSTERCLIP = 0x20000,
        CONTENT_TELEPORTER = 0x40000,
        CONTENT_JUMPPAD = 0x80000,
        CONTENT_CLUSTERPORTAL = 0x100000,
        CONTENT_DONOTENTER = 0x200000,
        CONTENT_BOTCLIP = 0x400000,
        CONTENT_MOVER = 0x800000,
        CONTENT_ORIGIN = 0x1000000,// removed before bsping an entity		
        CONTENT_BODY = 0x2000000,	// should never be on a brush, only in game
        CONTENT_CORPSE = 0x4000000,
        CONTENT_DETAIL = 0x8000000,// brushes not used for the bsp
        CONTENT_STRUCTURAL = 0x10000000,// brushes used for the bsp
        CONTENT_TRANSLUCENT = 0x20000000,// don't consume surface fragments inside
        CONTENT_TRIGGER = 0x40000000,
        CONTENT_NODROP = 0x80000000	// don't leave bodies or items (death fog, lava)
    };

    enum eFaceType
    {
        POLYFACE = 1,
        PATCHFACE = 2,
        MESHFACE = 3,
        BILLBOARD = 4
    };

    enum eFileLump
    {
        ENTITY_LUMP = 0,
        TEXTURE_LUMP = 1,
        PLANE_LUMP = 2,
        NODE_LUMP = 3,
        LEAF_LUMP = 4,
        LEAF_FACES_LUMP = 5,
        LEAF_BRUSHES_LUMP = 6,
        MODELS_LUMP = 7,
        BRUSHES_LUMP = 8,
        BRUSH_SIDES_LUMP = 9,
        VERTEX_LUMP = 10,
        MESH_VERTEX_LUMP = 11,
        EFFECTS_LUMP = 12,
        FACES_LUMP = 13,
        LIGHTMAP_LUMP = 14,
        LIGHTVOLS_LUMP = 15,
        VIS_DATA_LUMP = 16
    };

	enum eShaderSort
	{
		SORT_PORTAL = 1,
		SORT_SKY	= 2,
		SORT_OPAQUE = 3,
		SORT_BANNER = 6,
		SORT_UNDERWATER = 8,
		SORT_ADDITIVE  = 9,
		SORT_NEAREST = 16
	};

	enum eImageFlags
	{
		FLAGS_NONE = 0x00,
		FLAGS_ADD_ALPHA = 0x01
	};


    struct SurfaceFlag
    {
        String			m_name; //lookup name in shader
        std::uint32_t			m_flag;
    };


    struct Q3LumpInfo
    {
        int				m_Offset;
        int				m_Length;
    };

    struct Q3Header
    {
        std::uint8_t			m_MagicWord[4];
        int				m_Version;
        Q3LumpInfo		m_DirEntries[17];

        void			clear();
        bool			valid() const;
    };


	/*
	*@brief: Native quake 3 vertex as found in a bsp file
	*/
	struct Q3Vertex
	{
		Math::Vector3f	m_pos;
		Math::Vector2f	m_texCoord1;
		Math::Vector2f	m_texCoord2;
		Math::Vector3f	m_normal;
		Math::Vector4ub	m_rgba;
	};

    struct Q3Triangle
    {
        Q3Triangle();
        Q3Triangle(const Math::Vector3f& normal, int faceId, int shaderId, int lightId);

        Math::BBox3f	getBounds() const;
        float			area() const;
        void			calcNormal();

        Vertex			m_vertices[3];
        Math::Vector3f	m_normal;
		int				m_faceid;		//originalface id
        int				m_shaderId;		
        int				m_lightmapId;
		int				m_faceType;
    };


    struct Q3ShaderInfo
    {
        std::uint8_t		m_texName[64];
        int			m_TexFlags;
        int			m_TexContents;
    };

    struct Q3Plane
    {
        float		m_normal[3];
        float		m_distance;
    };

    //bsp node
    struct Q3BspNode
    {
        int			m_splitPlane;
        int			m_children[2];
        int			m_min[3];
        int			m_max[3];
    };

	struct Q3Model
	{
		float		m_min[3];
		float		m_max[3];
		int			m_firstFace;
		int			m_numFaces;
		int			m_firstBrush;
		int			m_numBrushes;
	};


    //bsp leaf
    struct Q3BspLeaf
    {
        int			m_visCluster;
        int			m_areaPortal;
        int			m_min[3];
        int			m_max[3];
        int			m_firstLeafFace;
        int			m_numLeafFaces;
        int			m_firstLeafBrush;
        int			m_numLeafBrushes;
    };

    struct Q3LeafFace
    {
        int			m_faceIndex;
    };

    struct Q3LeafBrush
    {
        int			m_leafBrushIndex;
    };

    struct Q3Brush
    {
        int			m_brushIndex;
        int			m_numBrushes;
        int			m_texIndex;
    };

    struct Q3BrushSide
    {
        int			m_planeIndex;
        int			m_texIndex;
    };

    struct Q3MeshVertices
    {
        int			m_vertexOffset;
    };

    struct Q3Effect
    {
        std::uint8_t		m_name[64];
        int			m_brush;
        int			m_unknown;
    };

    struct Q3Face
    {
        int			m_texIndex;
        int			m_effect;
        int			m_faceType;
        int			m_vertex;
        int			m_numVerts;
        int			m_meshVert;
        int			m_numMeshVerts;
        int			m_lightmap;
        int			m_lightmapStart[2];
        int			m_lightmapSize[2];
        float		m_lightmapOrigin[3];
        float		m_lightmapStCoords[2][3];
        float		m_normal[3];
        int			m_patchSize[2];
    };

	/*
		@brief: Quake 3 lightmap
	*/
    struct Q3LightMap
    {
			
		std::uint8_t		m_lightmapData[128][128][3];

		void		rescale( float factor );
    };

    struct Q3LightVolume
    {
        std::uint8_t		m_rgb[3];
        std::uint8_t		m_rgbDirectional[3];
        std::uint8_t		m_direction;
    };


    //////////////////////////////////////////////////////////////////////////
    //\Q3BspMesh 
    //////////////////////////////////////////////////////////////////////////
    struct Q3Mesh
    {
        int								m_shaderId;
        int								m_lightmapId;
        int								m_width,
                                        m_height;
        std::vector<Q3Triangle>			m_faceTriangles;
        Math::BBox3f					m_bounds;
    };


    //////////////////////////////////////////////////////////////////////////
    //\Q3BspBiQuadPatch
    //////////////////////////////////////////////////////////////////////////
    struct Q3BiQuadPatch
    {
        const static int TESS_LEVEL = 7;

		TriangleList				tesselate(const Q3Face& face, int faceId, int level = TESS_LEVEL);

        App::MeshVertex				m_controls[9];
      //  std::vector<Q3Triangle>		m_triangles;
    };


    //////////////////////////////////////////////////////////////////////////
    //\Q3BspPatch 
    //////////////////////////////////////////////////////////////////////////
    struct Q3Patch
    {
        void							calcBounds();
		void							smoothPatchNormals();
        int								m_shaderId;
        int								m_lightmapId;
        int								m_width, m_height;
        Math::BBox3f					m_bounds;
        std::vector<Q3BiQuadPatch>		m_patches;
		TriangleList					m_triangles;
    };
}//namespace


