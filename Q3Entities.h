#pragma once

#include <map>

#include <QtCore/QDebug>
#include <Common/Common.h>
#include <Math/AABB.h>
#include <Math/GenMath.h>
#include <Events/EventSystem.hpp>
#include <App/AppTypeDefs.h>
#include <Engine/EngineContext.hpp>


namespace Misc
{
	class Q3Entity;
	using Q3EntityPtr	= std::shared_ptr<Q3Entity>;

	class Q3Entity : public EngineObject
	{

	public:

		Q3Entity(ContextPointer engine);



		virtual ~Q3Entity() {};

		void				beginFrame(App::Event& evt);
		virtual void		update(float timeStep);
				
		
		Math::Vector3f		m_origin		= { 0.0f, 0.0f, 0.0f };
		Math::Vector3f		m_direction		= { 1.0f, 0.0f, 0.0f };	 //from 'angle' field
		Math::Vector3f		m_direction2	= { 0.0f, 0.0f, -1.0f }; //from target -> targetName
		Math::Vector3f		m_color			= { 1.0f, 1.0f, 1.0f };

		


			
		float				m_wait			= 0.0f; //delay before triggering
		float				m_brightess		= 0.0f;	//light brightness
		float				m_light			= 0.0f; //light lightness
		float				m_random		= 0.0f;	//random variance
		float				m_speed			= 0.0f;	//rotation speed
		float				m_dmg			= 0.0f; //trigger hurt damage
		float				m_health		= 0.0f; //gives health
		float				m_armor			= 0.0f; //gives armor
		float				m_respawnTime   = 0.0f; //respawn time
		float				m_range			= 0.0f; //info camp
		float				m_weight		= 0.0f; //info camp
		float				m_roll			= 0.0f; //camera teleport
		float				m_radius		= 0.0f; //light
		float				m_minLight		= 0.0f; //door
		float				m_delay			= 0.0;  //add delay before triggering targets
		float				m_lighty		= 0.0f; //light
		
		float				m_height		= 0.0f; //func bobbing
		float				m_phase			= 0.0f; //pendulum
		float				m_visible		= 1.0f;
		float				m_angle			= 0.0f;

		//portal camera rotation
		float				m_portalMinMaxRotY		= 5.0f;
		float				m_portalCurRotY			= 0.0f;
		float				m_portalCamRotYSpeed	= 2.0f; //degrees per second

		int					m_notFree		= 0;
		int					m_notTeam		= 0;
		int					m_lip			= 0;  //door
		int					m_noBots		= 0;
		
		int					m_ambient		= 0;
		int					m_count			= 0; //armor shard ?

		int					m_slotId		= 0;  //id into global entity list

		float				m_totalTime		= 0.0f;
		int					m_frameNumber   = 0;	
		int					m_clusterId		= -1;		

		int				m_type			= 0;
		int				m_spawnFlags	= 0;
		int				m_style			= 0; //light?

		Math::BBox3f		m_bounds;		//local bounds -> worldbounds will be set from this & entity position
		String				m_target;		
		String				m_targetName;
		String				m_model;
		String				m_model2;
		String				m_noTeam;
		String				m_team;
		String				m_message; //worldspawn
		String				m_noise; //target speaker
		String				m_music;
		
		std::vector<int>			m_linkedEntities; 
		std::map<String, String>	m_keyValue; //all the key value pairs for this entity
	};


	
	//////////////////////////////////////////////////////////////////////////
	//\brief: Misc Entities
	//////////////////////////////////////////////////////////////////////////
	class Q3MiscPortalCamera : public Q3Entity
	{
	public:
		Q3MiscPortalCamera(ContextPointer engine) : Q3Entity(engine) {}

		bool					m_mirror = false;
		float					m_sign   = 1.0f;

		virtual	void			update( float timeStep ) override{
			//add  camera 'roll'			
			float maxRot = m_portalMinMaxRotY;
			if (m_portalCurRotY <= -maxRot)
				m_sign = 1.0f;
			if (m_portalCurRotY >= maxRot)
				m_sign = -1.0f;
			
			m_portalCurRotY += (m_portalCamRotYSpeed * m_sign) * timeStep;
			Q3Entity::update(timeStep);
		}
	};

	class Q3MiscPortalSurface : public Q3Entity
	{
	public:
		
		Q3MiscPortalSurface(ContextPointer engine) : Q3Entity(engine) {}
		/*
			@brief A portal surface is considered a mirror if
			there is no target camera location
		*/
		bool isMirror() const 
		{
			return m_linkedEntities.empty();
		}

		/*
			@brief: Return the index of the camera target locatoin
		*/
		int	getPortalCameraIndex() const 
		{
			if (m_linkedEntities.size() != 1)
				throw std::exception("Invalid portal camera link");
			return m_linkedEntities[0];
		}		
	};


	//////////////////////////////////////////////////////////////////////////
	// \Misc Entities
	//////////////////////////////////////////////////////////////////////////
	class Q3WorldSpawn : public Q3Entity
	{
	public:
		Q3WorldSpawn(ContextPointer engine) : Q3Entity(engine) {}
	};

	class Q3ShooterGrenade : public Q3Entity
	{
	public:
		Q3ShooterGrenade(ContextPointer engine) : Q3Entity(engine) {}
	};

	class Q3MiscTeleporterDest : public Q3Entity
	{
	public:
		Q3MiscTeleporterDest(ContextPointer engine) : Q3Entity(engine) {}
	};

	class Q3MiscModel : public Q3Entity
	{
	public:
		Q3MiscModel(ContextPointer engine) : Q3Entity(engine) {}
	};

	//////////////////////////////////////////////////////////////////////////
	//\brief: Info Entities
	//////////////////////////////////////////////////////////////////////////
	class Q3InfoPlayerIntermission : public Q3Entity
	{
	public:
		Q3InfoPlayerIntermission(ContextPointer engine) : Q3Entity(engine) {}
	};

	class Q3InfoPlayerStart : public Q3Entity
	{
	public:
		Q3InfoPlayerStart(ContextPointer engine) : Q3Entity(engine) {}
	};

	class Q3InfoNull : public Q3Entity
	{
	public:
		Q3InfoNull(ContextPointer engine) : Q3Entity(engine) {}
	};

	class Q3InfoNotNull : public Q3Entity
	{
	public:
		Q3InfoNotNull(ContextPointer engine) : Q3Entity(engine) {}
	};

	class Q3InfoFirstPlace : public Q3Entity
	{
	public:
		Q3InfoFirstPlace(ContextPointer engine) : Q3Entity(engine) {}
	};

	class Q3InfoSecondPlace : public Q3Entity
	{
	public:
		Q3InfoSecondPlace(ContextPointer engine) : Q3Entity(engine) {}
	};

	class Q3InfoThirdPlace : public Q3Entity
	{
	public:
		Q3InfoThirdPlace(ContextPointer engine) : Q3Entity(engine) {}
	};

	class Q3InfoSpecatorStart : public Q3Entity
	{
	public:
		Q3InfoSpecatorStart(ContextPointer engine) : Q3Entity(engine) {}
	};
	class Q3InfoCamp : public Q3Entity
	{
	public:
		Q3InfoCamp(ContextPointer engine) : Q3Entity(engine) {}
	};




	
	//////////////////////////////////////////////////////////////////////////
	//\brief: Light Entities
	//////////////////////////////////////////////////////////////////////////
	class Q3Light : public Q3Entity
	{		
	public:
		Q3Light(ContextPointer engine) : Q3Entity(engine) {}
	};

	//////////////////////////////////////////////////////////////////////////
	//\brief: Func Enttites
	//////////////////////////////////////////////////////////////////////////
	class Q3FuncRotating : public Q3Entity
	{
	public:
		Q3FuncRotating(ContextPointer engine) : Q3Entity(engine) {}
	};

	class Q3FuncStatic : public Q3Entity
	{
	public:
		Q3FuncStatic(ContextPointer engine) : Q3Entity(engine) {}
	};
	

	class Q3FuncTimer : public Q3Entity
	{		
	public:
		Q3FuncTimer(ContextPointer engine) : Q3Entity(engine) {}
	};

	class Q3FuncDoor : public Q3Entity
	{			
	public:
		Q3FuncDoor(ContextPointer engine) : Q3Entity(engine) {}
	};

	class Q3FuncButton : public Q3Entity
	{
	public:
		Q3FuncButton(ContextPointer engine) : Q3Entity(engine) {}
	};

	class Q3FuncBobbing : public Q3Entity
	{
	public:
		Q3FuncBobbing(ContextPointer engine) : Q3Entity(engine) {}
	};

	class Q3FuncPendulum : public Q3Entity
	{
	public:
		Q3FuncPendulum(ContextPointer engine) : Q3Entity(engine) {}
	};

	//////////////////////////////////////////////////////////////////////////
	//\brief: Target Enttites
	//////////////////////////////////////////////////////////////////////////
	class Q3TargetSpeaker : public Q3Entity
	{
	public:
		Q3TargetSpeaker(ContextPointer engine) : Q3Entity(engine) {}
	};

	class Q3TargetRelay : public Q3Entity
	{			
	public:
		Q3TargetRelay(ContextPointer engine) : Q3Entity(engine) {}
	};

	class Q3TargetDelay : public Q3Entity
	{
	public:
		Q3TargetDelay(ContextPointer engine) : Q3Entity(engine) {}
	};
	
	class Q3TargetPosition : public Q3Entity
	{
	public:
		Q3TargetPosition(ContextPointer engine) : Q3Entity(engine) {}
	};

	class Q3TargetGive : public Q3Entity
	{
	public:
		Q3TargetGive(ContextPointer engine) : Q3Entity(engine) {}
	};

	class Q3TargetLocation : public Q3Entity
	{
	public:
		Q3TargetLocation(ContextPointer engine) : Q3Entity(engine) {}
	};	

	class Q3TargetRemovePowerUps : public Q3Entity
	{
	public:
		Q3TargetRemovePowerUps(ContextPointer engine) : Q3Entity(engine) {}
	};

	class Q3TargetPrint : public Q3Entity
	{
	public:
		Q3TargetPrint(ContextPointer engine) : Q3Entity(engine) {}
	};
	
	//////////////////////////////////////////////////////////////////////////
	// \Triggers brush based
	//////////////////////////////////////////////////////////////////////////
	class Q3TriggerHurt : public Q3Entity
	{		
	public:
		Q3TriggerHurt(ContextPointer engine) : Q3Entity(engine) {}
	};

	class Q3TriggerPush : public Q3Entity
	{		
	public:
		Q3TriggerPush(ContextPointer engine) : Q3Entity(engine) {}
	};

	class Q3TriggerTeleport : public Q3Entity
	{
	public:
		Q3TriggerTeleport(ContextPointer engine) : Q3Entity(engine) {}
	};

	class Q3TriggerMultiple : public Q3Entity
	{
	public:
		Q3TriggerMultiple(ContextPointer engine) : Q3Entity(engine) {}
	};

	class Q3TriggerAlways : public Q3Entity
	{
	public:
		Q3TriggerAlways(ContextPointer engine) : Q3Entity(engine) {}
	};	

	//////////////////////////////////////////////////////////////////////////
	//\brief: Armor Enttites
	//////////////////////////////////////////////////////////////////////////
	class Q3ItemArmorShard : public Q3Entity
	{
	public:
		Q3ItemArmorShard(ContextPointer engine) : Q3Entity(engine) {}
	};

	class Q3ItemArmorBody : public Q3Entity
	{
	public:
		Q3ItemArmorBody(ContextPointer engine) : Q3Entity(engine) {}
	};
	
	class Q3ItemArmorCombat: public Q3Entity
	{
	public:
		Q3ItemArmorCombat(ContextPointer engine) : Q3Entity(engine) {}
	};

	//////////////////////////////////////////////////////////////////////////
	//\brief: Powerup Entities
	//////////////////////////////////////////////////////////////////////////
	class Q3HoldableTeleporter : public Q3Entity
	{
	public:
		Q3HoldableTeleporter(ContextPointer engine) : Q3Entity(engine) {}
	};

	class Q3HoldableMedkit : public Q3Entity
	{
	public:
		Q3HoldableMedkit(ContextPointer engine) : Q3Entity(engine) {}
	};

	class Q3ItemQuad : public Q3Entity
	{
	public:
		Q3ItemQuad(ContextPointer engine) : Q3Entity(engine) {}
	};

	class Q3ItemEnviro : public Q3Entity
	{
	public:
		Q3ItemEnviro(ContextPointer engine) : Q3Entity(engine) {}
	};

	class Q3ItemInvis : public Q3Entity
	{
	public:
		Q3ItemInvis(ContextPointer engine) : Q3Entity(engine) {}
	};

	class Q3ItemHaste : public Q3Entity
	{
	public:
		Q3ItemHaste(ContextPointer engine) : Q3Entity(engine) {}
	};

	class Q3ItemRegen : public Q3Entity
	{
	public:
		Q3ItemRegen(ContextPointer engine) : Q3Entity(engine) {}
	};

	class Q3ItemFlight : public Q3Entity
	{
	public:
		Q3ItemFlight(ContextPointer engine) : Q3Entity(engine) {}
	};

	//////////////////////////////////////////////////////////////////////////
	//\brief: Item Health Enttites
	//////////////////////////////////////////////////////////////////////////
	class Q3ItemHealthSmall : public Q3Entity
	{
	public:
		Q3ItemHealthSmall(ContextPointer engine) : Q3Entity(engine) {}
	};

	class Q3ItemHealth : public Q3Entity
	{
	public:
		Q3ItemHealth(ContextPointer engine) : Q3Entity(engine) {}
	};

	class Q3ItemHealthLarge : public Q3Entity
	{
	public:
		Q3ItemHealthLarge(ContextPointer engine) : Q3Entity(engine) {}
	};

	class Q3ItemHealthMega : public Q3Entity
	{
	public:
		Q3ItemHealthMega(ContextPointer engine) : Q3Entity(engine) {}
	};

	//////////////////////////////////////////////////////////////////////////
	//\brief: Item Weapons Enttites
	//////////////////////////////////////////////////////////////////////////
	class Q3WeaponGauntlet : public Q3Entity
	{
	public:
		Q3WeaponGauntlet(ContextPointer engine) : Q3Entity(engine) {}
	};

	class Q3WeaponMachineGun : public Q3Entity
	{
	public:
		Q3WeaponMachineGun(ContextPointer engine) : Q3Entity(engine) {}
	};

	class Q3WeaponShotGun : public Q3Entity
	{
	public:
		Q3WeaponShotGun(ContextPointer engine) : Q3Entity(engine) {}
	};
	
	class Q3WeaponGrenadeLauncher : public Q3Entity
	{
	public:
		Q3WeaponGrenadeLauncher(ContextPointer engine) : Q3Entity(engine) {}
	};

	class Q3WeaponRocketLauncher : public Q3Entity
	{
	public:
		Q3WeaponRocketLauncher(ContextPointer engine) : Q3Entity(engine) {}
	};

	class Q3WeaponLightning : public Q3Entity
	{
	public:
		Q3WeaponLightning(ContextPointer engine) : Q3Entity(engine) {}
	};

	class Q3WeaponRailGun : public Q3Entity
	{
	public:
		Q3WeaponRailGun(ContextPointer engine) : Q3Entity(engine) {}
	};
	
	class Q3WeaponPlasmaGun : public Q3Entity
	{
	public:
		Q3WeaponPlasmaGun(ContextPointer engine) : Q3Entity(engine) {}
	};

	class Q3WeaponBFG : public Q3Entity
	{
	public:
		Q3WeaponBFG(ContextPointer engine) : Q3Entity(engine) {}
	};

	//////////////////////////////////////////////////////////////////////////
	//\brief: Ammo Enttites
	//////////////////////////////////////////////////////////////////////////
	class Q3AmmoBullets : public Q3Entity
	{
	public:
		Q3AmmoBullets(ContextPointer engine) : Q3Entity(engine) {}
	};
	class Q3AmmoShells : public Q3Entity
	{
	public:
		Q3AmmoShells(ContextPointer engine) : Q3Entity(engine) {}
	};
	class Q3AmmoGrenades : public Q3Entity
	{
	public:
		Q3AmmoGrenades(ContextPointer engine) : Q3Entity(engine) {}
	};

	class Q3AmmoRockets : public Q3Entity
	{
	public:
		Q3AmmoRockets(ContextPointer engine) : Q3Entity(engine) {}
	};

	class Q3AmmoLightning : public Q3Entity
	{
	public:
		Q3AmmoLightning(ContextPointer engine) : Q3Entity(engine) {}
	};

	class Q3AmmoSlugs : public Q3Entity
	{
	public:
		Q3AmmoSlugs(ContextPointer engine) : Q3Entity(engine) {}
	};

	class Q3AmmoCells : public Q3Entity
	{
	public:
		Q3AmmoCells(ContextPointer engine) : Q3Entity(engine) {}
	};

	class Q3AmmoBFG : public Q3Entity
	{
	public:
		Q3AmmoBFG(ContextPointer engine) : Q3Entity(engine) {}
	};	



	/*
		@brief: Convert key value pairs to basic types
	*/
	inline Q3EntityPtr	parseGeneric( std::map<String, String> kv, const std::shared_ptr<Q3Entity>& entity)
	{
		
		using namespace Common;
		using namespace Math;	

		//some handy lambda's
		auto angleToDir = [](float angle)
		{
			auto angleRad = Math::ToRadians(angle);
			return Vector3f(cosf(angleRad), sinf(angleRad), 0.0f);
		};
		auto hasKey = [](const std::map<String, String>& kv, const String& key)
		{
			return kv.find(key) != std::end(kv); 
		};
		
		auto delim = String(" ");
		auto succeed = true;

		//vec3 based vlues
		if (hasKey(kv, "origin"))
			entity->m_origin = ParseVector3(kv.at("origin"), delim, succeed);
		if (hasKey(kv, "_color"))
			entity->m_color = ParseVector3(kv.at("_color"), delim, succeed);

		//int based values
		if (hasKey(kv, "spawnflags"))
			entity->m_spawnFlags = ParseInt(kv.at("spawnflags"), succeed);
		if (hasKey(kv, "notfree"))
			entity->m_notFree = ParseInt(kv.at("notfree"), succeed);
		if (hasKey(kv, "nobots"))
			entity->m_noBots = ParseInt(kv.at("nobots"), succeed);
		if (hasKey(kv, "ambient"))
			entity->m_ambient = ParseInt(kv.at("ambient"), succeed);
		if (hasKey(kv, "notteam"))
			entity->m_notTeam = ParseInt(kv.at("notteam"), succeed);
		if (hasKey(kv, "style"))
			entity->m_weight = ParseInt(kv.at("style"), succeed);
		if (hasKey(kv, "count"))
			entity->m_count = ParseInt(kv.at("count"), succeed);

		//float based values
		if (hasKey(kv, "angle")) {
			entity->m_angle     = ParseFloat(kv.at("angle"), succeed);
			entity->m_direction = angleToDir( entity->m_angle );
		}
		if (hasKey(kv, "wait"))
			entity->m_wait = ParseFloat(kv.at("wait"), succeed);
		if (hasKey(kv, "light"))
			entity->m_light = ParseFloat(kv.at("light"), succeed);
		if (hasKey(kv, "lighty"))
			entity->m_lighty = ParseFloat(kv.at("lighty"), succeed);
		if (hasKey(kv, "speed"))
			entity->m_speed = ParseFloat(kv.at("speed"), succeed);
		if (hasKey(kv, "random"))
			entity->m_random = ParseFloat(kv.at("random"), succeed);
		if (hasKey(kv, "lip"))
			entity->m_lip = ParseFloat(kv.at("lip"), succeed);
		if (hasKey(kv, "dmg"))
			entity->m_dmg = ParseFloat(kv.at("dmg"), succeed);
		if (hasKey(kv, "health"))
			entity->m_health = ParseFloat(kv.at("health"), succeed);
		if (hasKey(kv, "roll"))
			entity->m_roll = ParseFloat(kv.at("roll"), succeed);
		if (hasKey(kv, "_minlight"))
			entity->m_minLight = ParseFloat(kv.at("_minlight"), succeed);
		if (hasKey(kv, "radius"))
			entity->m_radius = ParseFloat(kv.at("radius"), succeed);
		if (hasKey(kv, "range"))
			entity->m_range = ParseFloat(kv.at("range"), succeed);
		if (hasKey(kv, "weight"))
			entity->m_weight = ParseFloat(kv.at("weight"), succeed);
		if (hasKey(kv, "delay"))
			entity->m_delay = ParseFloat(kv.at("delay"), succeed);
		if (hasKey(kv, "height"))
			entity->m_height = ParseFloat(kv.at("height"), succeed);
		if (hasKey(kv, "phase"))
			entity->m_phase = ParseFloat(kv.at("phase"), succeed);

		//string based values
		if (hasKey(kv, "message"))
			entity->m_message = kv.at("message");
		if (hasKey(kv, "target"))
			entity->m_target = kv.at("target");
		if (hasKey(kv, "targetname"))
			entity->m_targetName = kv.at("targetname");
		if (hasKey(kv, "model"))
			entity->m_model = kv.at("model");
		if (hasKey(kv, "noteam"))
			entity->m_noTeam = kv.at("noteam");
		if (hasKey(kv, "team"))
			entity->m_team = kv.at("team");
		if (hasKey(kv, "model"))
			entity->m_model = kv.at("model");
		if (hasKey(kv, "noise"))
			entity->m_noise = kv.at("noise");
		if (hasKey(kv, "model2"))
			entity->m_model2 = kv.at("model2");
		if (hasKey(kv, "music"))
			entity->m_music = kv.at("music");
		return entity;
	}


	/*
		Simple 'object factory', in need of refactoring
	*/
	inline Q3EntityPtr  EntityForClassName(const std::map<String, String>& keyValues, ContextPointer engine )
	{
		
		const auto& strEq = Common::StringEquals; //function alias
		auto className = keyValues.at("classname");

		//////////////////////////////////////////////////////////////////////////
		// \brief: Info Entities
		//////////////////////////////////////////////////////////////////////////
		if (strEq(className, "info_player_deathmatch", true))
		{
			auto entity = std::make_shared<Q3InfoPlayerStart>(engine);
			return parseGeneric(keyValues, entity);		
		}
		if (strEq(className, "info_player_start", true))
		{
			auto entity = std::make_shared<Q3InfoPlayerStart>(engine);
			return parseGeneric(keyValues, entity);			
		}
		if (strEq(className, "info_player_intermission", true))
		{
			auto entity = std::make_shared<Q3InfoPlayerIntermission>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "info_firstplace", true))
		{
			auto entity = std::make_shared<Q3InfoFirstPlace>(engine);
			return parseGeneric(keyValues, entity);
		}		
		if (strEq(className, "info_secondplace", true))
		{
			auto entity = std::make_shared<Q3InfoSecondPlace>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "info_thirdplace", true))
		{
			auto entity = std::make_shared<Q3InfoThirdPlace>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "info_notnull", true))
		{
			auto entity = std::make_shared<Q3InfoNotNull>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "info_null", true))
		{
			auto entity = std::make_shared<Q3InfoNull>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "info_notnull", true))
		{
			auto entity = std::make_shared<Q3InfoNotNull>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "info_spectator_start", true))
		{
			auto entity = std::make_shared<Q3InfoSpecatorStart>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "info_camp", true))
		{
			auto entity = std::make_shared<Q3InfoCamp>(engine);
			return parseGeneric(keyValues, entity);
		}
		
		//////////////////////////////////////////////////////////////////////////
		// \brief: Misc Entities
		//////////////////////////////////////////////////////////////////////////
		if (strEq(className, "misc_portal_surface", true)) {
			auto entity = std::make_shared<Q3MiscPortalSurface>(engine);
			return parseGeneric(keyValues, entity);			
		}
		if (strEq(className, "misc_portal_camera", true)){
			auto entity = std::make_shared<Q3MiscPortalCamera>(engine);
			return parseGeneric(keyValues, entity);			
		}
		if (strEq(className, "misc_model", true)){
			auto entity = std::make_shared<Q3MiscModel>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "misc_teleporter_dest", true)) {
			auto entity = std::make_shared<Q3MiscTeleporterDest>(engine);
			return parseGeneric(keyValues, entity);
		}			
		//////////////////////////////////////////////////////////////////////////
		//\brief: Func Entities
		//////////////////////////////////////////////////////////////////////////	
		if (strEq(className, "func_rotating", true))
		{
			auto entity = std::make_shared<Q3FuncRotating>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "func_door", true))
		{
			auto entity = std::make_shared<Q3FuncDoor>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "func_bobbing", true))
		{
			auto entity = std::make_shared<Q3FuncBobbing>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "func_timer", true))
		{
			auto entity = std::make_shared<Q3FuncTimer>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "trigger_hurt", true))
		{
			auto entity = std::make_shared<Q3TriggerHurt>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "func_button", true))
		{
			auto entity = std::make_shared<Q3FuncButton>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "func_static", true))
		{
			auto entity = std::make_shared<Q3FuncStatic>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "func_pendulum", true))
		{
			auto entity = std::make_shared<Q3FuncPendulum>(engine);
			return parseGeneric(keyValues, entity);
		}
		//////////////////////////////////////////////////////////////////////////
		//\brief: Target Entities
		//////////////////////////////////////////////////////////////////////////	
		if (strEq(className, "target_print", true))
		{
			auto entity = std::make_shared<Q3TargetPrint>(engine);
			return 	parseGeneric(keyValues, entity);
		}
		if (strEq(className, "trigger_push", true))
		{
			auto entity = std::make_shared<Q3TriggerPush>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "trigger_teleport", true))
		{
			auto entity = std::make_shared<Q3TriggerTeleport>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "target_relay", true))
		{
			auto entity = std::make_shared<Q3TargetRelay>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "target_delay", true))
		{
			auto entity = std::make_shared<Q3TargetDelay>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "target_speaker", true))
		{
			auto entity = std::make_shared<Q3TargetSpeaker>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "target_position", true))
		{
			auto entity = std::make_shared<Q3TargetPosition>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "target_location", true))
		{
			auto entity = std::make_shared<Q3TargetLocation>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "target_remove_powerups", true))
		{
			auto entity = std::make_shared<Q3TargetRemovePowerUps>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "target_give", true))
		{
			auto entity = std::make_shared<Q3TargetGive>(engine);
			return parseGeneric(keyValues, entity);
		}

		//////////////////////////////////////////////////////////////////////////
		//\brief: Trigger entities
		//////////////////////////////////////////////////////////////////////////
		if (strEq(className, "trigger_multiple", true))
		{
			auto entity = std::make_shared<Q3TriggerMultiple>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "trigger_always", true))
		{
			auto entity = std::make_shared<Q3TriggerAlways>(engine);
			return parseGeneric(keyValues, entity);
		}
		
		//////////////////////////////////////////////////////////////////////////
		//\brief: Armor entities
		//////////////////////////////////////////////////////////////////////////
		if (strEq(className, "item_armor_shard", true))
		{
			auto entity = std::make_shared<Q3ItemArmorShard>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "item_armor_body", true))
		{
			auto entity = std::make_shared<Q3ItemArmorBody>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "item_armor_combat", true))
		{
			auto entity = std::make_shared<Q3ItemArmorCombat>(engine);
			return parseGeneric(keyValues, entity);
		}
		
		//////////////////////////////////////////////////////////////////////////
		//\brief: Health entities
		//////////////////////////////////////////////////////////////////////////
		if (strEq(className, "item_health_small", true))
		{
			auto entity = std::make_shared<Q3ItemHealthSmall>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "item_health", true))
		{
			auto entity = std::make_shared<Q3ItemHealth>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "item_health_large", true))
		{
			auto entity = std::make_shared<Q3ItemHealthLarge>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "item_health_mega", true))
		{
			auto entity = std::make_shared<Q3ItemHealthMega>(engine);
			return parseGeneric(keyValues, entity);
		}
		//////////////////////////////////////////////////////////////////////////
		//\brief: Weapon entities
		//////////////////////////////////////////////////////////////////////////
		if (strEq(className, "weapon_gauntlet", true))
		{
			auto entity = std::make_shared<Q3WeaponGauntlet>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "weapon_machinegun", true))
		{
			auto entity = std::make_shared<Q3WeaponMachineGun>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "weapon_shotgun", true))
		{
			auto entity = std::make_shared<Q3WeaponShotGun>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "weapon_grenadelauncher", true))
		{
			auto entity = std::make_shared<Q3WeaponGrenadeLauncher>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "weapon_rocketlauncher", true))
		{
			auto entity = std::make_shared<Q3WeaponRocketLauncher>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "weapon_lightning", true))
		{
			auto entity = std::make_shared<Q3WeaponLightning>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "weapon_railgun", true))
		{
			auto entity = std::make_shared<Q3WeaponRailGun>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "weapon_plasmagun", true))
		{
			auto entity = std::make_shared<Q3WeaponPlasmaGun>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "weapon_bfg", true))
		{
			auto entity = std::make_shared<Q3WeaponBFG>(engine);
			return parseGeneric(keyValues, entity);
		}
		//////////////////////////////////////////////////////////////////////////
		//\brief: Ammo entities
		//////////////////////////////////////////////////////////////////////////
		if (strEq(className, "ammo_bullets", true))
		{
			auto entity = std::make_shared<Q3AmmoBullets>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "ammo_shells", true))
		{
			auto entity = std::make_shared<Q3AmmoShells>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "ammo_grenades", true))
		{
			auto entity = std::make_shared<Q3AmmoGrenades>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "ammo_rockets", true))
		{
			auto entity = std::make_shared<Q3AmmoRockets>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "ammo_lightning", true))
		{
			auto entity = std::make_shared<Q3AmmoLightning>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "ammo_slugs", true))
		{
			auto entity = std::make_shared<Q3AmmoSlugs>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "ammo_cells", true))
		{
			auto entity = std::make_shared<Q3AmmoCells>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "ammo_bfg", true))
		{
			auto entity = std::make_shared<Q3AmmoBFG>(engine);
			return parseGeneric(keyValues, entity);
		}
		//////////////////////////////////////////////////////////////////////////
		//\brief: Powerup entities
		//////////////////////////////////////////////////////////////////////////
		if (strEq(className, "item_quad", true))
		{
			auto entity = std::make_shared<Q3ItemQuad>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "item_enviro", true))
		{
			auto entity = std::make_shared<Q3ItemEnviro>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "item_invis", true))
		{
			auto entity = std::make_shared<Q3ItemInvis>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "item_regen", true))
		{
			auto entity = std::make_shared<Q3ItemRegen>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "item_haste", true))
		{
			auto entity = std::make_shared<Q3ItemHaste>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "item_flight", true))
		{
			auto entity = std::make_shared<Q3ItemFlight>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "holdable_teleporter", true))
		{
			auto entity = std::make_shared<Q3HoldableTeleporter>(engine);
			return parseGeneric(keyValues, entity);
		}

		if (strEq(className, "holdable_medkit", true))
		{
			auto entity = std::make_shared<Q3HoldableMedkit>(engine);
			return parseGeneric(keyValues, entity);
		}
		//////////////////////////////////////////////////////////////////////////
		//\brief: Remaining entities
		//////////////////////////////////////////////////////////////////////////
		if (strEq(className, "shooter_grenade", true))
		{
			auto entity = std::make_shared<Q3ShooterGrenade>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "light", true))
		{
			auto entity = std::make_shared<Q3Light>(engine);
			return parseGeneric(keyValues, entity);
		}
		if (strEq(className, "worldspawn", true))
		{
			auto entity = std::make_shared<Q3WorldSpawn>(engine);
			return parseGeneric(keyValues, entity);
		}
		return nullptr;
	}
}