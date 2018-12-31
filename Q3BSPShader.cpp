
#include <Common/Image.h>
#include <Common/Debug.h>

#include <Render/OpenGLIncludes.h>

#include <Engine/EngineContext.hpp>
#include <Resource/ResourceManager.hpp>
#include <Resource/ConcreteResources.hpp>
#include <ConsoleIncludes.h>
#include <App/AppCommon.h>
#include <Misc/Q3BuildGLSL.h>
#include <Misc/Q3BSPShader.h>

namespace Misc
{
 	const String GlobalKeyWordsToSkip[] =
	{
		"q3map",
		"qer",
		"//"
	};

	const String ShaderKeyWordsToSkip[] =
	{
		"cloudparms",
		"light1",
		"lightning",
		"entitymergable",
		"foggen",
		"backsided",
		"alphaMap"
	};

	const String StageKeyWordsToSkip[] =
	{
		"alphamap"
	};

	StringList ShaderDirectives =
	{
		"skyparms", "cull", "deformvertexes", "fogparms", "nopicmip",
		"nomipmaps", "polygonoffset", "portal", "sort", "surfaceparm",
		"light", "tesssize", "sky", "fogonly"
	};

	StringList StageDirectives =
	{
		"map", "clampmap", "animmap", "videomap",
		"blendfunc", "rgbgen", "alphagen",
		"tcgen", "tcmod", "depthfunc", "depthwrite",
		"detail", "alphafunc"
	};


	const String comment("//");
	const String vec2("vec2 ");
	const String vec3("vec3 ");
	const String vec4("vec4 ");
	const String texture("texture");
	const String funcStart("( ");
	const String comma(" , ");
	const String semicol(";");
	const String funcEnd(")");
	const String tab("    ");
	const String doubleTab("        ");
	const String assign(" = ");
	const String assignAdd(" += ");
	const String add(" + ");
	const String mul(" * ");
	const String assignMul(" *= ");
	const String sub(" - ");
	const String div(" / ");
	const String endLn("\n");
	const String arrStart("[");
	const String arrEnd("]");
	const String dot(".");
	const String space(" ");
	const String openBrack("{");
	const String closeBrack("}");

	const SurfaceFlag SurfaceContents[] =
	{
		{ "none"			, 0x00 },
		{ "solid"			, 0x01 },		// an eye is never valid in a solid
		{ "lava"			, 0x08 },
		{ "slime"			, 0x10 },
		{ "water"			, 0x20 },
		{ "fog"				, 0x40 },
		{ "notteam1"		, 0x0080 },
		{ "notteam2"		, 0x0100 },
		{ "nobotclip"		, 0x0200 },
		{ "areaportal"		, 0x8000 },
		{ "playerclip"		, 0x10000 },
		{ "monsterclip"		, 0x20000 },
		{ "teleporter"		, 0x40000 },
		{ "jumppad"			, 0x80000 },
		{ "clusterportal"	, 0x100000 },
		{ "donotenter"		, 0x200000 },
		{ "botclip"			, 0x400000 },
		{ "mover"			, 0x800000 },
		{ "origin"			, 0x1000000 },	// removed before bsping an entity		
		{ "body"			, 0x2000000 },	// should never be on a brush, only in game
		{ "corpse"			, 0x4000000 },
		{ "detail"			, 0x8000000 },	// brushes not used for the bsp
		{ "structural"		, 0x10000000 },	// brushes used for the bsp
		{ "translucent"		, 0x20000000 },	// don't consume surface fragments inside
		{ "trigger"			, 0x40000000 },
		{ "nodrop"			, 0x80000000 },	// don't leave bodies or items (death fog, lava)
		{ "trans"			, 0x20000000 }
	};

	const SurfaceFlag SurfaceParams[] =
	{
		{ "none"		, 0x0 },
		{ "nodamage"	, 0x1 },
		{ "slick"		, 0x2 },
		{ "sky"			, 0x4 },
		{ "ladder"		, 0x8 },
		{ "noimpact"	, 0x10 },
		{ "nomarks"		, 0x20 },
		{ "flesh"		, 0x40 },
		{ "nodraw"		, 0x80 },
		{ "hint"		, 0x100 },
		{ "skip"		, 0x200 },
		{ "nolightmap"	, 0x400 },
		{ "pointlight"	, 0x800 },
		{ "metalsteps"	, 0x1000 },
		{ "nosteps"		, 0x2000 },
		{ "nonsolid"	, 0x4000 },
		{ "lightfilter"	, 0x8000 },
		{ "alphashadow"	, 0x10000 },
		{ "nodlight"	, 0x20000 },
		{ "dust"		, 0x40000 },
		{ "nomipmaps"	, 0x80000 }
	};

	const String ImageExtensions[6] =
	{
		".tga",
		".jpg",
		".jpeg",
		".png",
		".dxt",
		".bmp"
	};


	int IsShaderDirective(const String& token)
	{
		for (auto i = 0; i < ShaderDirectives.size(); ++i)
		{
			if (Common::StringEquals(token, ShaderDirectives[i], true))
				return i;
		}
		return INVALID_INDEX;
	}

	int IsStageDirective(const String& token)
	{
		for (auto i = 0; i < StageDirectives.size(); ++i)
		{
			if (Common::StringEquals(token, StageDirectives[i], true))
				return i;
		}
		return INVALID_INDEX;
	}

	bool IgnoreGlobalDirective(const String& key)
	{
		auto keyLower = Common::ToLower(key);
		for (const auto& skipStr : GlobalKeyWordsToSkip)
		{
			if (Common::StartsWidth(keyLower, skipStr))
				return true;
		}
		return false;
	}


	bool IgnoreStageDirective(const String& key)
	{
		for (const auto& skipStr : StageKeyWordsToSkip)
		{
			if (Common::StringEquals(skipStr, key, true))
				return true;
		}
		return false;
	}

	bool IgnoreShaderDirective(const String& key)
	{
		for (const auto& skipStr : ShaderKeyWordsToSkip)
		{
			if (Common::StringEquals(skipStr, key, true))
				return true;
		}
		return false;
	}


	bool SurfParamForToken(const String& token, std::uint32_t& surfFlag)
	{
		for (const auto& param : SurfaceParams)
		{
			if (param.m_name == token)
			{
				surfFlag |= param.m_flag;
				return true;
			}
		}
		return false;
	}

	bool ContentsForToken(const String& token, std::uint32_t& contents)
	{
		for (const auto& param : SurfaceContents)
		{
			if (param.m_name == token)
			{
				contents |= param.m_flag;
				return true;
			}
		}
		return false;
	}

	int TexturePathValid(const String& fileName)
	{
		auto pathToImg = Q3BasePath() + fileName;
		for (auto i = 0; i < 6; ++i)
		{
			auto imgPathExt = pathToImg + ImageExtensions[i];
			if (App::FileExists(imgPathExt))
				return i;
		}
		return INVALID_INDEX;
	}


	Q3ShaderPtr  CreateRegularShaderImp(App::EngineContext* context, const String& texturePath)
	{
		Q3ShaderPtr result = std::make_shared<Q3Shader>(context);

		auto idx = TexturePathValid(texturePath);
		if (idx == INVALID_INDEX) {
			App::AddConsoleMessage(context, "File path not found: " + texturePath, App::LOG_LEVEL_WARNING);
			return nullptr;
		}

		result->m_name = texturePath;
		result->m_path = texturePath + ImageExtensions[idx];

		//pass 1 light map
		Q3ShaderStage stage(context);
		stage.m_lightmap = true;
		stage.m_blendFunc[0] = GL_ONE;
		stage.m_blendFunc[1] = GL_ZERO;
		result->m_shaderStages.push_back(stage);

		//pass 2 albedo tex
		stage.m_lightmap = false;
		stage.m_blendFunc[0] = GL_DST_COLOR;
		stage.m_blendFunc[1] = GL_ZERO;
		stage.m_textures.push_back(texturePath);
		result->m_shaderStages.push_back(stage);

		return result;
	}





	Q3ShaderPtr CreateFallBackShader(App::EngineContext* context)
	{
		Q3ShaderPtr result = std::make_shared<Q3Shader>(context);
		result->m_name = FALLBACK_SHADER;
		result->m_path = "";
		result->m_loaded = true;

		SurfParamForToken("nolightmap", result->m_sufaceFlags);

		auto resMan = context->getSystem<App::ResourceManager>();

		Q3ShaderStage defStage(context);
		defStage.m_lightmap = false;
		defStage.m_textures.emplace_back("DefaultAlbedo");
		defStage.m_textureList.push_back(resMan->getResourceSafe<App::Texture>("DefaultAlbedo"));
		result->m_shaderStages.push_back(defStage);
		return result;
	}
	

	auto makeVec2 = [=](const auto& a, const auto& b) -> String
	{
		using namespace std;
		auto result = vec2 + funcStart + to_string(a) + comma +
										 to_string(b) + funcEnd;
		return result;
	};

	auto makeVec3 = [=](const auto& a, const auto& b, const auto& c) -> String
	{
		using namespace std;
		auto result = vec3 + funcStart + to_string(a) + comma +
										 to_string(b) + comma +
										 to_string(c) + funcEnd;
		return result;
	};

	auto makeVec4 = [=](const auto& a, const auto& b, const auto& c, const auto& d) -> String
	{
		using namespace std;
		auto result = vec4 + funcStart + to_string(a) + comma +
										 to_string(b) + comma +
										 to_string(c) + comma +
										 to_string(d) + funcEnd;
		return result;
	};

	auto makeWave3 = [=](eQ3WaveFunc func, String base, String phase, String amp, String freq) ->String
	{
		String result;
		if (func == eQ3WaveFunc::SIN)
		{
			result += "float  waveResult = " + base + " + sin( ( " + phase + " + m_programTime * " + freq + " ) * 6.2831 ) * " + amp + ";\n";
			return result;
		}

		result = "float val = " + base + " + ( " + phase + " + m_programTime * " + freq + " );\n";
		if (func == eQ3WaveFunc::SQUARE)
			result += "float waveResult = (( mod( floor( val * 2.0 ) + 1.0, 2.0 ) * 2.0 ) - 1.0) * " + amp + ";\n";
		else if (func == eQ3WaveFunc::TRIANGLE)
			result += "float waveResult = abs(2.0 * fract(val) - 1.0) * " + amp + ";\n";
		else if (func == eQ3WaveFunc::SAWTOOTH)
			result += "float waveResult = fract( val ) * " + amp + ";\n";
		else if (func == eQ3WaveFunc::INV_SAWTOOTH)
			result += "float waveResult =  (1.0 - fract( val ) ) * " + amp + ";\n";
		else if (func == eQ3WaveFunc::NOISE)
			result += String("float waveResult = noise1(val) * ") + amp + ";\n";
		else
			throw std::exception("Invalid wave function");
		return result;
	};

	auto makeWave = [=](const Q3WaveForm& wf) ->String
	{
		using namespace std;
		auto base = to_string(wf.m_base);
		auto phase = to_string(wf.m_phase);
		auto amp = to_string(wf.m_amp);
		auto freq = to_string(wf.m_freq);

		String result;
		if (wf.m_wavefunc == eQ3WaveFunc::SIN)
		{
			result += "float  waveResult = " + base + " + sin( ( " + phase + " + m_programTime * " + freq + " ) * 6.2831 ) * " + amp + ";\n";
			return result;
		}

		result = "float val = " + base + " + ( " + phase + " + m_programTime * " + freq + " );\n";
		if (wf.m_wavefunc == eQ3WaveFunc::SQUARE)
			result += "float waveResult = (( mod( floor( val * 2.0 ) + 1.0, 2.0 ) * 2.0 ) - 1.0) * " + amp + ";\n";
		else if (wf.m_wavefunc == eQ3WaveFunc::TRIANGLE)
			result += "float waveResult = abs(2.0 * fract(val) - 1.0) * " + amp + ";\n";
		else if (wf.m_wavefunc == eQ3WaveFunc::SAWTOOTH)
			result += "float waveResult = fract( val ) * " + amp + ";\n";
		else if (wf.m_wavefunc == eQ3WaveFunc::INV_SAWTOOTH)
			result += "float waveResult =  (1.0 - fract( val ) ) * " + amp + ";\n";
		else if (wf.m_wavefunc == eQ3WaveFunc::NOISE)
			result += String("float waveResult = noise1(val) * ") + amp + ";\n";
		else
			throw std::exception("Invalid wave function");
		return result;
	};


	String ApplyBlendFunc(const Q3ShaderStage &shadStage)
	{
		String result;

		const auto& blendFunc = shadStage.m_blendFunc;
		String srcEq, dstEq;
		switch (blendFunc[0])
		{
		case GL_ZERO:
			srcEq = "vec4( 0.0 )";
			break;
		case GL_ONE:
			srcEq = "vec4( src )";
			break;
		case GL_DST_COLOR:
			srcEq = "( dst * src )";
			break;
		case GL_ONE_MINUS_DST_COLOR:
			srcEq = "( (1.0 - dst) * src )";
			break;
		case GL_SRC_ALPHA:
			srcEq = "( src.a * src )";
			break;
		case GL_ONE_MINUS_SRC_ALPHA:
			srcEq = "( ( 1.0 - src.a ) * src )";
			break;
		case GL_DST_ALPHA:
			srcEq = "( dst.a * src )";
			break;
		case GL_ONE_MINUS_DST_ALPHA:
			srcEq = "( ( 1.0 - dst.a ) * src )";
			break;
		default:
			throw std::exception("Invalid src blend func");
		}
		switch (blendFunc[1])
		{
		case GL_ZERO:
			dstEq = "vec4( 0.0 )";
			break;
		case GL_ONE:
			dstEq = "vec4( dst )";
			break;
		case GL_SRC_COLOR:
			dstEq = "( src * dst )";
			break;
		case GL_ONE_MINUS_SRC_COLOR:
			dstEq = "( (1.0 - src) * dst )";
			break;
		case GL_SRC_ALPHA:
			dstEq = "( src.a * dst )";
			break;
		case GL_ONE_MINUS_SRC_ALPHA:
			dstEq = "( ( 1.0 - src.a ) * dst )";
			break;
		case GL_DST_ALPHA:
			dstEq = "( dst.a * dst )";
			break;
		case GL_ONE_MINUS_DST_ALPHA:
			dstEq = "( ( 1.0 - dst.a ) * dst )";
			break;
		default:
			throw std::exception("Invalid dst blend func");
		}
		result += tab + String("finalColor = (") + srcEq + String(" + ") + dstEq + String(");\n"); //apply blend equation
		result += tab + String("finalColor = clamp( finalColor, vec4(0.0), vec4(1.0) ) ") + String(";\n"); //clamp values
		return result;
	}



	String ApplyRGBAGen( const Q3ShaderStage &shadStage)
	{
		String result;		
		switch (shadStage.m_rgbaGen.m_alphaType)
		{
			case eQ3RgbGen::ALPHA_LIGHTING_SPEC:
				break;
			case eQ3RgbGen::ALPHA_PORTAL:
				result += tab + openBrack + endLn;
				result += tab + "float dist = distance( world.xyz, m_cameraPos.xyz );\n";
				result += tab + "src.a = min( 1.0, dist / 512.0);\n";
				result += tab + closeBrack + endLn;
				break;
			case eQ3RgbGen::EXACTVERTEX:
				//calcFragment += tab + String("src.a = rgba.a;\n");
				break;
			case eQ3RgbGen::VERTEX:
				result += tab + String("src.a *= rgba.a;\n");
				break;
			case eQ3RgbGen::NONE:
				result += doubleTab + String("src.a = 0.0;\n");
				break;
			case eQ3RgbGen::IDENTITY:
				//calcFragment += doubleTab + String("src.a *= 1.0;\n");
				break;
			case eQ3RgbGen::WAVE:
				result += tab + openBrack + endLn;
				result += comment + "alphagen wave" + endLn;
				result += doubleTab + makeWave(shadStage.m_rgbaGen.m_alphaWaveForm);
				result += doubleTab + String("src.a *= waveResult;\n;");
				result += tab + closeBrack + endLn;
				break;
		}

		switch (shadStage.m_rgbaGen.m_rgbType)
		{
			case eQ3RgbGen::EXACTVERTEX:
				result += tab + String("src.rgb = rgba.xyz;\n");
				break;
			case eQ3RgbGen::VERTEX:
				result += tab + String("src.rgb *= rgba.xyz;\n");
				break;
			case eQ3RgbGen::NONE:
				result += tab + String("src.rgb *= rgba.xyz;\n");
				break;
			case eQ3RgbGen::IDENTITY:
				//calcFragment += tab + String("src.rgba *= vec4(1.0);\n");
				break;
			case eQ3RgbGen::WAVE:
				result += tab + openBrack + endLn;
				result += comment + "rgbgen wave" + endLn;
				result += doubleTab + makeWave(shadStage.m_rgbaGen.m_rgbWaveForm);
				result += doubleTab + String("src.rgb *= waveResult;\n;");
				result += tab + closeBrack + endLn;
				break;
			default:
				break;
		}

		//alpha function
		switch (shadStage.m_alphaFunc)
		{
			case eQ3AlphaFunc::GEQUALS_THAN128:
				result += tab + String("if( src.a < 0.5 ) discard;\n");
				break;
			case  eQ3AlphaFunc::GREATER_THAN0:
				result += tab + String("if( src.a == 0.0 ) discard;\n");
				break;
			case eQ3AlphaFunc::LESS_THAN128:
				result += tab + String("if( src.a >= 0.5  ) discard;\n");
				break;
			case eQ3AlphaFunc::NONE:
				break;
			default:
				throw std::exception("Invalid alphafunc state");
		}
		return result;
	}

	String AddVertexDeform(const Q3Shader* shader )
	{
		using namespace std;
		String vertFunction;
		//apply vertex deform
		for (int i = 0; i < shader->m_vertexDeform.size(); ++i)
		{
			const auto& vDef = shader->m_vertexDeform[i];		
			String offName = String("deformOff") + to_string(i);
			vertFunction += tab + "{\n";
			switch (vDef.m_vertexDeform)
			{
			case eQ3VertexDeformFunc::VD_WAVE:
			{
				const auto& wave = vDef.m_waveForm;
				vertFunction += doubleTab + "float deformTmp = ( vertIn.x + vertIn.y + vertIn.z  ) * " + to_string(vDef.m_dvDiv) + ";\n";
				vertFunction += doubleTab + "float base   = " + to_string(wave.m_base) + ";\n";
				vertFunction += doubleTab + "float phase  = " + to_string(wave.m_phase) + " + deformTmp;\n";
				vertFunction += doubleTab + "float amp    = " + to_string(wave.m_amp) + ";\n";
				vertFunction += doubleTab + "float freq   = " + to_string(wave.m_freq) + ";\n";
				vertFunction += doubleTab + makeWave3( wave.m_wavefunc, "base", "phase", "amp", "freq ");
				vertFunction += doubleTab + "worldCalc  += (normal * waveResult);\n";
				break;
			}
			case eQ3VertexDeformFunc::VD_BULGE:
			{
				//const auto& bulge = vDef.m_dvBulge;
				float bulgeWidth  = vDef.m_dvBulge[0];
				float bulgeHeight = vDef.m_dvBulge[1];
				float bulgeSpeed  = vDef.m_dvBulge[2];
				vertFunction += doubleTab + "vec2 stCoord = stIn;\n";
				vertFunction += doubleTab + "float width = stCoord[0] * " + to_string(bulgeWidth) + ";\n";
				vertFunction += doubleTab + "float speed =  m_programTime * " + to_string(bulgeSpeed) + ";\n";
				vertFunction += doubleTab + "float offset = sin( speed + width ) * " + to_string(bulgeHeight) + ";\n";
				vertFunction += doubleTab + "worldCalc  += (normal * offset);\n";
				break;
			}
			case eQ3VertexDeformFunc::VD_MOVE:
			{
				const auto& wave = vDef.m_waveForm;
				vertFunction += doubleTab + "vec3 moveVec = " + makeVec3(vDef.m_dvMove[0], vDef.m_dvMove[1], vDef.m_dvMove[2]) + ";\n";
				vertFunction += doubleTab + makeWave( wave );
				vertFunction += doubleTab + "worldCalc  += (moveVec * waveResult);\n";
				break;
			}

			case eQ3VertexDeformFunc::VD_AUTOSPRITE:
			{
				vertFunction += doubleTab + "int vIndex		= gl_VertexID - asStartIdx;\n";
				vertFunction += doubleTab + "int quadId		= quadIndices[vIndex];\n";
				vertFunction += doubleTab + "vec3 vecRight	= m_viewMatrix[0].xyz;\n";
				vertFunction += doubleTab + "vec3 vecUp		= m_viewMatrix[1].xyz;\n";

				vertFunction += doubleTab + "vec3 xOff		= vecRight  * quadDirs[quadId].x;\n";
				vertFunction += doubleTab + "vec3 yOff		= vecUp		* quadDirs[quadId].y;\n";
				vertFunction += doubleTab + "worldCalc  += normalize(xOff + yOff) * asWidth;\n";
				break;
			}
			case eQ3VertexDeformFunc::VD_AUTOSPRITE2:
			{
				vertFunction += doubleTab + "int vIndex			= gl_VertexID - asStartIdx;							\n";
				vertFunction += doubleTab + "int quadId			= quadIndices[vIndex];								\n";
				vertFunction += doubleTab + "vec3 vecForward	= m_viewMatrix[2].xyz;								\n";
				vertFunction += doubleTab + "vec3 asDirSideNorm	= normalize( cross( asMajorDir,  vecForward ));		\n";
				vertFunction += doubleTab + "vec3 asDirUp		= (asMajorDir    * asHeight) * quadDirs[quadId].y;	\n";
				vertFunction += doubleTab + "vec3 asDirRight	= (asDirSideNorm * asWidth) *  quadDirs[quadId].x;	\n";
				vertFunction += doubleTab + "worldCalc  += asDirUp + asDirRight;									\n";
				break;
			}
			default:
				break;
			}
			vertFunction += tab + "}\n";
		}
		return vertFunction;
	}

	bool	BuildOpenGLShader(const Q3Shader* shader, String& vertProgram, String& fragProgram)
	{
		if (shader->m_shaderStages.empty())
			return false;

		using namespace std;
		using namespace Math;		
		

		vertProgram += comment + shader->m_name + "\n\n";
		fragProgram += comment + shader->m_name + "\n\n";;
		vertProgram += q3ShaderGlobal;
		fragProgram += q3ShaderGlobal;

		bool print = false;


		{ //Build vertex shader
			
			String vertFunction = vec4 + "getWorld( vec3 normal )" + openBrack + endLn ; //vec4 getWorld() { 
			vertFunction += tab + "vec3 worldCalc = vertIn;\n";
			vertFunction += AddVertexDeform(shader );

			vertFunction += tab + "return vec4(worldCalc, 1.0);\n";
			vertFunction += closeBrack + endLn;			
					
			vertProgram += vertDefault;
			vertProgram += vertFunction;			
			//add main
			vertProgram += vertMain;

		}

		{ //Build fragment program		
		

			const auto& shadStages = shader->m_shaderStages;
			auto numStages = shadStages.size();

			fragProgram += fragDefault;
			fragProgram += fragGetNormal;

			auto hasAlphaFunc = INVALID_INDEX;
			auto hasLightMap = INVALID_INDEX;

			StringList uvSets;
			StringList uvNames;
			StringList texNames;
			StringList textureNames;


			for (auto i = 0; i < shadStages.size(); ++i) 
			{

				if (shadStages[i].m_alphaFunc != eQ3AlphaFunc::NONE)
					hasAlphaFunc = i;

				if (shadStages[i].m_lightmap)
					hasLightMap = i;

				auto uvSet		 = shadStages[i].m_lightmap ? String("uv") : String("st");
				auto uvName		 = String("texCoord0")	+ to_string(i);
				auto texName	 = String("texColor0")	+ to_string(i);
				auto textureName = String("tex_0")		+ to_string(i);

				uvSets.push_back(uvSet);
				uvNames.push_back(uvName);
				texNames.push_back(texName);
				textureNames.push_back(textureName);
				fragProgram += "uniform sampler2D tex_0" + to_string(i) + ";\n";
			}
			{
				String calcFragment =
					"\nvec4 calcFragment(){														\n";
				calcFragment += tab + "vec3 normalDecoded =  normalize(normal.xyz);				\n";
				calcFragment += tab + "vec3 viewpos = normalize( world.xyz - m_cameraPos.xyz);	\n";
				calcFragment += tab + "float d = dot(normalDecoded.xyz, viewpos);				\n";
				calcFragment += tab + "vec3 reflected = normalDecoded.xyz *2.0 * d - viewpos;	\n";

				for (auto i = 0; i < shadStages.size(); ++i)
				{
					const auto& curStage = shadStages[i];
					calcFragment += tab + vec2 + space + uvNames[i] + semicol + tab + endLn;

					//apply tc gen
					switch (curStage.m_tcGen.m_tcGen)
					{
					case eQTcGen::BASE:
					case eQTcGen::LIGHTMAP:
						calcFragment += tab + uvNames[i] + assign + uvSets[i] + semicol + endLn;
						break;
					case eQTcGen::ENVIRONMENT:
						calcFragment += tab + openBrack + endLn;
						calcFragment += doubleTab + uvNames[i] + " = vec2( 0.5, 0.5 ) + reflected.xy * 0.5;\n";
						calcFragment += tab + closeBrack + endLn;
						break;
					case eQTcGen::VECTOR:
						calcFragment += tab + uvNames[i] + assign + uvSets[i] + semicol + endLn;
						//calcFragment += doubleTab + uvNames[i] + assign + uvSets[i] + semicol + endLn; //TODO
						break;
					}

					//apply texture coord mods if any
					for (auto j = 0; j < curStage.m_texMods.size(); ++j)
					{
						String iIndex = to_string(i);
						String jIndex = to_string(j);
						String stage = "shaderStages" + arrStart + iIndex + arrEnd;
						stage += dot + String("texMod") + arrStart + jIndex + arrEnd;

						//begin new tcmod with brackets
						String modStr = tab + openBrack + endLn;
						const auto& curMod = curStage.m_texMods[j];
						switch (curMod.m_tcMod)
						{
						case eQ3TcMod::NONE:
							break;
						case eQ3TcMod::SCROLL:
							modStr += comment + "Scroll" + endLn;
							modStr += doubleTab + uvNames[i] + assignAdd + makeVec2(curMod.m_scroll[0], curMod.m_scroll[1]) +
								space + mul + String("m_programTime") + semicol + endLn;
							break;
						case eQ3TcMod::ROTATE:
						{
							auto angle = to_string(ToRadians(curMod.m_rotSpeed)) + mul + String("m_programTime");
							auto uvNamesX = uvNames[i] + dot + "x";
							auto uvNamesY = uvNames[i] + dot + "y";
							auto x = doubleTab + String("float x = ") + uvNamesX + "-0.5;\n";
							auto y = doubleTab + String("float y = ") + uvNamesY + "-0.5;\n";
							auto cX = doubleTab + String("float cX = ") + "cos( " + angle + " );\n";
							auto sY = doubleTab + String("float sY = ") + "sin( " + angle + " );\n";
							auto xResult = doubleTab + uvNamesX + assign + "(x * cX - y * sY) + 0.5;\n";
							auto yResult = doubleTab + uvNamesY + assign + "(x * sY + y * cX) + 0.5;\n";
							modStr += comment + "Rotate" + endLn;
							modStr += x + y + cX + sY + xResult + yResult;
							break;
						}

						case eQ3TcMod::SCALE:
						{
							auto vec2Var = makeVec2(curMod.m_scale[0], curMod.m_scale[1]);
							modStr += comment + "Scale" + endLn;
							modStr += doubleTab + uvNames[i] + assignMul + vec2Var + ";\n";
							break;
						}
						case eQ3TcMod::TRANSFORM:
						{
							modStr += comment + "Transform" + endLn;
							break;
						}
						case eQ3TcMod::STRETCH:
						{
							modStr += comment + "Stretch" + endLn;
							
							const auto& wave = curMod.m_waveForm;						

							modStr += doubleTab + "float base   = " + to_string(wave.m_base) + ";\n";
							modStr += doubleTab + "float phase  = " + to_string(wave.m_phase) + ";\n";
							modStr += doubleTab + "float amp    = " + to_string(wave.m_amp) + ";\n";
							modStr += doubleTab + "float freq   = " + to_string(wave.m_freq) + ";\n";
							modStr += doubleTab + makeWave3(wave.m_wavefunc, "base", "phase", "amp", "freq ");							
							modStr += doubleTab + "waveResult = 1.0" + div + "waveResult;" + endLn;
							modStr += doubleTab + uvNames[i] + assignMul + "waveResult;" + endLn;
							modStr += doubleTab + uvNames[i] + assignAdd + "vec2( 0.5 - (0.5 * waveResult), 0.5 - (0.5 * waveResult));" + endLn;
							break;
						}
						case eQ3TcMod::TURB:
						{
							auto& wf = curMod.m_waveForm;
							modStr += comment + "Turb" + endLn;
							modStr += doubleTab + "float turbVal  = " + to_string(wf.m_phase) + " + m_programTime * " + to_string(wf.m_freq) + ";\n";
							modStr += doubleTab + "float amp = " + to_string(wf.m_amp) + ";\n";
							auto uvNamesX = uvNames[i] + dot + "x";
							auto uvNamesY = uvNames[i] + dot + "y";
							modStr += doubleTab + uvNamesX + " += sin( ( ( m_cameraPos.x + m_cameraPos.y ) * 1.0 / 128.0 * 0.125f + turbVal )  ) * 6.2831 * amp;\n";
							modStr += doubleTab + uvNamesY + " += sin( ( m_cameraPos.z * 1.0 / 128.0 * 0.125f + turbVal ) )  * 6.2831 * amp;\n";
							break;
						}
						default:
							throw std::exception("Invalid tcMod");
						}
						//add texture mod
						modStr += tab + closeBrack + endLn;
						calcFragment += modStr;
					}

					auto texColor = tab + vec4 + texNames[i] + assign + texture + funcStart +
						textureNames[i] + comma + uvNames[i] + funcEnd + semicol + endLn;

					calcFragment += texColor;
				}

				
				calcFragment += tab + String("vec4 finalColor = ") + texNames[0] + ";\n";
				calcFragment += tab + String("vec4 src, dst;\n");
				calcFragment += tab + String("vec4 vertexColor = rgba;\n");

				{
					auto numStages = shader->m_shaderStages.size();
					bool isFirst = true;
					bool isLast  = false;
					int start = shader->isSolid() ? 0 : 0;
					for (auto i = start; i < numStages; ++i)
					{
						isFirst = (i == start);
						isLast = (i == numStages - 1);

						const auto& shadStage = shader->m_shaderStages[i];					
						//blend equations
						calcFragment += tab + String("dst = finalColor;\n");
						calcFragment += tab + String("src = ") + texNames[i] + String(";\n");
						calcFragment += ApplyRGBAGen(shadStage);
						calcFragment += ApplyBlendFunc(shadStage);
						
						if (shadStage.m_rgbaGen.m_rgbType == eQ3RgbGen::IDENTITY 
							/*|| (!shadStage.m_hasAlphaMap )*/ )
						{
							calcFragment += tab + String("finalColor.a = 1.0;\n");
						}					
					} //blending
				}

#if 0
				calcFragment += tab + String("finalColor = vec4(finalColor.x, normal.w, normal.w, 1.0 );\n");
			//	calcFragment += tab + String("finalColor = vec4( (normalDecoded.xyz * 0.5) + 0.5 ,  1.0 );\n");
#endif		
				calcFragment += tab + String("return finalColor;\n\n");
				
				calcFragment += String("}\n");
			
				fragProgram += calcFragment;
			}
			if(shader->isSolid() )
				fragProgram += fragMain;
			else
				fragProgram += fragMainAlpha;
		}		
	
		return true;
	}



	Q3ParseShader::Q3ParseShader(App::EngineContext* context, const String& fileName)
		: m_context(context)
		, m_fileName(fileName)
		, m_dataPtr(nullptr)
		, m_lineNumber(1)
	{
	
	}

	bool Q3ParseShader::parseShaderStage(Q3ShaderStage& curStage, Q3ShaderPtr shader)
	{
		using namespace Common;

		String curToken;
		if (!getToken(curToken)) {
			printError("Invalid shader stage");
			skipToNewLine();
			return false;
		}
		//don't parse tokens in the ignore list
		if (IgnoreStageDirective(curToken))
		{
			skipToNewLine();
			return true;
		}

		curToken = ToLower(curToken);
		auto idx = IsStageDirective(curToken);
		if (idx == INVALID_INDEX)
		{
			printError("Invalid stage directive: ", curToken);
			skipToNewLine();
			return false;
		}

		auto result = true;
		if (StringEquals("map", curToken, true))
		{
			result = getToken(curToken);
			if (result)
			{
				if (StringEquals("$lightmap", curToken, true))
					curStage.m_lightmap = true;
				else
					curStage.m_textures.push_back(App::StripExtension(curToken));
			}
		}
		else if (StringEquals("clampmap", curToken, true))
		{
			result = getToken(curToken);
			if (result) {
				curStage.m_clamp = true;
				curStage.m_textures.push_back(App::StripExtension(curToken));
			}
		}

		else if (StringEquals("videomap", curToken, true))
		{
			result = getToken(curToken);
			if (result) {
				curStage.m_video = true;
				curStage.m_textures.push_back(curToken);
			}
		}
		else if (StringEquals("tcgen", curToken, true))
			result = parseTcGen(curStage.m_tcGen);

		else if (StringEquals("tcmod", curToken, true))
		{
			Q3TextureMod tcMod;
			result = parseTcMod(tcMod);
			if (result) {
				curStage.m_texMods.emplace_back(tcMod);
				
			}
		}
		else if (StringEquals("depthwrite", curToken, true))
			result = curStage.m_depthWrite = true;
		else if (StringEquals("blendfunc", curToken, true))
			result = parseBlendFunc(curStage);
		else if (StringEquals("depthFunc", curToken, true))
			parseDepthFunc(curStage);
		else if (StringEquals("alphaFunc", curToken, true))
			result = parseAlphaFunc(curStage);
		else if (StringEquals("alphagen", curToken, true))
			result = parseRGBAGen(curStage.m_rgbaGen, true);
		else if (StringEquals("rgbgen", curToken, true))
			result = parseRGBAGen(curStage.m_rgbaGen, false);
		else if (StringEquals("animmap", curToken, true))
			result = parseAnimMap(curStage);
		else if (StringEquals("detail", curToken, true))
			skipToNewLine();
		else
			result = false;
		if (!result)
			printError("Invalid stage: ", curToken);
		skipToNewLine();
		return result;
	}

	bool Q3ParseShader::parseShaderLocal(Q3ShaderPtr curShader)
	{
		using namespace Common;
		String curToken;
		if (!getToken(curToken))
		{
			printError("Invalid shader");
			skipToNewLine();
			return false;
		}

		//don't parse tokens in the ignore list
		if (IgnoreShaderDirective(curToken))
		{
			skipToNewLine();
			return true;
		}

		curToken = ToLower(curToken);
		auto idx = IsShaderDirective(curToken);
		if (idx == INVALID_INDEX)
		{
			printError("Invalid shader directive: ", curToken);
			skipToNewLine();
			return false;
		}

		auto result = true;
		if (StringEquals("nomipmaps", curToken))
			curShader->m_mipmaps = false;
		else if (StringEquals("portal", curToken))
			curShader->m_sufaceFlags |= SURFACE_PORTAL;
		else if (StringEquals("sky", curToken))
			curShader->m_sufaceFlags |= SURFACE_SKY;
		else if (StringEquals("polygonoffset", curToken))
			curShader->m_polyOffset = true;
		else if (StringEquals("cull", curToken))
			result = parseCull(curShader->m_cullFace);
		else if (StringEquals("surfaceparm", curToken))
			result = parseSurfaceParam(curShader->m_sufaceFlags, curShader->m_contents);
		else if (StringEquals("fogparms", curToken))
			parseFogParam(curShader->m_fogParams, curShader->m_fogOpacity);
		else if (StringEquals("tesssize", curToken))
			result = parseInt(curShader->m_tessSize);
		else if (StringEquals("light", curToken) || StringEquals("light1", curToken))
			result = parseFloat(curShader->m_light);
		else if (StringEquals("skyparms", curToken))
		{
			result = parseSkybox(curShader->m_skyBox);
			curShader->m_sufaceFlags |= SURFACE_SKY;
		}
		else if (StringEquals("deformvertexes", curToken)) {
			Q3VertexDeform vDeform;
			result = parseVertexDeform(vDeform);
			if (result)
				curShader->m_vertexDeform.push_back(vDeform);
		}
		else if (StringEquals("fogonly", curToken))
			result = ContentsForToken("fog", curShader->m_contents);
		//ignore these for now
		else if (StringEquals("sort", curToken) )
			result = parseSort(curShader->m_sort);
		else if (StringEquals("nopicmip", curToken))
		{

		}
		else
			result = false;
		if (!result)
			printError("Invalid shader directive: ", curToken);

		skipToNewLine();
		return true;
	}

	bool Q3ParseShader::parseShaderFile()
	{
		using namespace Common;
		QFile shaderFile(m_fileName.c_str());
		if (!shaderFile.open(QFile::ReadOnly))
			return false;

		App::AddConsoleMessage(m_context, String("Begin parsing of: ") + m_fileName);

		//TODO use a lexical analyzer in the future
		QTextStream in(&shaderFile);
		QString result;
		while (!in.atEnd())
		{
			QString line = in.readLine();
			result += line + "\n";
		}

		auto data = result.toStdString();
		m_dataPtr = data.data();
		enum eShaderState
		{
			STATE_SHADER_GLOBAL = 0,
			STATE_SHADER_LOCAL = 1,
			STATE_SHADER_STAGE = 2
		};

		Q3ShaderPtr		curShader;
		Q3ShaderStage	curStage(m_context);

		eShaderState	curState = STATE_SHADER_GLOBAL;	//init state
		String		curToken;
		do
		{
			//skip empty space
			while (isWhiteSpace(peekChar()))
				m_dataPtr++;

			if (peekChar() == '{') //entering shader or shader state
			{
				eatChar(); //eat char
				switch (curState)
				{
				case STATE_SHADER_GLOBAL:
					curState = STATE_SHADER_LOCAL;
					break;
				case STATE_SHADER_LOCAL:
					curState = STATE_SHADER_STAGE;
					break;
				default:
					throw std::exception("Invalid shader state");
				}
			}
			else if (peekChar() == '}') //leaving state or shader
			{
				eatChar(); //eat char
				switch (curState)
				{
				case STATE_SHADER_LOCAL:	//leaving shader scope --> add shader to list to list
					curState = STATE_SHADER_GLOBAL;
					m_shaders.emplace_back(curShader);

					curShader = nullptr;
					break;
				case STATE_SHADER_STAGE:	//leaving shader stage scope
					curState = STATE_SHADER_LOCAL;
					curShader->m_shaderStages.push_back(curStage);
					curStage.reset();
					break;
				default:
					throw std::exception("Invalid shader state");
				}
			}
			else if (peekToken(curToken))
			{
				if (IgnoreGlobalDirective(curToken))
				{
					skipToNewLine();
					continue;
				}

				//global scope --> parse the shader name
				if (curState == STATE_SHADER_GLOBAL)
				{
					// AddConsoleMessage(m_context, "\tParse shader: " + curToken);
					curShader = std::make_shared<Q3Shader>(m_context, m_fileName, curToken);
					skipToNewLine();
				}
				else if (curState == STATE_SHADER_LOCAL) //local shader scope --> parse shader properties
				{
					if (!parseShaderLocal(curShader))
					{
						skipToNewLine();
						continue;
					}
				}
				else if (curState == STATE_SHADER_STAGE) //shader stage scope --> parse shader stage 
				{
					if (!parseShaderStage(curStage, curShader))
					{
						skipToNewLine();
						continue;
					}
				}
				else
					throw std::exception("Invalid shader state");
			}
		} while (*m_dataPtr != '\0');

		App::AddConsoleMessage(m_context, String("Parsed  ") + std::to_string(m_shaders.size())
			+ String(" from: ") + m_fileName);

		return true;
	}



	bool Q3ParseShader::isWhiteSpace(char val)
	{
		if (Common::isNewLine(val))
			m_lineNumber++;

		return (val == ' ' || val == '\t' || val == '\n');
	}

	void Q3ParseShader::ungetChar()
	{
		--m_dataPtr;
	}

	char Q3ParseShader::eatChar()
	{
		auto res = *(m_dataPtr++);
		return res;
	}

	char Q3ParseShader::peekChar() const
	{
		auto res = *(m_dataPtr);
		return res;
	}

	char Q3ParseShader::peekCharWS()
	{
		auto backup = m_dataPtr;
		while (isWhiteSpace(peekChar())) //skip empty space
			m_dataPtr++;
		auto res = peekChar();
		m_dataPtr = backup;
		return res;
	}

	String Q3ParseShader::skipToNewLine()
	{
		String remain;
		while (peekChar() != '\n') {
			remain += *m_dataPtr;
			m_dataPtr++;
		}
		return remain;
	}

	bool Q3ParseShader::parseToken(String& result)
	{
		result = "";
		while (Common::isValidCharToken(peekChar()))
			result += eatChar();
		return !result.empty();
	}

	bool Q3ParseShader::getToken(String& result)
	{
		while (isWhiteSpace(peekChar())) //skip empty space
			m_dataPtr++;
		return parseToken(result);
	}

	bool Q3ParseShader::peekToken(String& result)
	{
		const auto* restorePt = m_dataPtr;
		auto valid = getToken(result);
		m_dataPtr = restorePt;
		return valid;
	}
	
	bool Q3ParseShader::parseFloat(float& result)
	{
		String token;
		if (!getToken(token))
			return false;
		try {
			result = std::stof(token);
		}
		catch (...) {
			printError("Error parsing float");
			result = 0.0f;
			return false;
		}
		return true;
	}

	bool Q3ParseShader::parseInt(int& result)
	{
		String token;
		if (!getToken(token))
			return false;
		try {
			result = std::stoi(token);
		}
		catch (...)
		{
			printError("Error parsing int");
			result = 0;
			return false;
		}

		return true;
	}

	bool Q3ParseShader::parseVec3(Math::Vector3f& result)
	{
		using namespace Common;
		auto readVec3 = [this, &result]() -> bool
		{
			for (auto& elem : result.m_data)
				if (!parseFloat(elem))
					return false;
			return true;
		};

		//a vector3 can be stored like this ( a, b, c ) or plain like this a b c
		String token;
		if (!peekToken(token))
			return false;

		if (!StringEquals("(", token))
		{
			bool succes = readVec3();
			if (succes)
				return true;
			printError("Error parsing vec3");
			return false;
		}
		getToken(token);

		bool succes = readVec3();
		if (!succes)
		{
			printError("Error parsing vec3");
			return false;
		}
		if (!getToken(token))
			return false;

		if (!StringEquals(")", token))
		{
			printError("Error parsing vec3");
			return false;
		}
		return true;
	}
	

	bool  Q3ParseShader::parseRGBAGen(Q3RgbaGen& val, bool isAlpha)
	{
		using namespace Common;
		String token;
		if (!getToken(token))
			return false;
		auto& wave = isAlpha ? val.m_alphaWaveForm : val.m_rgbWaveForm;
		auto& type = isAlpha ? val.m_alphaType : val.m_rgbType;


		bool result = true;
		if (StringEquals("identity", token, true) || StringEquals("identityLighting", token, true))
			type = eQ3RgbGen::IDENTITY;
		else if (StringEquals("lightingDiffuse", token, true))
			type = eQ3RgbGen::LIGHTNING_DIFFUSE;
		else if (StringEquals("vertex", token, true))
			type = eQ3RgbGen::VERTEX;
		else if (StringEquals("entity", token, true))
			type = eQ3RgbGen::ENTITY;
		else if (StringEquals("exactvertex", token, true))
			type = eQ3RgbGen::EXACTVERTEX;
		else if (StringEquals("portal", token, true))
			type = eQ3RgbGen::ALPHA_PORTAL;
		else if (StringEquals("lightingSpecular", token, true))
			type = eQ3RgbGen::ALPHA_LIGHTING_SPEC;
		else if (StringEquals("wave", token, true)) {
			result &= parseWave(wave);
			if (result)
				type = eQ3RgbGen::WAVE;
		}
		else
			result = false;
		if (!result)
			printError("Error parsing rgb(A)Gen", token);
		return result;
	}

	bool Q3ParseShader::parseSort(int& result)
	{
		using namespace Common;
		result = eShaderSort::SORT_OPAQUE;

		String curToken;
		if (!getToken(curToken)) {
			printError("Invalid sort parameter");
			return false;
		}

		if (StringEquals("portal", curToken, true) )		
			result = eShaderSort::SORT_PORTAL;
		else if(StringEquals("sky", curToken, true))
			result = eShaderSort::SORT_SKY;
		else if (StringEquals("opaque", curToken, true))
			result = eShaderSort::SORT_OPAQUE;
		else if (StringEquals("banner", curToken, true))
			result = eShaderSort::SORT_BANNER;
		else if (StringEquals("underwater", curToken, true))
			result = eShaderSort::SORT_UNDERWATER;
		else if (StringEquals("additive", curToken, true))
			result = eShaderSort::SORT_ADDITIVE;
		else if (StringEquals("nearest", curToken, true))
			result = eShaderSort::SORT_NEAREST;
		else
		{
			try{
				result = std::atoi(curToken.c_str());
			}
			catch (...){
				return false;
			}		
		}

		return true;
	}

	bool Q3ParseShader::parseVertexDeform(Q3VertexDeform& vd)
	{
		using namespace Common;
		String token;
		if (!getToken(token))
			return false;

		bool result = true;
		if (StringEquals("move", token, true))
		{
			vd.m_vertexDeform = eQ3VertexDeformFunc::VD_MOVE;
			result &= parseFloat(vd.m_dvMove[0]);
			result &= parseFloat(vd.m_dvMove[1]);
			result &= parseFloat(vd.m_dvMove[2]);
			result &= parseWave(vd.m_waveForm);
		}
		else if (StringEquals("normal", token, true))
		{
			vd.m_vertexDeform = eQ3VertexDeformFunc::VD_NORMAL;
			result &= parseFloat(vd.m_dvNormal[0]);
			result &= parseFloat(vd.m_dvNormal[1]);
		}
		else if (StringEquals("bulge", token, true))
		{
			vd.m_vertexDeform = eQ3VertexDeformFunc::VD_BULGE;
			result &= parseFloat(vd.m_dvBulge[0]); //width
			result &= parseFloat(vd.m_dvBulge[1]); //height
			result &= parseFloat(vd.m_dvBulge[2]); //speed
		}
		else if (StringEquals("wave", token, true))
		{
			vd.m_vertexDeform = eQ3VertexDeformFunc::VD_WAVE;
			result &= parseFloat(vd.m_dvDiv);
			result &= parseWave(vd.m_waveForm);
		}
		else if (StringEquals("autosprite", token, true))
			vd.m_vertexDeform = eQ3VertexDeformFunc::VD_AUTOSPRITE;
		else if (StringEquals("autosprite2", token, true))
			vd.m_vertexDeform = eQ3VertexDeformFunc::VD_AUTOSPRITE2;
		else if (StringEquals("text0", token, true))
			vd.m_vertexDeform = eQ3VertexDeformFunc::VD_TEXT0;
		else if (StringEquals("projectionShadow", token, true))
			vd.m_vertexDeform = eQ3VertexDeformFunc::VD_TEXT1;
		if (!result) {
			vd.m_vertexDeform = eQ3VertexDeformFunc::NONE;
			printError("Error parsing vertex deform");
		}
		return result;
	}

	bool Q3ParseShader::parseWave(Q3WaveForm& wave)
	{
		using namespace Common;
		String token;
		if (!getToken(token))
			return false;
		auto result = true;
		if (StringEquals("sin", token, true) || StringEquals("sine", token, true))
			wave.m_wavefunc = eQ3WaveFunc::SIN;
		else if (StringEquals("inversesawtooth", token, true))
			wave.m_wavefunc = eQ3WaveFunc::INV_SAWTOOTH;
		else if (StringEquals("sawtooth", token, true))
			wave.m_wavefunc = eQ3WaveFunc::SAWTOOTH;
		else if (StringEquals("triangle", token, true))
			wave.m_wavefunc = eQ3WaveFunc::TRIANGLE;
		else if (StringEquals("square", token, true))
			wave.m_wavefunc = eQ3WaveFunc::SQUARE;
		else if (StringEquals("noise", token, true))
			wave.m_wavefunc = eQ3WaveFunc::NOISE;
		else
			result = false;

		result &= parseFloat(wave.m_base);
		result &= parseFloat(wave.m_amp);
		result &= parseFloat(wave.m_phase);
		result &= parseFloat(wave.m_freq);
		if (!result)
			printError("Error parsing waveform");

		return result;
	}

	bool Q3ParseShader::parseSkybox(Q3SkyBox& box)
	{
		auto result = true;
		result &= getToken(box.m_farBox);
		result &= getToken(box.m_height);
		result &= getToken(box.m_nearBox);
		if (!result)
			printError("Error parsing skyparm");

		return result;
	}

	bool Q3ParseShader::parseDepthFunc(Q3ShaderStage& shaderStage)
	{
		String token;
		if (!getToken(token))
			return false;
		if (Common::StringEquals("equal", token, true))
			shaderStage.m_depthFunc = GL_EQUAL;
		else {
			printError("Error depthfunc", token);
			return false;
		}
		return true;
	}

	bool Q3ParseShader::parseBlendFunc(Q3ShaderStage& shaderStage)
	{
		using namespace Common;
		static std::map<String, int> blendLUT;
		blendLUT["GL_ZERO"] = GL_ZERO;
		blendLUT["GL_ONE"] = GL_ONE;
		blendLUT["GL_DST_COLOR"] = GL_DST_COLOR;
		blendLUT["GL_DST_ALPHA"] = GL_DST_ALPHA;
		blendLUT["GL_SRC_COLOR"] = GL_SRC_COLOR;
		blendLUT["GL_SRC_ALPHA"] = GL_SRC_ALPHA;
		blendLUT["GL_ONE_MINUS_DST_COLOR"] = GL_ONE_MINUS_DST_COLOR;
		blendLUT["GL_ONE_MINUS_DST_ALPHA"] = GL_ONE_MINUS_DST_ALPHA;
		blendLUT["GL_ONE_MINUS_SRC_COLOR"] = GL_ONE_MINUS_SRC_COLOR;
		blendLUT["GL_ONE_MINUS_SRC_ALPHA"] = GL_ONE_MINUS_SRC_ALPHA;


		String srcBlend,
			dstBlend;

		if (!getToken(srcBlend)) {
			printError("Error parsing blendfunc:", srcBlend);
			return false;
		}

		bool result = true;
		if (StringEquals("filter", srcBlend))
		{
			shaderStage.m_blendFunc[0] = GL_DST_COLOR;
			shaderStage.m_blendFunc[1] = GL_ZERO;
		}
		else if (StringEquals("add", srcBlend, true) ||
			StringEquals("GL_add", srcBlend, true))
		{
			shaderStage.m_blendFunc[0] = GL_ONE;
			shaderStage.m_blendFunc[1] = GL_ONE;
		}
		else if (StringEquals("blend", srcBlend, true))
		{
			shaderStage.m_blendFunc[0] = GL_SRC_ALPHA;
			shaderStage.m_blendFunc[1] = GL_ONE_MINUS_SRC_ALPHA;
		}
		else
		{
			if (!getToken(dstBlend))
			{
				printError("Error parsing blendfunc:", srcBlend + " " + dstBlend);
				return false;
			}
			srcBlend = ToUpper(srcBlend);
			dstBlend = ToUpper(dstBlend);
			result &= blendLUT.find(srcBlend) != std::end(blendLUT);
			result &= blendLUT.find(dstBlend) != std::end(blendLUT);

			if (result)
			{
				shaderStage.m_blendFunc[0] = blendLUT[srcBlend];
				shaderStage.m_blendFunc[1] = blendLUT[dstBlend];
			}
			else
				printError("Error find blend LUT:", srcBlend + " " + dstBlend);
		}

		return result;
	}

	bool Q3ParseShader::parseTcGen(Q3TcGen& tcGen)
	{
		using namespace Common;
		String token;
		if (!getToken(token)) {
			printError("Error parsing tcGen");
			return false;
		}
		bool result = true;
		if (StringEquals("environment", token, true))
			tcGen.m_tcGen = eQTcGen::ENVIRONMENT;
		else  if (StringEquals("base", token, true))
			tcGen.m_tcGen = eQTcGen::BASE;
		else if (StringEquals("lightmap", token, true))
			tcGen.m_tcGen = eQTcGen::LIGHTMAP;
		else if (StringEquals("vector", token, true))
		{
			tcGen.m_tcGen = eQTcGen::VECTOR;
			result &= parseVec3(tcGen.m_v1);
			result &= parseVec3(tcGen.m_v2);
		}
		else
			printError("Error parsing tcGen: ", token);
		return result;
	}

	bool Q3ParseShader::parseTcMod(Q3TextureMod& tcMod)
	{
		using namespace Common;
		String token;
		if (!getToken(token)) {
			printError("Error parsing tcMod");
			return false;
		}

		bool result = true;
		if (StringEquals("scroll", token, true))
		{
			tcMod.m_tcMod = eQ3TcMod::SCROLL;
			result &= parseFloat(tcMod.m_scroll[0]);
			result &= parseFloat(tcMod.m_scroll[1]);

		}
		else  if (StringEquals("scale", token, true))
		{
			tcMod.m_tcMod = eQ3TcMod::SCALE;
			result &= parseFloat(tcMod.m_scale[0]);
			result &= parseFloat(tcMod.m_scale[1]);
		}
		else if (StringEquals("rotate", token, true))
		{
			tcMod.m_tcMod = eQ3TcMod::ROTATE;
			result &= parseFloat(tcMod.m_rotSpeed);
		}
		else if (StringEquals("transform", token, true))
		{
			tcMod.m_tcMod = eQ3TcMod::TRANSFORM;
			result &= parseFloat(tcMod.m_transform[0][0]);
			result &= parseFloat(tcMod.m_transform[0][1]);
			result &= parseFloat(tcMod.m_transform[1][0]);
			result &= parseFloat(tcMod.m_transform[1][1]);
			result &= parseFloat(tcMod.m_translation[0]);
			result &= parseFloat(tcMod.m_translation[1]);
		}
		else if (StringEquals("turb", token, true))
		{
			tcMod.m_tcMod = eQ3TcMod::TURB;
			peekToken(token);
			if (StringEquals(token, "sin", true))
				result = parseWave(tcMod.m_waveForm);
			else
			{
				result &= parseFloat(tcMod.m_waveForm.m_base);
				result &= parseFloat(tcMod.m_waveForm.m_amp);
				result &= parseFloat(tcMod.m_waveForm.m_phase);
				result &= parseFloat(tcMod.m_waveForm.m_freq);
			}

		}
		else if (StringEquals("stretch", token, true))
		{
			result &= parseWave(tcMod.m_waveForm);
			tcMod.m_tcMod = eQ3TcMod::STRETCH;
		}
		else
			printError("Error parsing tcMod:", token);
		return result;
	}

	bool Q3ParseShader::parseCull(int& result)
	{
		using namespace Common;
		result = GL_BACK;
		
		String curToken;
		if (!getToken(curToken)) {
			printError("Invalid cull parameters");
			return false;
		}

		if (StringEquals("back", curToken, true) ||
			StringEquals("backSided", curToken, true))
			result = GL_BACK;
		else if (StringEquals("front", curToken, true))
			result = GL_FRONT;
		else if ( StringEquals("disable",	curToken, true) ||
				  StringEquals("none",		curToken, true) ||
				  StringEquals("twoSided",	curToken, true))
			result = GL_NONE;
		else {
			printError("Invalid cull id: ", curToken);
			return false;
		}
		return true;
	}

	bool Q3ParseShader::parseFogParam(Math::Vector3f& fogColor, float& fogOpacity)
	{
		auto result = parseVec3(fogColor) && parseFloat(fogOpacity);
		if (!result)
			printError("Invalid fog parameters");
		return result;

	}

	bool Q3ParseShader::parseAlphaFunc(Q3ShaderStage& shaderStage)
	{
		using namespace Common;
		String curToken;
		if (!getToken(curToken)) {
			printError("Invalid alphafunc parameters");
			return false;
		}
		if (StringEquals("GT0", curToken, true))
			shaderStage.m_alphaFunc = eQ3AlphaFunc::GREATER_THAN0;
		else if (StringEquals("LT128", curToken, true))
			shaderStage.m_alphaFunc = eQ3AlphaFunc::LESS_THAN128;
		else if (StringEquals("GE128", curToken, true))
			shaderStage.m_alphaFunc = eQ3AlphaFunc::GEQUALS_THAN128;
		else {
			printError("Invalid alphafunc id: ", curToken);
			return false;
		}
		return true;
	}

	bool Q3ParseShader::parseAnimMap(Q3ShaderStage& shaderStage)
	{
		bool result = parseFloat(shaderStage.m_animSpeed);
		if (!result)
			return false;

		auto token = skipToNewLine();
		if (token.empty())
			return false;

		shaderStage.m_animated = true;
		std::istringstream ss(token, std::istringstream::in);
		String animTex;
		while (ss >> animTex)
			shaderStage.m_textures.push_back(App::StripExtension(animTex));

		return !shaderStage.m_textures.empty();
	}

	bool Q3ParseShader::parseSurfaceParam(std::uint32_t& surface, std::uint32_t& contents)
	{
		String curToken;
		if (!getToken(curToken)) {
			printError("Invalid surface parameters");
			return false;
		}

		auto result = false;
		result = result || SurfParamForToken(curToken, surface);
		result = result || ContentsForToken(curToken, contents);

		if (!result)
			printError("Invalid surfaceparm id: ", curToken);

		return result;
	}



	void Q3ParseShader::printError(const String& str, const String& optional) const
	{
		auto msg = str + String(" ") + optional;
		msg += " [";
		msg += m_fileName;
		msg += "][";
		msg += std::to_string(m_lineNumber);
		msg += "]";
		AddConsoleMessage(m_context, msg, App::LOG_LEVEL_WARNING);
	}

	Q3Shader::Q3Shader(App::EngineContext* context, const String& fileName, const String& shadName)
		: App::Resource(context)
		, m_transparent(false)
		, m_loaded(false)
		, m_mipmaps(true)
		, m_contents(eQ3SurfaceContents::CONTENT_SOLID)
		, m_sufaceFlags(eQ3SurfaceParam::SURFACE_NONE)
		, m_cullFace(GL_BACK)
		, m_tessSize(0)
		, m_sort(eShaderSort::SORT_PORTAL)
		, m_polyOffset(false)
		, m_light(0.0f)
		, m_fogOpacity(0.0f)
		, m_path(fileName)
		, m_name(shadName)		
	{

	}

	bool Q3Shader::unloadShader()
	{
		if (!m_loaded)
			return true;
		//clear all textures
		auto resMan = getContext()->getSystem<App::ResourceManager>();
		if( m_gpuShader)
			resMan->removeResource( m_gpuShader->getResourceHandle() );
		for (auto& stage : m_shaderStages)
		for (auto& tex : stage.m_textureList)
			resMan->removeResource(tex->getResourceHandle());

		return true;
	}


	bool Q3Shader::isDefault() const
	{
		if (m_shaderStages.size() != 2)
			return false;
		if (m_shaderStages[0].m_blendFunc[0] != GL_ONE ||
			m_shaderStages[0].m_blendFunc[1] != GL_ZERO)
			return false;
		if (m_shaderStages[1].m_blendFunc[0] != GL_DST_COLOR ||
			m_shaderStages[1].m_blendFunc[1] != GL_ZERO)
			return false;

		return true;
	}




	bool Q3Shader::isSolid() const
	{
		if (m_shaderStages.empty())
			return true;
		return  m_shaderStages[0].m_blendFunc[0] == GL_ONE &&
			m_shaderStages[0].m_blendFunc[1] == GL_ZERO;
	}

	bool Q3Shader::isMultiPass() const
	{
		auto alphaMask = hasAlphaTest();
		auto numAlphaStages = Math::popCount8(static_cast<std::uint8_t>(alphaMask));
		if (numAlphaStages > 1)
			return true;
		return (!isSolid() && numAlphaStages);
	}

	bool Q3Shader::hasVertexDeform() const
	{
		return !m_vertexDeform.empty();
	}

	std::uint32_t Q3Shader::hasAutoSprite() const
	{
		std::uint32_t result = 0;
		
		if (!hasVertexDeform())
			return result;
		for (auto& vDef : m_vertexDeform)
		{
			auto type = vDef.m_vertexDeform;
			switch (type) {
				case eQ3VertexDeformFunc::VD_AUTOSPRITE:
				case eQ3VertexDeformFunc::VD_AUTOSPRITE2:
					result |= /*1 << */static_cast<std::uint32_t>(type);
					break;
				default:
					break;
			}
		}
		return result;
	}

	bool Q3Shader::generateGLSL()
	{
		String vertShader;
		String fragShader;
		if (!BuildOpenGLShader(this, vertShader, fragShader))
			return false;

		//create resource name
		auto shaderName = m_name + "_q3glsl";
		auto resman = getContext()->getSystem<App::ResourceManager>();

#if DEBUG
		assert(resman->resourceNameAvailable(shaderName.c_str()));
#endif
		//crate a new glsl shader
		auto shader = std::dynamic_pointer_cast<App::Shader>(resman->createResource("Shader"));
		shader->setResourceName(shaderName);
		if ( !shader->load( vertShader, fragShader) ) 
		{
			AddConsoleMessage( m_context, String( "Error loading shader: " ) + m_name, App::LOG_LEVEL_WARNING);
			return false;
		}		
		AddConsoleMessage( m_context, String("Loaded shader: ") + m_name, App::LOG_LEVEL_INFO);
		resman->addResource(shader, false);		
		m_gpuShader = shader;

		shader->bind();
		//register common glsl uniforms(autosprite)
		m_useSkyBox.registerUniform("useSkyBox",		shader);
		m_skyMatrix.registerUniform("skyMatrix",		shader);
		m_asStartIdx.registerUniform("asStartIdx",		shader);
		m_asCenter.registerUniform("asCenter",			shader);
		m_asMajorDir.registerUniform("asMajorDir",		shader);		
		m_asWidth.registerUniform("asWidth",			shader);
		m_asHeight.registerUniform("asHeight",			shader);

		//register texture stages
		for (int i = 0; i < m_shaderStages.size(); ++i)
		{
			auto& stage = m_shaderStages[i];
			//stage.m_uniform.registerUniform( SamplerNames[i].c_str(), shader );
		}

		shader->unBind();

		return true;
	}

	Misc::Q3ShaderPtr Q3Shader::CreateRegularShader( App::EngineContext* context, const String& name)
	{
		return CreateRegularShaderImp(context, name);
	}

	Misc::Q3ShaderPtr Q3Shader::CreateFallBackShader(App::EngineContext* context)
	{
		Q3ShaderPtr result = std::make_shared<Q3Shader>(context);
		result->m_name = FALLBACK_SHADER;
		result->m_path = "";

		SurfParamForToken("nolightmap", result->m_sufaceFlags);

		auto resMan = context->getSystem<App::ResourceManager>();

		Q3ShaderStage defStage(context);
		defStage.m_lightmap = false;
		defStage.m_textures.emplace_back("DefaultAlbedo");
		defStage.m_textureList.push_back(resMan->getResourceSafe<App::Texture>("DefaultAlbedo"));
		result->m_shaderStages.push_back(defStage);
		return result;
	}

	void Q3Shader::setDrawInfo(const Q3DrawInfo& dI)
	{
		m_asWidth.setData(dI.m_asWidth);
		m_asCenter.setData(dI.m_asCenter);
		m_asStartIdx.setData(dI.m_vertexStart);
		//autosprite 2
		m_asHeight.setData(dI.m_asHeight);
		m_asMajorDir.setData(dI.m_asMajorDir);
	}

	bool Q3Shader::bind()
	{
		Common::ExpectFalse(m_objBound);

		using namespace Render;
		if (!m_gpuShader) //allow for no gpu shader?????
			return false;		
		if (m_shaderStages.empty()) //allow for empty stages?????
			return false;
				
		bool succes = true;		
		if (GL_NONE == m_cullFace)
			glDisable(GL_CULL_FACE);
		else
		{
			glEnable( GL_CULL_FACE);
			glCullFace( m_cullFace );
		}		
				
		succes &= applyBlend();
		succes &= m_gpuShader->bind();		

		//set all stages
		for (int i =0; i < m_shaderStages.size(); ++i) 
		{
			m_gpuShader->bind(SamplerNames[i].c_str(), m_shaderStages[i].getStageTexture()->getRawPointer());
			//stage.m_uniform.setData( stage.getStageTexture() );		
		}
		m_objBound = succes;
		return succes;
	}

	bool Q3Shader::unBind()
	{
		Common::ExpectTrue(m_objBound);		
		
		bool succes = true;	
		if (m_gpuShader)
			succes &= m_gpuShader->unBind();
		m_objBound = false;
		return succes;
		
	}

	void Q3Shader::beginLoad()
	{
		if (m_loaded)
			return;
		m_loaded = true;
		//load all the textures
		std::uint32_t flag = isSolid() ? 0 : FLAGS_ADD_ALPHA;
		for (auto& stage : m_shaderStages) {		
			m_loaded &= stage.loadTexture(0);
		}

		m_status = m_loaded ? App::RESOURCE_LOADED : App::RESOURCE_FAILURE;
		if (!m_status) {
			App::AddConsoleMessage( m_context, String( "Failed to load shader: " ) + m_name, App::LOG_LEVEL_WARNING);
			return;
		}
	}

	bool Q3Shader::applyBlend()
	{
		
		const auto* blendFun = &m_shaderStages[0].m_blendFunc[0];
		glBlendFunc(blendFun[0], blendFun[1]);
		return true;
	}

	bool Q3Shader::hasSurfaceFlag(std::uint32_t flag) const
	{
		return m_sufaceFlags & flag;
	}

	std::uint32_t Q3Shader::hasAlphaTest() const
	{
		std::uint32_t mask = 0;
		for(int idx = 0; idx < m_shaderStages.size(); ++idx )
		{
			if (eQ3AlphaFunc::NONE != m_shaderStages[idx].m_alphaFunc)
				mask |= 1 << idx;
		}
		return mask;
	}

	bool Q3Shader::hasSameDepthTest() const
	{
		if (m_shaderStages.empty())
			return true;
		for (int i = 1; i < m_shaderStages.size(); ++i)
			if (m_shaderStages[0].m_depthFunc != m_shaderStages[i].m_depthFunc)
				return false;
		return true;
	}

	bool Q3Shader::hasContentsFlag(std::uint32_t flag) const
	{
		return m_contents & flag;
	}

	Q3ShaderStage::Q3ShaderStage()
		: Q3ShaderStage(nullptr)
	{

	}

	Q3ShaderStage::Q3ShaderStage(App::EngineContext* context)
		: m_context(context)
	{
		reset();
	}

	void Q3ShaderStage::reset()
	{
		m_alphaFunc		= eQ3AlphaFunc::NONE;
		m_tcGen.m_tcGen = eQTcGen::BASE;
		m_depthFunc		= GL_LEQUAL;
		m_depthWrite	= true;
		m_clamp			= false;
		m_lightmap		= false;
		m_animated		= false;
		m_video			= false;
		m_hasAlphaMap	= false;
		m_animSpeed		= 0.0f;	
		m_blendFunc[0]	= GL_ONE;
		m_blendFunc[1]	= GL_ZERO;
		m_rgbaGen.reset();
		m_textures.clear();
		m_textureList.clear();
		m_texMods.clear();
	}


	bool Q3ShaderStage::loadTexture(const std::uint32_t flags )
	{
		auto resMan = m_context->getSystem<App::ResourceManager>();
		for (const auto& str : m_textures)
		{
			if (Common::StringEquals(str, "$whiteimage", true))
			{
				auto white = resMan->getResourceSafe<App::Texture>("DefaultWhiteTexture");
				m_textureList.push_back(white);
				continue;
			}

			auto resource = std::dynamic_pointer_cast<App::Texture>(resMan->getResource(str));
			if (!resource) //try to load a resource from path
			{
				int idx = TexturePathValid(str);
				bool resOk = false;
				if (idx != INVALID_INDEX) // texture with valid extension found, try to load it
				{
					resource = std::make_shared<App::Texture>(m_context);
					//flip tga images
					if (Common::EndsWith(ImageExtensions[idx], "tga"))
						resource->m_flipMode = Common::FLIPMASK_VERTICAL;
					if (m_clamp)
					{
						resource->m_params.m_clampMode_S = GL_CLAMP_TO_EDGE;
						resource->m_params.m_clampMode_T = GL_CLAMP_TO_EDGE;
					}
					if (FLAGS_ADD_ALPHA & flags)
					{
						resource->m_params.m_addAlphaChannel = true;
						resource->m_params.m_alphaValue      = 0;
					}

					resource->m_fromInternal = true;
					resource->setResourcePath(BASE_PATH + str + ImageExtensions[idx]); //add base path
					resource->setResourceName(str.c_str());
					resource->beginLoad();
					if (resource->getStatus() == App::RESOURCE_LOADED) 
					{
						resMan->addResource(resource, false);
						resOk = true;
					}
				}
				if (!resOk) //use fallback resource
				{
					App::AddConsoleMessage(m_context, String("Could not load texture: ") + str, App::LOG_LEVEL_WARNING);
					resource = resMan->getResourceSafe<App::Texture>("DefaultAlbedo");
				}
			}
			if (resource) {
				m_hasAlphaMap = resource->m_texture->getImageFormat().m_numChannels == 4;
				m_textureList.push_back(resource);
			}
		}
		return m_textureList.size() == m_textures.size();
	}



	const TexturePtr& Q3ShaderStage::getStageTexture() const
	{
		if (!m_animated)
			return m_textureList[0];

		//TODO merge into single texture then simply adjust uv coordinates
		auto time		= m_context->getProgramTime();
		auto  numFrames = static_cast<int>(m_textureList.size());
		auto  animIdx	= static_cast<int>(std::floor(time * m_animSpeed)) % numFrames;
		return m_textureList[animIdx];
	}


	Q3WaveForm::Q3WaveForm()
		: m_wavefunc(eQ3WaveFunc::NONE)
		, m_base(0.0f)
		, m_amp(0.0f)
		, m_phase(0.0f)
		, m_freq(0.0f)
	{

	}

	void Q3WaveForm::reset()
	{
		m_wavefunc	= eQ3WaveFunc::NONE;
		m_base		= 0.0f;
		m_amp		= 0.0f;
		m_phase		= 0.0f;
		m_freq		= 0.0f;
	}

	Q3TextureMod::Q3TextureMod()
		: m_tcMod(eQ3TcMod::NONE)
		, m_rotSpeed(0.0f)
	{

	}
	void Q3RgbaGen::reset()
	{
		m_rgbType = m_alphaType = eQ3RgbGen::IDENTITY;
		m_rgbWaveForm.reset();
		m_alphaWaveForm.reset();
	}

	void Q3VertexDeform::reset()
	{
		m_vertexDeform = eQ3VertexDeformFunc::NONE;
		m_waveForm.reset();
		m_dvDiv = 0;
		m_dvBulge = { 0.0f, 0.0f, 0.0f };
		m_dvMove = { 0.0f, 0.0f, 0.0f };
	}
	

	//QBSPLeaf DrawInfo
	Q3DrawInfo::Q3DrawInfo()
		: m_shaderId(INVALID_INDEX)
		, m_lightMapId(INVALID_INDEX)
		, m_leafId(INVALID_INDEX)
		, m_vertexStart(0)
		, m_vertexCount(0)
	{

	}

	Q3DrawInfo::Q3DrawInfo(int shader, int lightmap, int leaf, int start, int count, const Math::BBox3f& bounds)
		: m_shaderId(shader)
		, m_lightMapId(lightmap)
		, m_leafId(leaf)
		, m_vertexStart(start)
		, m_vertexCount(count)
		, m_bounds(bounds)


	{
		//m_autooSpritePos1 = bounds.getCenter();	
	}

	bool Q3DrawInfo::operator==(const Q3DrawInfo& rhs) const
	{
		return m_shaderId == rhs.m_shaderId &&
			m_lightMapId == rhs.m_lightMapId;
	}

	bool Q3DrawInfo::operator!=(const Q3DrawInfo& rhs) const
	{
		return !(*this == rhs);
	}


}