#include "renderer.h"

#include <algorithm> //sort

#include "camera.h"
#include "../gfx/gfx.h"
#include "../gfx/shader.h"
#include "../gfx/mesh.h"
#include "../gfx/texture.h"
#include "../gfx/fbo.h"
#include "../pipeline/prefab.h"
#include "../pipeline/material.h"
#include "../pipeline/animation.h"
#include "../utils/utils.h"
#include "../extra/hdre.h"
#include "../core/ui.h"

#include "scene.h"


using namespace SCN;

//some globals
GFX::Mesh sphere;

Renderer::Renderer(const char* shader_atlas_filename)
{
	render_wireframe = false;
	render_boundaries = false;
	scene = nullptr;
	skybox_cubemap = nullptr;

	if (!GFX::Shader::LoadAtlas(shader_atlas_filename))
		exit(1);
	GFX::checkGLErrors();

	sphere.createSphere(1.0f);
	sphere.uploadToVRAM();

	alpha_sort_mode = eAlphaSortMode::SORTED;
	render_mode = eRenderMode::LIGHTS;
	lights_mode = eLightsMode::SINGLE;
	nmap_mode = eNormalMapMode::WITH_NMAP;
	spec_mode = eSpecMode::WITH_SPEC;

	// initialize render_calls vector
	render_calls = *(new std::vector<RenderCall>());

	// get white texture for the mesh renderer, we only get it once and can use it whenever we need to
	white_texture = GFX::Texture::getWhiteTexture();
}

void Renderer::setupScene(Camera * camera)
{
	render_calls.clear();

	if (scene->skybox_filename.size())
		skybox_cubemap = GFX::Texture::Get(std::string(scene->base_folder + "/" + scene->skybox_filename).c_str());
	else
		skybox_cubemap = nullptr;

	lights.clear();

	//process entities
	for (int i = 0; i < scene->entities.size(); ++i)
	{
		BaseEntity* ent = scene->entities[i];
		if (!ent->visible)
			continue;

		//is a prefab!
		if (ent->getType() == eEntityType::PREFAB)
		{
			PrefabEntity* pent = (SCN::PrefabEntity*)ent;
			if (pent->prefab)
				createNodeRC(&pent->root, camera);
		}
		else if (ent->getType() == eEntityType::LIGHT)
		{
			lights.push_back((SCN::LightEntity*)ent);
		}
	}
	
}

void Renderer::renderScene(SCN::Scene* scene, Camera* camera)
{
	this->scene = scene;
	setupScene(camera);

	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);

	//set the clear color (the background color)
	glClearColor(scene->background_color.x, scene->background_color.y, scene->background_color.z, 1.0);

	// Clear the color and the depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	GFX::checkGLErrors();

	//render skybox
	if(skybox_cubemap)
		renderSkybox(skybox_cubemap);

	if(alpha_sort_mode == SORTED) std::sort(render_calls.begin(), render_calls.end(), rc_sorter());

	// render everything
	switch (render_mode) {
		case eRenderMode::FLAT:
			for (auto& rc : render_calls) {
				renderMeshWithMaterial(rc);
			}
			break;
		case eRenderMode::LIGHTS:
			updateRCLights();

			switch (lights_mode) {
				case MULTI:
					for (auto& rc : render_calls) {
						renderMeshWithMaterialLight(rc);
					}
					break;
				case SINGLE:
					for (auto& rc : render_calls) {
						renderMeshWithMaterialLightSingle(rc);
					}
					break;
			}

			break;
	}

}


void Renderer::renderSkybox(GFX::Texture* cubemap)
{
	Camera* camera = Camera::current;

	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	if (render_wireframe)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	GFX::Shader* shader = GFX::Shader::Get("skybox");
	if (!shader)
		return;
	shader->enable();

	Matrix44 m;
	m.setTranslation(camera->eye.x, camera->eye.y, camera->eye.z);
	m.scale(10, 10, 10);
	shader->setUniform("u_model", m);
	cameraToShader(camera, shader);
	shader->setUniform("u_texture", cubemap, 0);
	sphere.render(GL_TRIANGLES);
	shader->disable();
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_DEPTH_TEST);
}

//renders a mesh given its transform and material
void Renderer::renderMeshWithMaterial(const RenderCall rc)
{
	//in case there is nothing to do
	if (!rc.mesh || !rc.mesh->getNumVertices() || !rc.material )
		return;
    assert(glGetError() == GL_NO_ERROR);

	//define locals to simplify coding
	GFX::Shader* shader = NULL;
	Camera* camera = Camera::current;
	
	GFX::Texture* albedo_texture = rc.material->textures[SCN::eTextureChannel::ALBEDO].texture;

	//select the blending
	if (rc.material->alpha_mode == SCN::eAlphaMode::BLEND)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	else
		glDisable(GL_BLEND);

	//select if render both sides of the triangles
	if(rc.material->two_sided)
		glDisable(GL_CULL_FACE);
	else
		glEnable(GL_CULL_FACE);
    assert(glGetError() == GL_NO_ERROR);

	glEnable(GL_DEPTH_TEST);

	//chose a shader
	shader = GFX::Shader::Get("texture");

    assert(glGetError() == GL_NO_ERROR);

	//no shader? then nothing to render
	if (!shader)
		return;
	shader->enable();

	//upload uniforms
	shader->setUniform("u_model", rc.model);
	cameraToShader(camera, shader);
	float t = getTime();
	shader->setUniform("u_time", t );


	// send textures to shader
	shader->setUniform("u_color", rc.material->color);
	shader->setUniform("u_texture", albedo_texture ? albedo_texture : white_texture, 0);

	//this is used to say which is the alpha threshold to what we should not paint a pixel on the screen (to cut polygons according to texture alpha)
	shader->setUniform("u_alpha_cutoff", rc.material->alpha_mode == SCN::eAlphaMode::MASK ? rc.material->alpha_cutoff : 0.001f);

	if (render_wireframe)
		glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

	//do the draw call that renders the mesh into the screen
	rc.mesh->render(GL_TRIANGLES);

	//disable shader
	shader->disable();

	//set the render state as it was before to avoid problems with future renders
	glDisable(GL_BLEND);
	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
}

//renders a mesh given its transform and material adding lights using, multi-pass
void Renderer::renderMeshWithMaterialLight(const RenderCall rc)
{
	//in case there is nothing to do
	if (!rc.mesh || !rc.mesh->getNumVertices() || !rc.material)
		return;
	assert(glGetError() == GL_NO_ERROR);

	//define locals to simplify coding
	GFX::Shader* shader = NULL;
	Camera* camera = Camera::current;

	GFX::Texture* albedo_texture = rc.material->textures[SCN::eTextureChannel::ALBEDO].texture;
	GFX::Texture* emissive_texture = rc.material->textures[SCN::eTextureChannel::EMISSIVE].texture;
	GFX::Texture* occ_metal_rough_text = rc.material->textures[SCN::eTextureChannel::METALLIC_ROUGHNESS].texture;
	GFX::Texture* normal_map = (nmap_mode == eNormalMapMode::WITH_NMAP) ? white_texture : rc.material->textures[SCN::eTextureChannel::NORMALMAP].texture;

	//select the blending
	if (rc.material->alpha_mode == SCN::eAlphaMode::BLEND)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	else
		glDisable(GL_BLEND);

	//select if render both sides of the triangles
	if (rc.material->two_sided)
		glDisable(GL_CULL_FACE);
	else
		glEnable(GL_CULL_FACE);
	assert(glGetError() == GL_NO_ERROR);

	glEnable(GL_DEPTH_TEST);

	//chose a shader
	shader = GFX::Shader::Get("light");

	assert(glGetError() == GL_NO_ERROR);

	//no shader? then nothing to render
	if (!shader)
		return;
	shader->enable();

	//upload uniforms
	shader->setUniform("u_model", rc.model);
	cameraToShader(camera, shader);
	float t = getTime();
	shader->setUniform("u_time", t);



	shader->setUniform("u_color", rc.material->color);
	shader->setUniform("u_emissive_factor", rc.material->emissive_factor);


	// send textures to shader
	// last parameter is the slot we assign
	shader->setUniform("u_albedo_texture", albedo_texture ? albedo_texture : white_texture, 0);
	shader->setUniform("u_emissive_texture", emissive_texture ? emissive_texture : white_texture, 1);
	shader->setUniform("u_normal_map", normal_map ? normal_map : white_texture, 3);


	shader->setUniform("u_metal_rough_factor", vec3(rc.material->metallic_factor, rc.material->roughness_factor, spec_mode));
	if (spec_mode) {
		shader->setUniform("u_occl_metal_rough_texture", occ_metal_rough_text ? occ_metal_rough_text : white_texture, 2);
	}

	//this is used to say which is the alpha threshold to what we should not paint a pixel on the screen (to cut polygons according to texture alpha)
	shader->setUniform("u_alpha_cutoff", rc.material->alpha_mode == SCN::eAlphaMode::MASK ? rc.material->alpha_cutoff : 0.001f);

	// pass the ambient light
	shader->setUniform("u_ambient_light", scene->ambient_light);


	if (render_wireframe)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);


	// Draw pixels if the z is the same or less than what is already drawn (closer to camera)
	glDepthFunc(GL_LEQUAL);

	int num_lights = rc.lights_affecting.size();

	if (num_lights) {
		// first iteration out of the loop
		LightEntity* light = rc.lights_affecting[0];

		shader->setUniform("u_light_position", light->root.model.getTranslation());
		shader->setUniform("u_light_front", light->root.model.rotateVector(vec3(0, 0, 1)));

		// When working with angle functions ALWAYS USE RADS
		if (light->light_type == eLightType::SPOT)
			shader->setUniform("u_light_cone", vec2(cos(light->cone_info.x * DEG2RAD), cos(light->cone_info.y * DEG2RAD)));

		// we can save some space on the shader if we directly multiply the color and intensity of the light in the CPU, as this will always be done
		shader->setUniform("u_light_color", light->color * light->intensity);
		shader->setUniform("u_light_info", vec4((int)light->light_type, light->near_distance, light->max_distance, nmap_mode));

		rc.mesh->render(GL_TRIANGLES);

		glEnable(GL_BLEND);
		// GL_ONE -> take color of what is already drawn and mult by 1, then take whatever you are trying to color now and mult by its alpha, add them together
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);

		// emissive and ambient light must be added only once
		shader->setUniform("u_ambient_light", vec3(0.0, 0.0, 0.0));
		shader->setUniform("u_emissive_factor", vec3(0.0, 0.0, 0.0));

		// loop with rest of the lights
		for (int i = 1; i < num_lights; i++) {
			LightEntity* light = rc.lights_affecting[i];


			if (light->light_type == eLightType::SPOT)
				shader->setUniform("u_light_cone", vec2(cos(light->cone_info.x * DEG2RAD), cos(light->cone_info.y * DEG2RAD)));

			shader->setUniform("u_light_position", light->root.model.getTranslation());
			shader->setUniform("u_light_front", light->root.model.rotateVector(vec3(0, 0, 1)));

			// we can save some space on the shader if we directly multiply the color and intensity of the light in the CPU, as this will always be done
			shader->setUniform("u_light_color", light->color * light->intensity);
			shader->setUniform("u_light_info", vec4((int)light->light_type, light->near_distance, light->max_distance, nmap_mode));

			rc.mesh->render(GL_TRIANGLES);
		}
	}
	else {
		shader->setUniform("u_light_info", vec4(eLightType::NO_LIGHT, 0.0, 0.0, nmap_mode));
		rc.mesh->render(GL_TRIANGLES);
	}

	//disable shader
	shader->disable();

	//set the render state as it was before to avoid problems with future renders
	glDisable(GL_BLEND);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDepthFunc(GL_LESS);
}


void SCN::Renderer::cameraToShader(Camera* camera, GFX::Shader* shader)
{
	shader->setUniform("u_viewprojection", camera->viewprojection_matrix );
	shader->setUniform("u_camera_position", camera->eye);
}

#ifndef SKIP_IMGUI

void Renderer::showUI()
{
		
	ImGui::Checkbox("Wireframe", &render_wireframe);
	ImGui::Checkbox("Boundaries", &render_boundaries);

	ImGui::Combo("Alpha Sorting Mode", (int*) & alpha_sort_mode, "NOT SORTED\0SORTED", 2);
	ImGui::Combo("Render Mode", (int*) & render_mode, "FLAT\0LIGHTS", 2);
	ImGui::Combo("Lights Mode", (int*) & lights_mode, "MULTI-PASS\0SINGLE PASS", 2);
	ImGui::Combo("Normal Map Mode", (int*) & nmap_mode, "WITHOUT\0WITH", 2);
	ImGui::Combo("Specular Light Mode", (int*) &spec_mode, "WITHOUT\0WITH", 2);

	//add here your stuff
	//...
}

#else
void Renderer::showUI() {}
#endif

// *********************************************** MY CODE ***********************************************

// Create the render calls for one node
void Renderer::createNodeRC(SCN::Node* node, Camera* camera)
{
	if (!node->visible)
		return;

	//compute global matrix
	Matrix44 node_model = node->getGlobalMatrix(true);

	//does this node have a mesh? then we must render it
	if (node->mesh && node->material)
	{
		//compute the bounding box of the object in world space (by using the mesh bounding box transformed to world space)
		BoundingBox world_bounding = transformBoundingBox(node_model, node->mesh->box);

		//if bounding box is inside the camera frustum then the object is probably visible
		if (camera->testBoxInFrustum(world_bounding.center, world_bounding.halfsize))
		{
			if (render_boundaries)
				node->mesh->renderBounding(node_model, true);
			// create the render call associated with the node
			createRenderCall(node_model, node->mesh, node->material);
		}
	}

	//iterate recursively with children
	for (int i = 0; i < node->children.size(); ++i)
		createNodeRC(node->children[i], camera);
}


// Function that will create a render_call based on a model matrix, a mesh and a material and will store it
// in the render_call vector
void Renderer::createRenderCall(Matrix44 model, GFX::Mesh* mesh, SCN::Material* material)
{
	RenderCall rc;
	Vector3f node_pos = model.getTranslation();

	// Fill out model, mesth and material
	rc.model = model;
	rc.mesh = mesh;
	rc.material = material;

	// Compute distance to the camera
	Camera* cam = Camera::current;
	rc.distance_to_camera = node_pos.distance(cam->eye);

	// Push RenderCall to the vector
	render_calls.push_back(rc);
}


void Renderer::updateRCLights() {
	if (lights.size()) {
		for (auto& rc : render_calls) {
			rc.lights_affecting.clear();
			BoundingBox world_bounding = transformBoundingBox(rc.model, rc.mesh->box);

			for (int i = 0; i < lights.size(); i++) {
				LightEntity* light = lights[i];

				if (light->light_type == eLightType::NO_LIGHT) {
					continue;
				}// Directional light always affects, no need to compute if the bounding boxes overlap
				else if (light->light_type == eLightType::DIRECTIONAL) {
					rc.lights_affecting.push_back(light);
					continue;
				}

				// if it overlaps, push light pointer to the vector of lights that affect the node
				if (BoundingBoxSphereOverlap(world_bounding, light->root.model.getTranslation(), light->max_distance))
					rc.lights_affecting.push_back(light);
			}
		}
	}

}

//renders a mesh given its transform and material adding lights, single - pass
void Renderer::renderMeshWithMaterialLightSingle(const RenderCall rc)
{
	//in case there is nothing to do
	if (!rc.mesh || !rc.mesh->getNumVertices() || !rc.material)
		return;
	assert(glGetError() == GL_NO_ERROR);

	//define locals to simplify coding
	GFX::Shader* shader = NULL;
	Camera* camera = Camera::current;

	GFX::Texture* albedo_texture = rc.material->textures[SCN::eTextureChannel::ALBEDO].texture;
	GFX::Texture* emissive_texture = rc.material->textures[SCN::eTextureChannel::EMISSIVE].texture;
	GFX::Texture* occ_metal_rough_text = rc.material->textures[SCN::eTextureChannel::METALLIC_ROUGHNESS].texture;
	GFX::Texture* normal_map = (nmap_mode == eNormalMapMode::WITH_NMAP) ? white_texture : rc.material->textures[SCN::eTextureChannel::NORMALMAP].texture;

	//select the blending
	if (rc.material->alpha_mode == SCN::eAlphaMode::BLEND)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	else
		glDisable(GL_BLEND);

	//select if render both sides of the triangles
	if (rc.material->two_sided)
		glDisable(GL_CULL_FACE);
	else
		glEnable(GL_CULL_FACE);
	assert(glGetError() == GL_NO_ERROR);

	glEnable(GL_DEPTH_TEST);

	//chose a shader
	shader = GFX::Shader::Get("lightSinglePass");

	assert(glGetError() == GL_NO_ERROR);

	//no shader? then nothing to render
	if (!shader)
		return;
	shader->enable();

	//upload uniforms
	shader->setUniform("u_model", rc.model);
	cameraToShader(camera, shader);
	float t = getTime();
	shader->setUniform("u_time", t);



	shader->setUniform("u_color", rc.material->color);
	shader->setUniform("u_emissive_factor", rc.material->emissive_factor);


	// send textures to shader
	// last parameter is the slot we assign
	shader->setUniform("u_albedo_texture", albedo_texture ? albedo_texture : white_texture, 0);
	shader->setUniform("u_emissive_texture", emissive_texture ? emissive_texture : white_texture, 1);
	shader->setUniform("u_normal_map", normal_map ? normal_map : white_texture, 3);


	shader->setUniform("u_metal_rough_factor", vec3(rc.material->metallic_factor, rc.material->roughness_factor, spec_mode));
	if (spec_mode) {
		shader->setUniform("u_occl_metal_rough_texture", occ_metal_rough_text ? occ_metal_rough_text : white_texture, 2);
	}

	//this is used to say which is the alpha threshold to what we should not paint a pixel on the screen (to cut polygons according to texture alpha)
	shader->setUniform("u_alpha_cutoff", rc.material->alpha_mode == SCN::eAlphaMode::MASK ? rc.material->alpha_cutoff : 0.001f);

	// pass the ambient light
	shader->setUniform("u_ambient_light", scene->ambient_light);


	if (render_wireframe)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);


	// Draw pixels if the z is the same or less than what is already drawn (closer to camera)
	glDepthFunc(GL_LEQUAL);

	int num_lights = rc.lights_affecting.size();


	vec2 light_cone[MAX_LIGHTS];
	vec3 light_position[MAX_LIGHTS];
	vec3 light_front[MAX_LIGHTS];
	vec3 light_color[MAX_LIGHTS];
	vec4 light_info[MAX_LIGHTS];

	if (num_lights) {

		// loop with the lights
		for (int i = 0; i < num_lights; i++) {
			LightEntity* light = rc.lights_affecting[i];

			if (light->light_type == eLightType::SPOT)
				light_cone[i] = vec2(cos(light->cone_info.x * DEG2RAD), cos(light->cone_info.y * DEG2RAD));


			light_position[i] = light->root.model.getTranslation();
			light_front[i] = light->root.model.rotateVector(vec3(0, 0, 1));
			light_color[i] = light->color * light->intensity;
			
			light_info[i] = vec4((int)light->light_type, light->near_distance, light->max_distance, nmap_mode);


			
		}

		shader->setUniform2Array("u_light_cone", (float*)&light_cone, MAX_LIGHTS);
		shader->setUniform3Array("u_light_position", (float*)&light_position, MAX_LIGHTS);
		shader->setUniform3Array("u_light_front", (float*)&light_front, MAX_LIGHTS);
		shader->setUniform3Array("u_light_color", (float*)&light_color, MAX_LIGHTS);
		shader->setUniform4Array("u_light_info", (float*)&light_info, MAX_LIGHTS);

		shader->setUniform("u_num_lights", num_lights);

		rc.mesh->render(GL_TRIANGLES);
	}
	else {
		shader->setUniform("u_num_lights", 1);

		light_info[0] = vec4(eLightType::NO_LIGHT, 0.0, 0.0, nmap_mode);
		shader->setUniform4Array("u_light_info", (float*)&light_info, MAX_LIGHTS);

		rc.mesh->render(GL_TRIANGLES);
	}

	//disable shader
	shader->disable();

	//set the render state as it was before to avoid problems with future renders
	glDisable(GL_BLEND);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDepthFunc(GL_LESS);
}

