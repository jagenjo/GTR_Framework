#pragma once

#include <string>

#include "../core/math.h"
#include "camera.h"
#include "animation.h"
#include "prefab.h"


//forward declaration
class cJSON; 


//our namespace
namespace SCN {

	//forward declaration
	class BaseEntity;
	class Scene;


	//list of plausible entity types
	enum eEntityType {
		NONE = 0,
		PREFAB = 1,
		LIGHT = 2,
		CAMERA = 3,
		DECAL = 4,
		TERRAIN = 5,
		VOXEL = 6,
		PARTICLE_SYSTEM = 7,
		CHARACTER = 8,

		REFLECTION_PROBE = 10,
		PLANAR_REFLECTION = 11,
		IRRADIANCE_VOLUME = 12,

		VOLUME = 20,
		SPLINE = 21,
		TRIGGER = 22,

		UNKNOWN = 0xFF
	};

	//used for ray picking against the scene
	struct RayTestResult {
		bool collided;
		float t;
		Vector3f collision;
		Vector3f normal;
		BaseEntity* entity;
	};

	#define ENTITY_METHODS(_A,_B,_ICONX,_ICONY) \
		virtual BaseEntity* clone() const { auto it = new _A(); *it = *this; it->scene = nullptr; return it; };\
		virtual eEntityType getType() const { return eEntityType::_B; }; \
		virtual const char* getTypeAsStr() const { return #_B; }; \
		virtual vec2 getTypeIcon() const { return vec2(_ICONX,_ICONY); };

	#define REGISTER_ENTITY_TYPE(_A) SCN::BaseEntity::registerEntityType(new _A());

	//represents one element of the scene (could be lights, prefabs, decals, etc)
	class BaseEntity
	{
	public:
		static BaseEntity* s_selected;
		static std::map<std::string, BaseEntity*> s_factory;
		Scene* scene;
		SCN::Node root;

		std::string name;
		bool visible;
		uint8 layers;

		BaseEntity() { scene = nullptr; visible = true; layers = 3; }
		virtual ~BaseEntity() { assert(!scene); if (s_selected == this) s_selected = nullptr; };
		
		virtual void configure(cJSON* json) {}
		virtual void serialize(cJSON* json) {}

		virtual BaseEntity* clone() const = 0; //must be implemented
		virtual eEntityType getType() const { return eEntityType::NONE; }
		virtual const char* getTypeAsStr() const { return "NONE"; }
		virtual vec2 getTypeIcon() const { return { 0, 1 }; }

		virtual bool testRay(const Ray& ray, Vector3f& coll, float max_dist = 100000.0f);

		static void registerEntityType(BaseEntity* entity);
		static BaseEntity* createEntity(const char* type);
	};

	//represents one prefab in the scene
	class PrefabEntity : public SCN::BaseEntity
	{
	public:
		std::string filename;
		Prefab* prefab;
		
		PrefabEntity();

		ENTITY_METHODS(PrefabEntity, PREFAB, 11,0);

		virtual void configure(cJSON* json);
		virtual void serialize(cJSON* json);
		void loadPrefab(const char* filename);

		bool testRay(const Ray& ray, Vector3f& coll, float max_dist = 100000.0f);
	};

	class UnknownEntity : public SCN::BaseEntity
	{
	public:
		std::string original_type;
		void* data;

		UnknownEntity();
		~UnknownEntity();

		ENTITY_METHODS( UnknownEntity, UNKNOWN,1,0 );
		virtual void configure(cJSON* json);
		virtual void serialize(cJSON* json);
		virtual const char* getTypeAsStr() { return original_type.c_str(); };
	};

	//contains all entities of the scene
	class Scene
	{
	public:
		static Scene* instance;

		Vector3f background_color;
		Vector3f ambient_light;
		std::string skybox_filename;
		Camera main_camera;

		Scene();

		std::string filename;
		std::string base_folder;
		std::vector<BaseEntity*> entities;

		void clear();
		void addEntity(BaseEntity* entity);
		void removeEntity(BaseEntity* entity);

		bool load(const char* filename);
		bool save(const char* filename);
		bool toString(std::string& data);
		bool fromString(std::string& data, const char* base_folder = nullptr);

		BaseEntity* getEntity(std::string name);

		RayTestResult testRay( Ray& ray, uint8 layers = 0xFF );
	};

};
