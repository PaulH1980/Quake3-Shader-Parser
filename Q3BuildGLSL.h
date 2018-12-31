#pragma once

#include <string>
namespace Misc
{
   // const int LIGHTMAP_ID = 7;

	const std::string SamplerNames[8] = {
		"tex_00", "tex_01", "tex_02", "tex_03",
		"tex_04", "tex_05", "tex_06", "tex_07"
	};



    const std::string q3ShaderGlobal =

        "#version 440                                            \n"
        "layout(std430, binding = 1)  buffer GlobalBuffer        \n"
        "{                                                       \n"
        "mat4            m_wvpMatrix;                            \n"
        "mat4            m_wvpMatrixInverse;                     \n"
        "mat4            m_wvpMatrixPrevious;                    \n"
        "mat4            m_projMatrix;                           \n"
        "mat4            m_viewMatrix;                           \n"
        "mat4            m_normalMatrix;                         \n"
        "mat4            m_orthoMatrix2d;                        \n"
		"mat4            m_skyMatrix;						     \n"
        "vec4            m_cameraPos;                            \n"
        "vec4            m_ambient;                              \n"
        "vec4            m_sunPos;                               \n"
        "vec2            m_frameBufferDims;                      \n"
		"vec2            m_invFrameBufferDims;                   \n"
		"vec2		     m_frameBufferStart;					 \n"
		"vec2            m_nearFar;                              \n"
	    "float           m_rootSize;                             \n"
        "float           m_pixScale;                             \n"
        "float           m_frameTime;                            \n"
        "float           m_programTime;                          \n"
        "};                                                      \n"
		"                                                        \n"
		"vec2 sign_not_zero(vec2 v) {							 \n"
		"	  return fma( step( vec2(0.0), v),					 \n"
		"				vec2(2.0), vec2(-1.0));					 \n"
		"	}													 \n"
		"														 \n"
		" vec3 unpack_normal_octahedron(vec2 packed_nrm)		 \n"
		" {														 \n"
		" 	vec2 norm = packed_nrm * 2.0 - 1.0;					 \n"
		" 	vec3 v = vec3(norm.xy, 1.0 - abs(norm.x) - abs(norm.y));		 \n"
		" 	if (v.z < 0) v.xy = (1.0 - abs(v.yx)) * sign_not_zero(v.xy);	 \n"
		" 	return normalize(v);								 \n"
		"}														 \n"
		"                                                        \n"
		"                                                        \n"
		"                                                        \n"
        "const float PI = 3.14159265359;                         \n";

   
	const std::string vertDefault =

		"                                                        \n"
		"layout(location = 0) in vec3        vertIn;             \n"
		"layout(location = 1) in vec4        rgbaIn;             \n"
		"layout(location = 2) in vec2        stIn;               \n" //normal tc
		"layout(location = 3) in vec2        uvIn;               \n" //lightmap tc
		"layout(location = 4) in vec4        normalIn;           \n"
		"                                                        \n"
		"out vec4        world;                                  \n"
		"out vec4        rgba;                                   \n"
		"out vec2        st;                                     \n"
		"out vec2        uv;                                     \n"
		"out vec4        normal;                                 \n"
		"                                                        \n"
		"uniform int  useSkyBox		= 0;					     \n"
		"uniform mat4 skyBoxMatrix;                              \n"
		"uniform float asWidth;                                  \n"
		"uniform float asHeight;                                 \n"
		"uniform vec3 asMajorDir;	                             \n"
		"uniform vec3 asCenter;		                             \n"
		"uniform int  asStartIdx;						         \n"
		"                                                        \n"
		"                                                        \n"
		"layout( std140) uniform Quake3Param                     \n"
		"{                                                       \n"
		"   mat4	m_skyBoxMatrix;								 \n"
		"   vec3	m_asMajorDir;								 \n"
		"   float	m_asWidth;									 \n"
		"   vec3	m_asCenter;									 \n"
		"   float	m_asHeight;									 \n"
		"   int		m_asStartIdx;								 \n"
		"   int		m_useSkyBox;							     \n"
		"                                                        \n"
		"                                                        \n"
		"};                                                      \n"
		"                                                        \n"
		"const int  quadIndices[6] = int[6]( 0, 2, 3, 1, 2, 0 ); \n"
		"                                                        \n"
		"const vec2 quadDirs[4] = vec2[4](						 \n"
		"	vec2( -1.0, -1.0 ), vec2(  1.0, -1.0 ),              \n"
		"   vec2(  1.0,  1.0 ), vec2( -1.0,  1.0 ) );            \n"
		"                                                        \n"
		"                                                        \n"
		"                                                        \n";

	const std::string vertMain =
		"void main()													\n"
		"{																\n"
		"    mat4 worldToClip = useSkyBox == 1 ? m_skyMatrix : m_wvpMatrix; \n"
		"    vec3 normalTemp =   unpack_normal_octahedron(normalIn.xy); \n"		
		"    rgba		   = rgbaIn;                             \n"
		"    st			   = stIn;                               \n"
		"    uv			   = uvIn;                               \n"
		"    normal.xyz    = normalTemp;						 \n"
		"    world		   = getWorld( normalTemp );				 \n"
		"    gl_Position = worldToClip * world;					 \n"
        "}                                                       \n";



    //fragment shader default inputs
	const std::string fragDefault =
		"                                                        \n"
		"in vec4        world;                                   \n"
		"in vec4        rgba;                                    \n"
		"in vec2        st;                                      \n"
		"in vec2        uv;                                      \n"
		"in vec4        normal;                                  \n"
		"                                                        \n"
		"                                                        \n"
		"                                                        \n"
		"                                                        \n"
		"                                                        \n";

	const std::string fragGetNormal =
		"vec3 getSurfNormal(){                                   \n"
		"   return unpack_normal_octahedron( normal.xy );        \n"
		"};                                                      \n";

	

    const std::string fragMain =
        "                                                        \n"
        "layout(location = 0) out vec4 FragColor;                \n"
        "void main()                                             \n"
        "{                                                       \n"
        "    vec3 color = calcFragment().xyz * 2.0;              \n"
        "    FragColor = vec4( color, 1.0 );					 \n"
        "}                                                       \n";    


	const std::string fragMainAlpha =
		"                                                        \n"
		"layout(location = 0) out vec4 FragColor;                \n"
		"void main()                                             \n"
		"{                                                       \n"
		"    vec4 color = calcFragment();						 \n"
		"    FragColor = vec4( color.xyz, color.w );		 \n"
		"}                                                       \n";


}