#pragma once
#include <Resource/IResource.hpp>
#include <Graphics/RenderUniforms.h>
#include <Misc/Q3BspTypes.h>
#include <App/AppTypeDefs.h>

namespace Misc
{
	class Q3Shader;
	using Q3ShaderPtr = std::shared_ptr<Q3Shader>;
	
	//////////////////////////////////////////////////////////////////////////
	//\Q3WaveForm
	//////////////////////////////////////////////////////////////////////////	

	struct Q3WaveForm
	{
		Q3WaveForm();

		void			reset();

		eQ3WaveFunc		m_wavefunc;
		float			m_base;
		float			m_amp;
		float			m_phase;
		float			m_freq;
	};

	//////////////////////////////////////////////////////////////////////////
	//\Q3TextureMod
	//////////////////////////////////////////////////////////////////////////	
	struct Q3TextureMod
	{

		Q3TextureMod();

		eQ3TcMod		m_tcMod;
		Q3WaveForm		m_waveForm;

		Math::Vector2f	m_scale;
		Math::Vector2f	m_scroll;

		//this forms a 2*3 matrix
		Math::Matrix2f	m_transform;
		Math::Vector2f	m_translation;

		float			m_rotSpeed;
	};

	struct Q3TcGen
	{
		Q3TcGen()
			: m_v1({ 1.0f, 0.0f, 0.0f })
			, m_v2({ 0.0f, 1.0f, 0.0f })
			, m_tcGen(eQTcGen::BASE)
		{

		}

		Math::Vector3f	m_v1,
						m_v2;

		eQTcGen			m_tcGen;
	};


	struct Q3SkyBox
	{
		String			m_nearBox;
		String			m_height;
		String			m_farBox;
	};


	struct Q3VertexDeform
	{
		Q3VertexDeform() : m_vertexDeform(eQ3VertexDeformFunc::NONE), m_dvDiv(0) {}

		void						reset();

		eQ3VertexDeformFunc			m_vertexDeform;
		float						m_dvDiv;
		Q3WaveForm					m_waveForm;
		Math::Vector3f				m_dvBulge;
		Math::Vector3f				m_dvMove;
		Math::Vector2f				m_dvNormal;
	};

	struct Q3RgbaGen
	{
		Q3RgbaGen()
			: m_rgbType(eQ3RgbGen::IDENTITY)
			, m_alphaType(eQ3RgbGen::IDENTITY)
		{

		}

		void reset();


		eQ3RgbGen       m_rgbType;
		Q3WaveForm		m_rgbWaveForm;

		eQ3RgbGen       m_alphaType;
		Q3WaveForm		m_alphaWaveForm;
	};


	/*
	@brief: Holds draw-information of the same shader type
	*/
	struct	Q3DrawInfo
	{

		Q3DrawInfo();

		Q3DrawInfo( int shader, int lightmap, int leaf, 
					int start, int count, const Math::BBox3f& bounds);


		bool		operator != (const Q3DrawInfo& rhs) const;
		bool		operator == (const Q3DrawInfo& rhs) const;

		int				m_shaderId;
		int				m_lightMapId;

		int				m_leafId;
		int				m_vertexStart;
		int				m_vertexCount;

		Math::BBox3f	 m_bounds;
		//auto sprite stuff
		Math::Vector3f	 m_asCenter;	   //autosprite center
		float			 m_asWidth;		   //autosprite width
		float			 m_asHeight;	   //autosprite2		
		Math::Vector3f	 m_asMajorDir;	   //autosprite2 direction
		Math::Vector3f	 m_asVertexPos[2]; //autosprite2
	};


	
	struct Q3ShaderStage
	{
		Q3ShaderStage();
		explicit Q3ShaderStage(App::EngineContext* context);

		void			reset();
		/*
			@brief: Load texture for this stage, 
			TODO stage shouldn't be aware of engine context
		*/
		bool							loadTexture( const std::uint32_t flags = 0 );

		const TexturePtr&				getStageTexture() const;

		App::EngineContext*	 m_context;

		
		eQ3AlphaFunc					m_alphaFunc;
		int								m_depthFunc;
		bool							m_depthWrite;
		bool							m_clamp;
		bool							m_lightmap;
		bool							m_animated;
		bool							m_video;
		float							m_animSpeed;
		bool							m_hasAlphaMap;
		int								m_blendFunc[2];

		std::vector<Q3TextureMod>		m_texMods;
		Q3TcGen							m_tcGen;
		Q3RgbaGen						m_rgbaGen;	
		StringList 						m_textures;
		//HwUniform<TexturePtr>			m_uniform;
		TexturePtrVector						m_textureList;
	};


	class Q3Shader : public App::Resource
	{
	public:

		struct DrawShader
		{
			ShaderPtr					 m_gpuShader;
			int						     m_stageStart;
			int							 m_stageCount;
		};

		Q3Shader() : Q3Shader(nullptr) {}
		explicit Q3Shader(App::EngineContext* context) : Q3Shader(context, "", "") {}
		explicit Q3Shader(App::EngineContext* context, const String& fileName, const String& shadName);
	
		bool						unloadShader();
		
		/*
		* @brief: Returns if this shader is a default shader -> 2 textures( lightmap & albedo ), no texture/vertex mods
		*/
		bool						isDefault() const;

		/*
		*@brief: Returns true if this shader is solid( in opengl terms -->
		* no blending for the first stage )
		*/
		bool						isSolid() const;

		/*
			@brief: Returns if this shader requires multi pass rendering e.g.
			blending in first pass and later stages have alpha testing,
			or multiple alpha tested surfaces
		*/
		bool						isMultiPass() const;


		/*
			@brief: Has this shader any vertex deform
		*/
		bool						hasVertexDeform() const;
		
		/*
			@brief: Has this shader any autosprite deform
		*/
		std::uint32_t						hasAutoSprite() const;
	
		/*
			@brief: returns true if this shader has a flag
		*/
		bool						hasSurfaceFlag(std::uint32_t flag) const;

		/*
		@brief: returns true if this shader has a flag
		*/
		bool						hasContentsFlag(std::uint32_t flag) const;

		/*
			@brief: Returns the mask(S) for stages that have alpha tests
		*/
		std::uint32_t						hasAlphaTest() const;

		/*
			@brief: Returns if all the stages have the same depth test
		*/
		bool						hasSameDepthTest() const;		

		/*
			@brief: Apply blending
		*/
		bool						applyBlend();
		
		/*
		*@brief: Generate GLSL shaders
		*/
		bool						generateGLSL();


		static Q3ShaderPtr			CreateRegularShader( App::EngineContext* context, const String& name );
		static Q3ShaderPtr			CreateFallBackShader( App::EngineContext* context );

		void						setDrawInfo		( const Q3DrawInfo& dI );

		bool						bind()			override;
		bool						unBind()		override;

		virtual void                beginLoad()		override;
		virtual void                reload()		override {};                               
		virtual void                endLoad()		override {};                       
		virtual void                release()		override {};
		
		bool					    m_transparent;
		bool						m_loaded;
		bool						m_mipmaps;
		std::uint32_t						m_contents;
		std::uint32_t						m_sufaceFlags;
		int							m_cullFace;
		int							m_tessSize;
		int							m_sort;

		bool						m_polyOffset;
		float						m_light;

		Math::Vector3f				m_fogParams;
		float						m_fogOpacity;

		String						m_path;
		String						m_name;

		std::vector<Q3ShaderStage>	m_shaderStages;
		std::vector<Q3VertexDeform>	m_vertexDeform;
		Q3SkyBox					m_skyBox;
		ShaderPtr					m_gpuShader;

		//unifroms

		HwUniform<int>				m_useSkyBox; //use skybox matrix
		HwUniform<Math::Matrix4f>	m_skyMatrix;

		//autosprite uniforms
		HwUniform<int>				m_asStartIdx;
		HwUniform<Math::Vector3f>	m_asCenter;
		HwUniform<float>			m_asWidth;
		HwUniform<float>			m_asHeight;		
		HwUniform<Math::Vector3f>	m_asMajorDir;	
	};


	class Q3ParseShader
	{
	public:
		friend class Q3BspFile;
		explicit Q3ParseShader(App::EngineContext* context, const String& fileName);


		bool					parseShaderFile();
		bool					parseShaderStage(Q3ShaderStage& curStage, Q3ShaderPtr shader);
		bool					parseShaderLocal(Q3ShaderPtr curShader);
		void					printError( const String& msg, const String& optional = "" ) const;


		/*
		* @brief: Go back 1 place
		*/
		void					ungetChar();

		/*
		* @brief: eat character return it
		*/
		char					eatChar();

		/*
		* @brief: peek next character without eating it
		*/
		char					peekChar() const;

		/*
		* @brief: Peek character ignore whitespace
		*/
		char					peekCharWS();


		/*
		* @brief: Skip pointer to next line, returns remaining line
		*/
		String					skipToNewLine();

		/*
		*@brief: return true if this character is whitespace,
		*counts number of lines
		*/
		bool					isWhiteSpace(char val);

		/*
		* @brief: parse token, assumes no whitespace
		*/
		bool					parseToken(String& result);
		
		/*
		* @brief: parse token, eats any whitespace before it
		*/
		bool					getToken(String& result);
		
		/*
		* @brief: peek next token, doesnt adjust the data ptr
		*/
		bool					peekToken(String& result);

		/*
		* @brief: Various parsing functions
		*/
		bool					parseVertexDeform(Q3VertexDeform& result);
		bool					parseFloat(float& result);
	
		bool					parseInt(int& result);
		bool					parseVec3(Math::Vector3f& result);
		bool					parseWave(Q3WaveForm& wave);
		bool					parseSkybox(Q3SkyBox& box);
		bool					parseDepthFunc(Q3ShaderStage& shaderStage);
		bool					parseBlendFunc(Q3ShaderStage& shaderStage);
		bool					parseTcGen(Q3TcGen& tcGen);
		bool					parseTcMod(Q3TextureMod& tcMod);
		bool					parseCull(int& result);
		bool					parseFogParam(Math::Vector3f& fogColor, float& fogOpacity);
		bool					parseAlphaFunc(Q3ShaderStage& shaderStage);
		bool					parseAnimMap(Q3ShaderStage& shaderStage);
		bool					parseSurfaceParam(std::uint32_t& surface, std::uint32_t& contents);
		bool					parseRGBAGen(Q3RgbaGen& val, bool isAlpha);
		bool					parseSort(int& result);

	private:
		App::EngineContext*			m_context;

		String						m_fileName;
		String						m_fileData;
		const char*					m_dataPtr;
		int							m_lineNumber;
		std::vector<Q3ShaderPtr>	m_shaders;

	};
}