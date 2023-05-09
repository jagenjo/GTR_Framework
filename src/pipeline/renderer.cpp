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

GFX::FBO* gbuffer_fbo = nullptr;
GFX::FBO* illumination_fbo = nullptr;

Renderer::Renderer(const char* shader_atlas_filename)
{
	render_wireframe = false;
	render_boundaries = false;
	render_mode = eRenderMode::LIGHTS; //default
	scene = nullptr;
	skybox_cubemap = nullptr;
	show_shadowmaps = true;
	show_gbuffers = false;

	if (!GFX::Shader::LoadAtlas(shader_atlas_filename))	exit(1);

	GFX::checkGLErrors();

	sphere.createSphere(1.0f);
	sphere.uploadToVRAM();
}

void Renderer::setupScene()
{
	if (scene->skybox_filename.size())
		skybox_cubemap = GFX::Texture::Get(std::string(scene->base_folder + "/" + scene->skybox_filename).c_str());
	else
		skybox_cubemap = nullptr;

	//to avoid adding lights infinetly
	lights.clear();

	//process entities
	for (int i = 0; i < scene->entities.size(); ++i)
	{
		BaseEntity* ent = scene->entities[i];
		if (!ent->visible)
			continue;

		//prefab entity
		if (ent->getType() == eEntityType::PREFAB)
		{
			PrefabEntity* pent = (SCN::PrefabEntity*)ent;

		}
		//light entity
		else if (ent->getType() == eEntityType::LIGHT)
		{
			lights.push_back((SCN::LightEntity*)ent);
		}
	}

	generateShadowMaps();

}

void Renderer::renderScene(SCN::Scene* scene, Camera* camera)
{
	this->scene = scene;
	setupScene();

	//base_ents = scene->entities;

	//std::sort(base_ents.begin(), base_ents.end(),
	//	[](const BaseEntity* a, const BaseEntity* b)
	//	{
	//		if (a->getType() == eEntityType::PREFAB && b->getType() == eEntityType::PREFAB)
	//		{
	//			PrefabEntity* pent_a = (SCN::PrefabEntity*)a;
	//			PrefabEntity* pent_b = (SCN::PrefabEntity*)b;
	//			if (&pent_a->root.distance_to_camera < &pent_b->root.distance_to_camera) return true;
	//		}
	//		return false;
	//	});

	//std::rotate(base_ents.begin(), base_ents.begin() + 2, base_ents.end());
	//std::rotate(base_ents.begin(), base_ents.begin() + 3, base_ents.end());

	
	renderFrame(scene, camera);

	if (show_shadowmaps) debugShadowMaps();

}

void Renderer::renderFrame(SCN::Scene* scene, Camera* camera)
{
	if (render_mode == eRenderMode::DEFERRED)
		renderDeferred(scene, camera);
	else 
		renderForward(scene, camera);
}

void Renderer::renderDeferred(SCN::Scene* scene, Camera* camera)
{		
	vec2 size = CORE::getWindowSize();

	//generate gbudder
	if (!gbuffer_fbo)
	{
		gbuffer_fbo = new GFX::FBO();
		gbuffer_fbo->create(size.x, size.y, 3, GL_RGBA, GL_UNSIGNED_BYTE, true);
	}

	//render inside the fbo all that is in the bind 
	gbuffer_fbo->bind();

		//gbuffer_fbo->enableBuffers(true, false, false, false);
		glClearColor(scene->background_color.x, scene->background_color.y, scene->background_color.z, 1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//gbuffer_fbo->enableAllBuffers();
		camera->enable();
		renderSceneNodes(scene, camera);

	gbuffer_fbo->unbind();

	camera->enable();
	//to clear the scene
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glClearColor(scene->background_color.x, scene->background_color.y, scene->background_color.z, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (skybox_cubemap) renderSkybox(skybox_cubemap);

	GFX::Mesh* mesh = GFX::Mesh::getQuad();
	GFX::Shader* shader = GFX::Shader::Get("deferred_global");

	shader->enable();
	shader->setTexture("u_albedo_texture", gbuffer_fbo->color_textures[0], 0);
	shader->setTexture("u_normal_texture", gbuffer_fbo->color_textures[1], 1);
	shader->setTexture("u_emissive_texture", gbuffer_fbo->color_textures[2], 2);
	shader->setTexture("u_depth_texture", gbuffer_fbo->depth_texture, 3);
	shader->setUniform("u_ambient_light", scene->ambient_light);
	
	mesh->render(GL_TRIANGLES);

	//shader = GFX::Shader::Get("deferred_light");
	//
	//shader->enable();
	//shader->setTexture("u_albedo_texture", gbuffer_fbo->color_textures[0], 0);
	//shader->setTexture("u_normal_texture", gbuffer_fbo->color_textures[1], 1);
	//shader->setTexture("u_emissive_texture", gbuffer_fbo->color_textures[2], 2);
	//shader->setTexture("u_depth_texture", gbuffer_fbo->depth_texture, 3);

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	for (auto light : lights)
	{
		shader->setUniform("u_iRes", vec2(1.0 / size.x, 1.0 / size.y));
		shader->setUniform("u_ivp", camera->inverse_viewprojection_matrix);
		lightToShader(light, shader);
		mesh->render(GL_TRIANGLES);
	}

	glDisable(GL_BLEND);


	if (show_gbuffers)
	{
		//albedo
		glViewport(0, size.y / 2, size.x / 2, size.y / 2);
		gbuffer_fbo->color_textures[0]->toViewport(); 
		//normal
		glViewport(size.x / 2, size.y / 2, size.x / 2, size.y / 2);
		gbuffer_fbo->color_textures[1]->toViewport();
		glViewport(0, 0, size.x / 2, size.y / 2);
		//emissive
		gbuffer_fbo->color_textures[2]->toViewport(); 
		glViewport(size.x / 2, 0, size.x / 2, size.y / 2);
		//depth
		GFX::Shader* shader = GFX::Shader::getDefaultShader("linear_depth");
		shader->enable();
		shader->setUniform("u_camera_nearfar", vec2(camera->near_plane, camera->far_plane));
		gbuffer_fbo->depth_texture->toViewport(shader);
		glViewport(0, 0, size.x, size.y);
	}

	//compute illuminaiton

}

void Renderer::renderForward(SCN::Scene* scene, Camera* camera)
{
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);

	//set the camera as default (used by some functions in the framework)
	camera->enable();

	//set the clear color (the background color)
	glClearColor(scene->background_color.x, scene->background_color.y, scene->background_color.z, 1.0);

	// Clear the color and the depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	GFX::checkGLErrors();

	//render skybox
	if (skybox_cubemap && render_mode != eRenderMode::FLAT)	renderSkybox(skybox_cubemap);

	renderSceneNodes(scene, camera);
}

void Renderer::renderSceneNodes(SCN::Scene* scene, Camera* camera)
{
	//render entities
	for (int i = 0; i < scene->entities.size(); ++i)
	{
		BaseEntity* ent = scene->entities[i];
		if (!ent->visible)	continue;

		if (ent->getType() == eEntityType::PREFAB)
		{
			PrefabEntity* pent = (SCN::PrefabEntity*)ent;
			renderNode(&pent->root, camera);
		}
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

//renders a node of the prefab and its children
void Renderer::renderNode(SCN::Node* node, Camera* camera)
{
	if (!node->visible)
		return;

	//compute global matrix
	Matrix44 node_model = node->getGlobalMatrix(true);

	//does this node have a mesh? then we must render it
	if (node->mesh && node->material)
	{
		//compute the bounding box of the object in world space (by using the mesh bounding box transformed to world space)
		BoundingBox world_bounding = transformBoundingBox(node_model,node->mesh->box);
		
		//if bounding box is inside the camera frustum then the object is probably visible
		if (camera->testBoxInFrustum(world_bounding.center, world_bounding.halfsize) )
		{
			//switch between render modes
			if(render_boundaries)
				node->mesh->renderBounding(node_model, true);
			switch (render_mode)
			{			
				case eRenderMode::FLAT: renderMeshWithMaterialFlat(node_model, node->mesh, node->material); break;
				case eRenderMode::TEXTURED: renderMeshWithMaterial(node_model, node->mesh, node->material); break;
				case eRenderMode::LIGHTS: renderMeshWithMaterialLight(node_model, node->mesh, node->material); break;
				case eRenderMode::DEFERRED: renderMeshWithMaterialGBuffers(node_model, node->mesh, node->material); break;

			}
		}
	}

	//iterate recursively with children
	for (int i = 0; i < node->children.size(); ++i)
	{
		renderNode(node->children[i], camera);
	}
}

//renders a mesh given its transform and material texture
void Renderer::renderMeshWithMaterial(Matrix44 model, GFX::Mesh* mesh, SCN::Material* material)
{
	//in case there is nothing to do
	if (!mesh || !mesh->getNumVertices() || !material )	return;
    assert(glGetError() == GL_NO_ERROR);

	//define locals to simplify coding
	GFX::Shader* shader = NULL;
	Camera* camera = Camera::current;
	GFX::Texture* white = GFX::Texture::getWhiteTexture();

	GFX::Texture* albedo_texture = material->textures[SCN::eTextureChannel::ALBEDO].texture;

	if (albedo_texture == NULL) albedo_texture = white; //a 1x1 white texture

	//select the blending
	if (material->alpha_mode == SCN::eAlphaMode::BLEND)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	else glDisable(GL_BLEND);

	//select if render both sides of the triangles
	if(material->two_sided)	glDisable(GL_CULL_FACE);
	else glEnable(GL_CULL_FACE);
    assert(glGetError() == GL_NO_ERROR);

	glEnable(GL_DEPTH_TEST);

	//chose a shader
	shader = GFX::Shader::Get("texture");

    assert(glGetError() == GL_NO_ERROR);

	//no shader? then nothing to render
	if (!shader) return;
	shader->enable();

	//upload uniforms
	shader->setUniform("u_model", model);
	cameraToShader(camera, shader);
	float t = getTime();
	shader->setUniform("u_time", t );

	shader->setUniform("u_albedo_factor", material->color);
	shader->setUniform("u_albedo_texture", albedo_texture ? albedo_texture : white, 0);

	//this is used to say which is the alpha threshold to what we should not paint a pixel on the screen (to cut polygons according to texture alpha)
	shader->setUniform("u_alpha_cutoff", material->alpha_mode == SCN::eAlphaMode::MASK ? material->alpha_cutoff : 0.001f);

	if (render_wireframe) glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

	//do the draw call that renders the mesh into the screen
	mesh->render(GL_TRIANGLES);

	//disable shader
	shader->disable();

	//set the render state as it was before to avoid problems with future renders
	glDisable(GL_BLEND);
	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
}

//renders a mesh given its transform and material 
void Renderer::renderMeshWithMaterialFlat(Matrix44 model, GFX::Mesh* mesh, SCN::Material* material)
{
	//in case there is nothing to do
	if (!mesh || !mesh->getNumVertices() || !material)	return;
	assert(glGetError() == GL_NO_ERROR);

	//define locals to simplify coding
	GFX::Shader* shader = NULL;
	Camera* camera = Camera::current;

	//select the blending
	if (material->alpha_mode == SCN::eAlphaMode::BLEND) return;
	glDisable(GL_BLEND);

	//select if render both sides of the triangles
	if (material->two_sided)	glDisable(GL_CULL_FACE);
	else glEnable(GL_CULL_FACE);
	assert(glGetError() == GL_NO_ERROR);

	glEnable(GL_DEPTH_TEST);

	//chose a shader
	shader = GFX::Shader::Get("flat");

	assert(glGetError() == GL_NO_ERROR);

	//no shader? then nothing to render
	if (!shader) return;
	shader->enable();

	//upload uniforms
	shader->setUniform("u_model", model);
	cameraToShader(camera, shader);

	//do the draw call that renders the mesh into the screen
	mesh->render(GL_TRIANGLES);

	//disable shader
	shader->disable();
}

//renders a mesh given its transform and material, lights adn shadows
void Renderer::renderMeshWithMaterialLight(const Matrix44 model, GFX::Mesh* mesh, SCN::Material* material)
{
	//in case there is nothing to do
	if (!mesh || !mesh->getNumVertices() || !material) return;
	assert(glGetError() == GL_NO_ERROR);

	//define locals to simplify coding
	GFX::Shader* shader = NULL;
	Camera* camera = Camera::current;
	GFX::Texture* white = GFX::Texture::getWhiteTexture();

	GFX::Texture* albedo_texture = material->textures[SCN::eTextureChannel::ALBEDO].texture;
	GFX::Texture* emissive_texture = material->textures[SCN::eTextureChannel::EMISSIVE].texture;
	GFX::Texture* normal_texture = material->textures[SCN::eTextureChannel::NORMALMAP].texture;
	//GFX::Texture* metallic_texture = material->textures[SCN::eTextureChannel::METALLIC_ROUGHNESS].texture;
	//GFX::Texture* occlusion_texture = material->textures[SCN::eTextureChannel::OCCLUSION].texture; //metallic.r

	if (albedo_texture == NULL) albedo_texture = white; //a 1x1 white texture

	//select the blending
	if (material->alpha_mode == SCN::eAlphaMode::BLEND)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	else glDisable(GL_BLEND);

	//select if render both sides of the triangles
	if (material->two_sided) glDisable(GL_CULL_FACE);
	else glEnable(GL_CULL_FACE);
	assert(glGetError() == GL_NO_ERROR);

	glEnable(GL_DEPTH_TEST);

	//chose a shader
	shader = GFX::Shader::Get("light");

	assert(glGetError() == GL_NO_ERROR);

	//no shader? then nothing to render
	if (!shader) return;
	shader->enable();

	//upload uniforms
	shader->setUniform("u_model", model);
	cameraToShader(camera, shader);
	float t = getTime();
	shader->setUniform("u_time", t);

	shader->setUniform("u_albedo_factor", material->color);
	shader->setUniform("u_emissive_factor", material->emissive_factor);
	//shader->setUniform("u_metallic_factor", material->metallic_factor);
	//shader->setUniform("u_roughness_factor", material->roughness_factor);
	shader->setUniform("u_albedo_texture", albedo_texture ? albedo_texture : white, 0);
	shader->setUniform("u_emissive_texture", emissive_texture ? emissive_texture : white, 1);
	shader->setUniform("u_normal_texture", normal_texture ? normal_texture : white, 2);
	//shader->setUniform("u_metallic_texture", metallic_texture ? metallic_texture : white, 3);

	//this is used to say which is the alpha threshold to what we should not paint a pixel on the screen (to cut polygons according to texture alpha)
	shader->setUniform("u_alpha_cutoff", material->alpha_mode == SCN::eAlphaMode::MASK ? material->alpha_cutoff : 0.001f);
	shader->setUniform("u_ambient_light", scene->ambient_light);

	if (render_wireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glDepthFunc(GL_LEQUAL); //draw pixels if depth is less or equal to camera

	if (lights.size() == 0)
	{
		shader->setUniform("u_light_type", 0);
		mesh->render(GL_TRIANGLES);
	}
	else
	{
		for (int i = 0; i < lights.size(); i++)
		{
			LightEntity* light = lights[i];

			if (light->visible)
			{
				lightToShader(light, shader);

				mesh->render(GL_TRIANGLES);

				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);

				shader->setUniform("u_emissive_factor", vec3(0.0));
				shader->setUniform("u_metallic_factor", vec3(0.0));
				shader->setUniform("u_ambient_light", vec3(0.0));

			}
		}
	}

	//do the draw call that renders the mesh into the screen	
	mesh->render(GL_TRIANGLES);

	//disable shader
	shader->disable();

	//set the render state as it was before to avoid problems with future renders
	glDisable(GL_BLEND);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDepthFunc(GL_LESS);
}

//renders a mesh given its transform and material with gbffers
void Renderer::renderMeshWithMaterialGBuffers(Matrix44 model, GFX::Mesh* mesh, SCN::Material* material)
{
	//in case there is nothing to do
	if (!mesh || !mesh->getNumVertices() || !material) return;
	assert(glGetError() == GL_NO_ERROR);

	if (material->alpha_mode == eAlphaMode::BLEND) return;

	//define locals to simplify coding
	GFX::Shader* shader = NULL;
	Camera* camera = Camera::current;
	GFX::Texture* white = GFX::Texture::getWhiteTexture();

	GFX::Texture* albedo_texture = material->textures[SCN::eTextureChannel::ALBEDO].texture;
	GFX::Texture* emissive_texture = material->textures[SCN::eTextureChannel::EMISSIVE].texture;
	//implement normal, metal, etc

	if (albedo_texture == NULL) albedo_texture = white; //a 1x1 white texture

	
	else glDisable(GL_BLEND);

	//select if render both sides of the triangles
	if (material->two_sided) glDisable(GL_CULL_FACE);
	else glEnable(GL_CULL_FACE);
	assert(glGetError() == GL_NO_ERROR);

	glEnable(GL_DEPTH_TEST);

	//chose a shader
	shader = GFX::Shader::Get("gbuffers");

	assert(glGetError() == GL_NO_ERROR);

	//no shader? then nothing to render
	if (!shader) return;
	shader->enable();

	//upload uniforms
	shader->setUniform("u_model", model);
	cameraToShader(camera, shader);
	float t = getTime();
	shader->setUniform("u_time", t);

	shader->setUniform("u_albedo_factor", material->color);
	shader->setUniform("u_emissive_factor", material->emissive_factor);
	shader->setUniform("u_albedo_texture", albedo_texture ? albedo_texture : white, 0);
	shader->setUniform("u_emissive_texture", emissive_texture ? emissive_texture : white, 1);

	//this is used to say which is the alpha threshold to what we should not paint a pixel on the screen (to cut polygons according to texture alpha)
	shader->setUniform("u_alpha_cutoff", material->alpha_mode == SCN::eAlphaMode::MASK ? material->alpha_cutoff : 0.001f);

	if (render_wireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	//do the draw call that renders the mesh into the screen
	mesh->render(GL_TRIANGLES);

	//disable shader
	shader->disable();

	//set the render state as it was before to avoid problems with future renders
	glDisable(GL_BLEND);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}


void SCN::Renderer::cameraToShader(Camera* camera, GFX::Shader* shader)
{
	shader->setUniform("u_viewprojection", camera->viewprojection_matrix );
	shader->setUniform("u_camera_position", camera->eye);
}

void SCN::Renderer::lightToShader(LightEntity* light, GFX::Shader* shader)
{
	shader->setUniform("u_light_position", light->root.model.getTranslation()); //for point and spot
	shader->setUniform("u_light_front", light->root.model.rotateVector(vec3(0, 0, 1))); //for spot and directional
	shader->setUniform("u_light_color", light->color * light->intensity);
	shader->setUniform("u_light_info", vec4((int)light->light_type, light->near_distance, light->max_distance, 0)); //0 as a place holder for another porperty
	shader->setUniform("u_light_cone", vec2(cos(light->cone_info.x * DEG2RAD), cos(light->cone_info.y * DEG2RAD))); //cone for the spot light

	shader->setUniform("u_shadow_param", vec2(light->shadowmap ? 1 : 0, light->shadow_bias));
	if (light->shadowmap)
	{
		shader->setUniform("u_shadowmap", light->shadowmap, 8); //use one of the last slots (16 max)
		shader->setUniform("u_shadow_viewproj", light->shadow_viewproj);
	}
}

#ifndef SKIP_IMGUI

void Renderer::showUI()
{
	ImGui::Checkbox("Wireframe", &render_wireframe);
	ImGui::Checkbox("Boundaries", &render_boundaries);
	ImGui::Checkbox("Show ShadowMaps", &show_shadowmaps);
	ImGui::Checkbox("Show Gbuffers", &show_gbuffers);

	ImGui::Combo("Render Mode", (int*)&render_mode, "TEXTURED\0LIGHTS\0DEFERRED" , 3);

	//add here your stuff
	//...
}

void Renderer::generateShadowMaps()
{
	Camera camera;

	GFX::startGPULabel("Shadowmaps");

	eRenderMode prev = render_mode;
	render_mode = eRenderMode::FLAT;

	for (auto light : lights)
	{
		if (!light->cast_shadows) continue;

		if (light->light_type == eLightType::POINT) continue;

		//TODO: check if light is inside camera

		if (!light->shadowmap_fbo) //build shadowmap fbo if we dont have one
		{
			int size{};
			if (light->light_type == eLightType::SPOT) size = 1024;
			if (light->light_type == eLightType::DIRECTIONAL) size = 15000;

			light->shadowmap_fbo = new GFX::FBO();
			light->shadowmap_fbo->setDepthOnly(size, size);
			light->shadowmap = light->shadowmap_fbo->depth_texture;
		}

		//create camera
		vec3 pos = light->root.model.getTranslation();
		vec3 front = light->root.model.rotateVector(vec3(0, 0, -1));
		vec3 up = vec3(0, 1, 0);
		camera.lookAt(pos, pos + front, up);

		if (light->light_type == eLightType::SPOT)
		{
			camera.setPerspective(light->cone_info.y * 2, 1.0, light->near_distance, light->max_distance);
		}
		if (light->light_type == eLightType::DIRECTIONAL)
		{
			camera.setOrthographic(-1000.0, 1000.0, -1000.0, 1000.0, light->near_distance, light->max_distance);
		}		

		light->shadowmap_fbo->bind(); //everything we render until the unbind will be inside this texture

		renderFrame(scene, &camera);

		light->shadowmap_fbo->unbind();

		light->shadow_viewproj = camera.viewprojection_matrix;
	}

	render_mode = prev;

	GFX::endGPULabel();
}

void Renderer::debugShadowMaps()
{
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	int x = 310;
	for (auto light : lights)
	{
		if (!light->shadowmap) continue;

		GFX::Shader* shader = GFX::Shader::getDefaultShader("linear_depth");
		shader->enable();
		shader->setUniform("u_camera_nearfar", vec2(light->near_distance, light->max_distance));
		
		glViewport(x, 10, 256, 256);

		light->shadowmap->toViewport(shader);
		
		x += 260;
	}

	vec2 size = CORE::getWindowSize();
	glViewport(0, 0, size.x, size.y);

}

float Renderer::distance(vec3* v1, vec3* v2)
{
	float vx = (v1->x - v2->x) * (v1->x - v2->x);
	float vy = (v1->y - v2->y) * (v1->y - v2->y);
	float vz = (v1->z - v2->z) * (v1->z - v2->z);

	float dist = sqrt(vx + vy + vz);
	return dist;
}

#else
void Renderer::showUI() {}
#endif