#include "scene.h"
#include "utils.h"

#include "prefab.h"
#include "extra/cJSON.h"

GTR::Scene* GTR::Scene::instance = NULL;

GTR::Scene::Scene()
{
	instance = this;
	
}

void GTR::Scene::clear()
{
	for (int i = 0; i < entities.size(); ++i)
	{
		BaseEntity* ent = entities[i];
		delete ent;
	}
	entities.resize(0);
}


void GTR::Scene::addEntity(BaseEntity* entity)
{
	entities.push_back(entity); 
	entity->scene = this;
}

bool GTR::Scene::load(const char* filename)
{
	std::string content;

	this->filename = filename;
	std::cout << " + Reading scene JSON: " << filename << "..." << std::endl;

	if (!readFile(filename, content))
	{
		std::cout << "- ERROR: Scene file not found: " << filename << std::endl;
		return false;
	}

	//parse json string 
	cJSON* json = cJSON_Parse(content.c_str());
	if (!json)
	{
		std::cout << "ERROR: Scene JSON has errors: " << filename << std::endl;
		return false;
	}

	//read global properties
	background_color = readJSONVector3(json, "background_color", background_color);
	ambient_light = readJSONVector3(json, "ambient_light", ambient_light );
	main_camera.eye = readJSONVector3(json, "camera_position", main_camera.eye);
	main_camera.center = readJSONVector3(json, "camera_target", main_camera.center);
	main_camera.fov = readJSONNumber(json, "camera_fov", main_camera.fov);

	//entities
	cJSON* entities_json = cJSON_GetObjectItemCaseSensitive(json, "entities");
	cJSON* entity_json;
	cJSON_ArrayForEach(entity_json, entities_json)
	{
		std::string type_str = cJSON_GetObjectItem(entity_json, "type")->valuestring;
		BaseEntity* ent = createEntity(type_str);
		if (!ent)
		{
			std::cout << " - ENTITY TYPE UNKNOWN: " << type_str << std::endl;
			//continue;
			ent = new BaseEntity();
		}

		addEntity(ent);

		if (cJSON_GetObjectItem(entity_json, "name"))
		{
			ent->name = cJSON_GetObjectItem(entity_json, "name")->valuestring;
			stdlog(std::string(" + entity: ") + ent->name);
		}

		//read transform
		if (cJSON_GetObjectItem(entity_json, "position"))
		{
			ent->model.setIdentity();
			Vector3 position = readJSONVector3(entity_json, "position", Vector3());
			ent->model.translate(position.x, position.y, position.z);
		}

		if (cJSON_GetObjectItem(entity_json, "angle"))
		{
			float angle = cJSON_GetObjectItem(entity_json, "angle")->valuedouble;
			ent->model.rotate(angle * DEG2RAD, Vector3(0, 1, 0));
		}

		if (cJSON_GetObjectItem(entity_json, "rotation"))
		{
			Vector4 rotation = readJSONVector4(entity_json, "rotation");
			Quaternion q(rotation.x, rotation.y, rotation.z, rotation.w);
			Matrix44 R;
			q.toMatrix(R);
			ent->model = R * ent->model;
		}

		if (cJSON_GetObjectItem(entity_json, "target"))
		{
			Vector3 target = readJSONVector3(entity_json, "target", Vector3(0,0,0));
			Vector3 front = target - ent->model.getTranslation();
			ent->model.setFrontAndOrthonormalize(front);
		}

		if (cJSON_GetObjectItem(entity_json, "scale"))
		{
			Vector3 scale = readJSONVector3(entity_json, "scale", Vector3(1, 1, 1));
			ent->model.scale(scale.x, scale.y, scale.z);
		}

		ent->configure(entity_json);
	}

	//free memory
	cJSON_Delete(json);

	return true;
}

GTR::BaseEntity* GTR::Scene::createEntity(std::string type)
{
	if (type == "PREFAB")
		return new GTR::PrefabEntity();
	if (type == "LIGHT")
		return new GTR::LightEntity();
	return NULL;
}

void GTR::BaseEntity::renderInMenu()
{
#ifndef SKIP_IMGUI
	ImGui::Text("Name: %s", name.c_str()); // Edit 3 floats representing a color
	ImGui::Checkbox("Visible", &visible); // Edit 3 floats representing a color
	//Model edit
	ImGuiMatrix44(model, "Model");
#endif
}




GTR::PrefabEntity::PrefabEntity()
{
	entity_type = PREFAB;
	prefab = NULL;
}

void GTR::PrefabEntity::configure(cJSON* json)
{
	if (cJSON_GetObjectItem(json, "filename"))
	{
		filename = cJSON_GetObjectItem(json, "filename")->valuestring;
		prefab = GTR::Prefab::Get( (std::string("data/") + filename).c_str());
	}

}



void GTR::PrefabEntity::renderInMenu()
{
	BaseEntity::renderInMenu();

#ifndef SKIP_IMGUI
	ImGui::Text("filename: %s", filename.c_str()); // Edit 3 floats representing a color
	if (prefab && ImGui::TreeNode(prefab, "Prefab Info"))
	{
		prefab->root.renderInMenu();
		ImGui::TreePop();
	}
#endif



}


GTR::LightEntity::LightEntity() {

	this->entity_type = LIGHT;
	//this->pos = this->model.getTranslation();
	this->color.set(1, 1, 1);
	this->target.set(0, 0, 0);

	this->intensity = 0;
	this->light_type = DIRECTIONAL;
	this->max_dist = 0;
	this->cone_angle = 0;
	this->area_size = 0;
	this->spot_exp = 0;

	this->light_camera.lookAt(this->model.getTranslation(), this->model * Vector3(0, 0, 1), this->model.rotateVector(Vector3(0, 1, 0)));

	
}


void GTR::LightEntity::uploadToShader(Shader* sh)
{
	sh->setUniform("u_light_type", this->light_type);
	sh->setUniform("u_light_color", this->color);
	
	 
	sh->setUniform("u_light_position", this->model.getTranslation() );
	sh->setUniform("u_light_vector", model.frontVector() );
	
	sh->setUniform("u_light_spotCosineCutoff", cosf(this->cone_angle * DEG2RAD));
	sh->setUniform("u_light_spotExponent", spot_exp );
	
	sh->setUniform("u_light_intensity", intensity );
	sh->setUniform("u_light_maxdist", max_dist );
	
	sh->setUniform("u_light_area_size", area_size );


}




void GTR::LightEntity::configure(cJSON* json)
{
	
	if (cJSON_GetObjectItem(json, "light_type"))
	{
		std::string lightType = cJSON_GetObjectItem(json, "light_type")->valuestring;
		if (lightType == "POINT") {
			this->light_type = POINT; 
		}
		if (lightType == "DIRECTIONAL") {
			this->light_type = DIRECTIONAL;
		}
		if (lightType == "SPOT") {
			this->light_type = SPOT;
		}
		
	}
	if (cJSON_GetObjectItem(json, "color"))
	{
		Vector3 light_color = readJSONVector3(json, "color", Vector3(1, 1, 1));
		this->color = light_color;
	}
	if (cJSON_GetObjectItem(json, "intensity"))
	{
		float intensity = cJSON_GetObjectItem(json, "intensity")->valuedouble;
		this->intensity = intensity;
	}
	if (cJSON_GetObjectItem(json, "max_dist"))
	{
		float max_dist = cJSON_GetObjectItem(json, "max_dist")->valuedouble;
		this->max_dist = max_dist;
	}

	if (cJSON_GetObjectItem(json, "area_size"))
	{
		float area_size = cJSON_GetObjectItem(json, "area_size")->valuedouble;
		this->area_size = area_size;
	}
	if (cJSON_GetObjectItem(json, "cone_angle"))
	{
		float cone_angle = cJSON_GetObjectItem(json, "cone_angle")->valuedouble;
		this->cone_angle = cone_angle;
	}
	if (cJSON_GetObjectItem(json, "cone_exp"))
	{
		float cone_exp = cJSON_GetObjectItem(json, "cone_exp")->valuedouble;
		this->spot_exp = cone_exp;
	}



}


void GTR::LightEntity::renderInMenu()
{
	BaseEntity::renderInMenu();

	#ifndef SKIP_IMGUI
		
		ImGui::ColorEdit3("Light Color", color.v);

		if (this->light_type == DIRECTIONAL) {
			ImGui::SliderFloat("Intensity", &intensity, 0, 20);
			ImGui::SliderFloat("Area size", &area_size, 0, 250);
		}

		if (this->light_type == POINT) {
			ImGui::SliderFloat("Max dist", &max_dist, 0, 1000);
			ImGui::SliderFloat("Intensity", &intensity, 0, 20);
		}

		if (this->light_type == SPOT) {
			ImGui::SliderFloat("Max dist", &max_dist, 0, 1000);
			ImGui::SliderFloat("Intensity", &intensity, 0, 20);
			ImGui::SliderFloat("Cone angle", &cone_angle, 0, 100);
			ImGui::SliderFloat("Spot exp", &spot_exp, 0, 30);
			ImGui::SliderFloat("Spot cutoff", &spot_cutoff, 0, 90);

		}
		
	#endif
}