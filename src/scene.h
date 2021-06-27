#ifndef SCENE_H
#define SCENE_H

#include "framework.h"
#include "camera.h"
#include <string>
#include "shader.h"
#include "fbo.h"


//forward declaration
class cJSON; 


//our namespace
namespace GTR {

	enum eEntityType {
		NONE = 0,
		PREFAB = 1,
		LIGHT = 2,
		CAMERA = 3,
		REFLECTION_PROBE = 4,
		DECAL = 5
	};

	class Scene;
	class Prefab;
	class Node;
	

	//represents one element of the scene (could be lights, prefabs, cameras, etc)
	class BaseEntity
	{
	public:
		Scene* scene; //puntero scene, modificable
		std::string name;
		eEntityType entity_type;
		Matrix44 model;
		bool visible;
		BaseEntity() { entity_type = NONE; visible = true; }
		virtual ~BaseEntity() {}

		virtual void renderInMenu();
		virtual void configure(cJSON* json) {
		};
	};

	//represents one prefab in the scene
	class PrefabEntity : public GTR::BaseEntity
	{
	public:
		std::string filename;
		Prefab* prefab;
		
		PrefabEntity();

		virtual void renderInMenu();
		virtual void configure(cJSON* json);
	};

	enum eLightType {
		DIRECTIONAL = 0,
		POINT = 1,
		SPOT = 2
	};

	class LightEntity : public GTR::BaseEntity 
	{
	public:
				
		eLightType light_type;
		Vector3 color;
		float intensity;			
		float max_dist; //how far the light can reach
		float area_size; // the size of the volume for directional light
		
		//spot light param
		float cone_angle;
		float spot_exp;
		//shadownmap param
		FBO* shadow_fbo;
		float shadow_bias;
		Camera* light_camera;
		bool cast_shadows;

		LightEntity();

		//uploads properties to shader uniforms
		void uploadToShader(Shader* shader);

		virtual void renderInMenu();
		virtual void configure(cJSON* json);

		void configureLightCamera();

	};

	enum eDecalTextureType {
		albedo = 0,
		normal = 1
	};
	

	class DecalEntity : public GTR::BaseEntity
	{
	public:
		Texture* decal_texture;
		DecalEntity();
		eDecalTextureType texture_type;
		//virtual void renderInMenu();
		virtual void configure(cJSON* json);
	};

	//contains all entities of the scene
	class Scene
	{
	public:

		static Scene* instance;
		Vector3 background_color;
		Vector3 ambient_light;
		float max_dist_ao;
		Camera main_camera;

		Scene();

		std::string filename;
		std::vector<BaseEntity*> entities;
		std::vector<LightEntity*> light_entities;

		void clear();
		void addEntity(BaseEntity* entity);

		bool load(const char* filename);
		BaseEntity* createEntity(std::string type);
	};

};

#endif