#include <algorithm> //std::find

#include "scene.h"
#include "../utils/utils.h"

#include "prefab.h"
#include "../extra/cJSON.h"
#include "../core/ui.h"
#include "../gfx/texture.h"

SCN::Scene* SCN::Scene::instance = NULL;

//test ray against sphere
bool SCN::BaseEntity::testRay(const Ray& ray, Vector3f& coll, float max_dist )
{
	return false;
	float t = 0.0f;
	Vector3f pos = root.model.getTranslation();
	bool col = RaySphereCollision(pos, .5f, ray.origin, ray.direction, coll, t);
	if (t > max_dist)
		return false;
	return true;
}

SCN::Scene::Scene()
{
	instance = this;
}

void SCN::Scene::clear()
{
	for (int i = 0; i < entities.size(); ++i)
	{
		BaseEntity* ent = entities[i];
		ent->scene = nullptr; //remove scene
		delete ent;
	}
	entities.resize(0);
	BaseEntity::s_selected = nullptr;
	SCN::Node::s_selected = nullptr;
}

bool SCN::Scene::load(const char* filename)
{
	std::string content;

	this->filename = filename;
	this->base_folder = getFolderName(filename);
	std::cout << " + Reading scene JSON: " << TermColor::YELLOW << filename << TermColor::DEFAULT << "..." << std::endl;

	if (!readFile(filename, content))
	{
		std::cout << "- ERROR: Scene file not found: " << TermColor::RED << filename << TermColor::DEFAULT << std::endl;
		return false;
	}

	return fromString(content, base_folder.c_str());
}

bool SCN::Scene::fromString(std::string& data, const char* base_folder)
{
	//erase all
	clear();

	//parse json string 
	cJSON* json = cJSON_Parse(data.c_str());
	if (!json)
	{
		std::cout << "ERROR: Scene JSON has errors: " << TermColor::RED << filename << TermColor::DEFAULT << std::endl;
		return false;
	}

	//read global properties
	background_color = readJSONVector3(json, "background_color", background_color);
	ambient_light = readJSONVector3(json, "ambient_light", ambient_light );
	main_camera.eye = readJSONVector3(json, "camera_position", main_camera.eye);
	main_camera.center = readJSONVector3(json, "camera_target", main_camera.center);
	main_camera.fov = readJSONNumber(json, "camera_fov", main_camera.fov);
	skybox_filename = readJSONString(json, "skybox", skybox_filename.c_str());

	//entities
	cJSON* entities_json = cJSON_GetObjectItemCaseSensitive(json, "entities");
	cJSON* entity_json;
	cJSON_ArrayForEach(entity_json, entities_json)
	{
		std::string type_str = cJSON_GetObjectItem(entity_json, "type")->valuestring;
		BaseEntity* ent = BaseEntity::createEntity(type_str.c_str());
		if (!ent)
		{
			std::cout << " - Entity type unknown: " << TermColor::RED << type_str << TermColor::DEFAULT << std::endl;
			UnknownEntity* uent = new UnknownEntity();
			uent->original_type = type_str;
			ent = uent;
		}

		//add to scene
		addEntity(ent);

		//parse generic stuff
		if (cJSON_GetObjectItem(entity_json, "name"))
		{
			ent->name = cJSON_GetObjectItem(entity_json, "name")->valuestring;
			std::cout << " + Entity: " << TermColor::GREEN << ent->name << TermColor::DEFAULT << std::endl;
		}

		if (cJSON_GetObjectItem(entity_json, "layers"))
			ent->layers = cJSON_GetObjectItem(entity_json, "layers")->valueint;

		//read transform
		if (cJSON_GetObjectItem(entity_json, "position"))
		{
			ent->root.model.setIdentity();
			Vector3f position = readJSONVector3(entity_json, "position", Vector3f());
			ent->root.model.translate(position.x, position.y, position.z);
		}

		if (cJSON_GetObjectItem(entity_json, "angle"))
		{
			float angle = cJSON_GetObjectItem(entity_json, "angle")->valuedouble;
			//Quaternion q;
			//Matrix44 R;
			//q.fromEuler(Vector3(angle * DEG2RAD, 0, 0));
			//q.toMatrix(R);
			ent->root.model.rotate(-angle * DEG2RAD, Vector3f(0, 1, 0));
			//ent->model = R * ent->model;
		}

		if (cJSON_GetObjectItem(entity_json, "rotation"))
		{
			Vector4f rotation = readJSONVector4(entity_json, "rotation");
			Quaternion q(rotation.x, rotation.y, rotation.z, rotation.w);
			Matrix44 R;
			q.toMatrix(R);
			ent->root.model = R * ent->root.model;
		}

		if (cJSON_GetObjectItem(entity_json, "target"))
		{
			Vector3f target = readJSONVector3(entity_json, "target", Vector3f());
			Vector3f front = (target - ent->root.model.getTranslation()) * -1.0f;
			ent->root.model.setFrontAndOrthonormalize(front);
		}

		if (cJSON_GetObjectItem(entity_json, "scale"))
		{
			Vector3f scale = readJSONVector3(entity_json, "scale", Vector3f(1, 1, 1));
			ent->root.model.scale(scale.x, scale.y, scale.z);
		}

		ent->visible = readJSONBool(entity_json, "visible", true);

		ent->configure(entity_json);
	}

	//free memory
	cJSON_Delete(json);

	return true;
}

bool SCN::Scene::toString( std::string& data )
{
	std::string content;

	//parse json string 
	cJSON* json = cJSON_CreateObject();

	//read global properties
	writeJSONVector3(json, "background_color", background_color);
	writeJSONVector3(json, "ambient_light", ambient_light);
	writeJSONVector3(json, "camera_position", main_camera.eye);
	writeJSONVector3(json, "camera_target", main_camera.center);
	writeJSONNumber(json, "camera_fov", main_camera.fov);
	writeJSONString(json, "skybox", skybox_filename.c_str());

	//entities
	cJSON* entities_json = cJSON_CreateArray();
	cJSON_AddItemToObject(json, "entities", entities_json);

	for (auto ent : entities)
	{
		cJSON* entity_json = cJSON_CreateObject();
		cJSON_AddItemToArray(entities_json, entity_json);
		cJSON_AddStringToObject(entity_json, "name", ent->name.c_str());
		cJSON_AddNumberToObject(entity_json, "layers", ent->layers);
		writeJSONVector3(entity_json, "position", ent->root.model.getTranslation());
		if (!ent->visible)
			writeJSONBool(entity_json, "visible", false);
		writeJSONVector3(entity_json, "scale", ent->root.model.getScale());

		Quaternion q;
		q.fromMatrix(ent->root.model);
		writeJSONVector4(entity_json, "rotation", Vector4f(q.q));

		ent->serialize(entity_json);

		if(!cJSON_GetObjectItemCaseSensitive((cJSON*)entity_json, "type"))
			cJSON_AddStringToObject(entity_json, "type", ent->getTypeAsStr()); //type is last
	}

	//to string
	char* str = cJSON_Print(json);
	cJSON_Delete(json);

	int size = strlen(str);
	data.resize(size);
	memcpy(&data[0], str, size);
	delete str;

	return true;
}

bool SCN::Scene::save(const char* filename)
{
	std::cout << " + Writing scene to JSON: " << filename << "..." << std::endl;
	std::string data;
	toString(data);

	bool result = writeFile(filename, data);

	//free memory
	return result;
}

void SCN::Scene::addEntity(BaseEntity* entity)
{
	entities.push_back(entity); 
	entity->scene = this;
}

void SCN::Scene::removeEntity(BaseEntity* entity)
{
	assert(entity->scene == this);
	auto it = std::find(entities.begin(), entities.end(), entity);
	//std::remove(entities.begin(), entities.end(), entity);
	entities.erase(it);
	//entities.resize(entities.size() - 1);
}

SCN::BaseEntity* SCN::Scene::getEntity(std::string name)
{
	for (int i = 0; i < entities.size(); ++i)
	{
		BaseEntity* ent = entities[i];
		if (ent->name == name)
			return ent;
	}
	return nullptr;
}

SCN::BaseEntity* SCN::BaseEntity::s_selected = nullptr;
std::map<std::string, SCN::BaseEntity*> SCN::BaseEntity::s_factory;

void SCN::BaseEntity::registerEntityType(SCN::BaseEntity* entity)
{
	s_factory[entity->getTypeAsStr()] = entity;
}

SCN::BaseEntity* SCN::BaseEntity::createEntity(const char* type)
{
	auto it = s_factory.find(type);
	if (it == s_factory.end())
		return nullptr;
	return it->second->clone();
}

SCN::PrefabEntity::PrefabEntity()
{
	prefab = NULL;
}

void SCN::PrefabEntity::configure(cJSON* json)
{
	if (cJSON_GetObjectItem(json, "filename"))
	{
		filename = cJSON_GetObjectItem(json, "filename")->valuestring;
		loadPrefab( filename.c_str() );
	}
}

void SCN::PrefabEntity::serialize(cJSON* json)
{
	cJSON_AddStringToObject(json, "filename", filename.c_str());
}

void SCN::PrefabEntity::loadPrefab(const char* filename)
{
	assert(scene && "Cannot assign filename without scene (to extract base folder)");
	std::string fullpath = scene->base_folder + "/" + filename;
	prefab = SCN::Prefab::Get(fullpath.c_str());
	if (!prefab)
		return;
	
	SCN::Node* child = new SCN::Node();
	*child = prefab->root;
	root.clear();
	root.addChild(child);
}

bool SCN::PrefabEntity::testRay(const Ray& ray, Vector3f& coll, float max_dist)
{
	root.model = root.model;
	return root.testRay(ray, coll, 0xFF, max_dist);
}

SCN::UnknownEntity::UnknownEntity()
{
	data = nullptr;
}

SCN::UnknownEntity::~UnknownEntity()
{
	if (data)
		cJSON_Delete((cJSON*)data);
}

void SCN::UnknownEntity::configure(cJSON* json)
{
	if(data)
		cJSON_Delete((cJSON*)data);
	data = cJSON_Duplicate(json, true);
}

void SCN::UnknownEntity::serialize(cJSON* json)
{
	cJSON_AddStringToObject(json, "type", original_type.c_str());

	if (!data)
		return;

	cJSON* item_json;
	cJSON_ArrayForEach(item_json, (cJSON*)data)
	{
		cJSON* child = cJSON_GetObjectItem(json, item_json->string);
		if (child)
			continue;
		cJSON* item = cJSON_Duplicate(item_json, true);
		cJSON_AddItemReferenceToObject(json, item->string, item);
	}
}

SCN::RayTestResult SCN::Scene::testRay(Ray& ray, uint8 layers)
{
	RayTestResult result;
	result.t = 1000000.0f;
	result.collided = false;
	result.entity = nullptr;
	Vector3f collision;

	//TODO: optimal way:
	//- prebuild BVH (check https://github.com/xelatihy/yocto-gl/blob/main/libs/yocto/yocto_bvh.h )
	//- get all nodes which bounding intersects ray
	//- sort by distance to ray origin
	//- test ray with closer one, if no collision, continue
	//- if collision check if collision point inside any other node bounding, and test only in those


	float max_dist = result.t;
	for (auto& ent : entities)
	{
		if (!(ent->layers & layers))
			continue;

		if (!ent->testRay(ray, collision, max_dist))
			continue;

		float t = ray.origin.distance(collision);
		if (t > result.t)
			continue;
		result.t = t;
		result.collision = collision;
		result.entity = ent;
		result.collided = true;
	}

	return result;
}


