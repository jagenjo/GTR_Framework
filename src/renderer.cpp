#include "renderer.h"

#include "camera.h"
#include "shader.h"
#include "mesh.h"
#include "texture.h"
#include "prefab.h"
#include "material.h"
#include "utils.h"
#include "scene.h"
#include "extra/hdre.h"
#include <random>
#include "framework.h"
#include "application.h"

using namespace GTR;

GTR::Renderer::Renderer()
{
	this->render_mode = eRenderMode::MULTI;
	this->rendering_shadowmap = TRUE;
	this->pipeline_mode = ePipelineMode::FORWARD;
	this->show_gbuffers = false;

	color_buffer = new Texture(Application::instance->window_width, Application::instance->window_height);
	this->fbo.setTexture(color_buffer); // para evitar de hacerlo en cada frame 
	
}

// render in texture
void Renderer::render2FBO(GTR::Scene* scene, Camera* camera) {
	renderScene(scene, camera);
	//drawGrid();
	//fbo.bind();
	//drawGrid(); Cuando tengo que dibujar el grid??? no es al final de renderizar todo? 

	//fbo.unbind();

	//color_buffer->toViewport();

	
}


struct sortRC {
	inline bool operator()(RenderCall& a, RenderCall& b) const {

		//sort the rc through distance more to less if the material is the type BLEND
		if ((a.material->alpha_mode == GTR::eAlphaMode::BLEND) && (b.material->alpha_mode == GTR::eAlphaMode::BLEND))
			return a.dist2camera > b.dist2camera;
		//if the material is the type OPAQUE OR OTHERS, we sort it less to more
		else if ((a.material->alpha_mode != GTR::eAlphaMode::BLEND) && (b.material->alpha_mode != GTR::eAlphaMode::BLEND))
			return a.dist2camera < b.dist2camera;
		//if don't comply above conditions, we still sort it by the type of the material. If is type BLEND, sort it at the end 
		return a.material->alpha_mode < b.material->alpha_mode;

	}
};


void Renderer::renderScene(GTR::Scene* scene, Camera* camera)
{
	
	collectRenderCalls(scene, camera);
	//sort each rcs after rendering one pass of all the scene
	std::sort(this->rc_data_list.begin(), this->rc_data_list.end(), sortRC());

	
	if (pipeline_mode == FORWARD) 
		renderForward(scene, this->rc_data_list, camera);
		
	else if (pipeline_mode == DEFERRED)
		renderDeferred(scene, this->rc_data_list, camera);

	
	
}

//collect all RC
void Renderer::collectRenderCalls(GTR::Scene* scene, Camera* camera) {

	//clear data_lists
	this->rc_data_list.resize(0); //like .clear but keep the capacity, so there are fewer reallocations.
	this->light_entities.resize(0);

	//render entities
	for (int i = 0; i < scene->entities.size(); ++i)
	{
		BaseEntity* ent = scene->entities[i];
		if (!ent->visible)
			continue;

		//is a prefab!
		if (ent->entity_type == PREFAB)
		{
			PrefabEntity* pent = (GTR::PrefabEntity*)ent; //down-cast 
			if (pent->prefab)

				getRCsfromPrefab(ent->model, pent->prefab, camera);

		}

		//is a light!
		else if (ent->entity_type == LIGHT)
		{
			LightEntity* lig = (GTR::LightEntity*)ent; //down-cast 

			// if light is not in the fustrum of the camera, we don't add it to the conteiner
			// directional light affect all the places so we will add it
			Vector3 light_pos = lig->model.getTranslation();
			if (lig->light_type != eLightType::DIRECTIONAL && camera->testSphereInFrustum(light_pos, lig->max_dist) == CLIP_OUTSIDE)
				continue;

			this->light_entities.push_back(lig);


		}
	}
}


void GTR::Renderer::renderForward(GTR::Scene* scene, std::vector <RenderCall>& rendercalls, Camera* camera)
{

	//set the clear color (the background color)
	glClearColor(scene->background_color.x, scene->background_color.y, scene->background_color.z, 1.0);
	// Clear the color and the depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	checkGLErrors();

	//render RenderCalls through reference 
	for (int i = 0; i < rendercalls.size(); i++)
	{
		RenderCall& rc = rendercalls[i];
		renderMeshWithMaterial(this->render_mode, rc.model, rc.mesh, rc.material, camera);
	}

}

void GTR::Renderer::renderDeferred(GTR::Scene* scene, std::vector <RenderCall>& rendercalls, Camera* camera)
{
	int width = Application::instance->window_width;
	int height = Application::instance->window_height;

	if (gbuffers_fbo.fbo_id == 0) { //this att tell me if it is already created (by default is 0). Memory is reserved?

		//we reserve memory to each textures...
		gbuffers_fbo.create(width, 
							height,
							3, // num of textures to create
							GL_RGBA, // four channels
							GL_UNSIGNED_BYTE, //1byte
							true); //add depth_texture
	}
	//start rendeing inside the gbuffers
	gbuffers_fbo.bind(); 

	// if we want to clear all in once
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	checkGLErrors();



	/* //if clear each GB independently
	//Now we clear in several passes, so we can control the clear color independently for every gbuffer
	//disable all but the GB0 (and the depth) 
	gbuffers_fbo.enableSingleBuffer(0);

	//clear the 1º GB with the color (and depth)
	glClearColor( 0.1 , 0.1 , 0.1 , 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	checkGLErrors();

	//now enable the 2º GB and clear. This time we haven't clear the GL_COLOR
	gbuffers_fbo.enableSingleBuffer(1);
	glClearColor( 0, 0, 0, 1.0);
	glClear(GL_DEPTH_BUFFER_BIT);
	checkGLErrors();

	// clear 3º GB

	// clear 4º GB

	//enable all buffers back
	gbuffers_fbo.enableAllBuffers();


	*/


	//render all what we want
	for (int i = 0; i < rendercalls.size(); i++)
	{
		RenderCall& rc = rendercalls[i];
		// solo queremos que coja los shaders de Gbuffers
		// no quiero cambiar modo de render -> ahora always este modo
		renderMeshWithMaterial(eRenderMode::GBUFFERS, rc.model, rc.mesh, rc.material, camera);
	}

	//stop rendering to the gbuffers
	gbuffers_fbo.unbind();

	//clear 
	//glClearColor(0, 0, 0, 0);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//desactivo los flags
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	
	//---------Ilumination_Pass--------------
	//create and FBO
	
	if (illumination_fbo.fbo_id == 0) {

		//create 3 textures of 4 components
		illumination_fbo.create(width, 
								height,
								1, 			//three textures
								GL_RGB, 		//three channels
								GL_UNSIGNED_BYTE, //1 byte
								false);		//add depth_texture

	}

	illumination_fbo.bind();

	// if we want to clear all in once
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	checkGLErrors();

	
	Mesh* quad = Mesh::getQuad(); 
	Shader* shader = Shader::Get("deferred");
	shader->enable();
	shader->setTexture("u_color_texture", gbuffers_fbo.color_textures[0], 0);
	shader->setTexture("u_normal_texture", gbuffers_fbo.color_textures[1], 1);
	shader->setTexture("u_extra_texture", gbuffers_fbo.color_textures[2], 2);
	shader->setTexture("u_depth_texture", gbuffers_fbo.depth_texture, 3);
	shader->setUniform("u_ambient_light", scene->ambient_light);
	

	Matrix44 inv_vp = camera->viewprojection_matrix;
	inv_vp.inverse();
	shader->setUniform("u_inverse_viewprojection", inv_vp);

	LightEntity* light;
	for (int i = 0; i < this->light_entities.size(); i++)
	{
		light = this->light_entities[i];
		//we assume that there is always at least one directional ///luego si da tiempo corregir para el caso de no directional light
		if (light->light_type == DIRECTIONAL) {
			light->uploadToShader(shader);
		
			quad->render(GL_TRIANGLES);

			//in case there are more than one directional light:
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE);
			shader->setUniform("u_ambient_light", Vector3(0, 0, 0));
		}
		
			
	}
	
	//glDisable(GL_BLEND);

	//using geomrtry
	// 
	//we can use a sphere mesh for point lights
	Mesh* sphere = Mesh::Get("data/meshes/sphere.obj", false, false);

	glDisable(GL_CULL_FACE); 
	
	//this deferred_ws shader uses the basic.vs instead of quad.vs
	shader = Shader::Get("deferred_ws");

	shader->enable();
	shader->setTexture("u_color_texture", gbuffers_fbo.color_textures[0], 0);
	shader->setTexture("u_normal_texture", gbuffers_fbo.color_textures[1], 1);
	shader->setTexture("u_extra_texture", gbuffers_fbo.color_textures[2], 2);
	shader->setTexture("u_depth_texture", gbuffers_fbo.depth_texture, 3);
	
	//basic.vs will need the model and the viewproj of the camera
	shader->setUniform("u_viewprojection", camera->viewprojection_matrix);
	shader->setUniform("u_inverse_viewprojection", inv_vp);
	shader->setUniform("u_iRes", Vector2(1.0 / (float)width, 1.0 / (float)height));
	shader->setUniform("u_camera_position", camera->eye );


	Matrix44 m; Vector3 pos; 
	for (int i = 0; i < this->light_entities.size(); i++)
	{
		light = this->light_entities[i];
		if (light->light_type == DIRECTIONAL)
			continue;
		//we must translate the model to the center of the light
		// and scale it according to the max_distance of the light
		pos = light->model.getTranslation();
		m.setTranslation(pos.x, pos.y, pos.z);
		m.scale(light->max_dist, light->max_dist, light->max_dist);
		shader->setUniform("u_model", m); //pass the model to render the sphere

		light->uploadToShader(shader);
		glFrontFace(GL_CW);
		sphere->render(GL_TRIANGLES);
		
		//glEnable(GL_BLEND);

		//only pixels behind a surface are rendered
		//glEnable(GL_DEPTH_TEST); //no s donde poner

		glDepthFunc(GL_GREATER);
		glBlendFunc(GL_ONE, GL_ONE);// sum each pixels with the befors...
	}


	//stop rendering to the fbo, render to screen
	illumination_fbo.unbind();

	//set to back //be sure blending is not active
	glFrontFace(GL_CCW);
	glDisable(GL_BLEND);

	//and render the texture into the screen
	illumination_fbo.color_textures[0]->toViewport();


	
	



	//to plot every textures in the viewport
	//gbuffers_fbo.color_textures[0]->toViewport(Shader::Get("showAlpha"));// pq puedo llamar directamente el shader?
	if (show_gbuffers){

		//int width = Application::instance->window_width;
		//int height = Application::instance->window_height;

		//GB0 color
		glViewport(0, 0, width * 0.5, height * 0.5);
		gbuffers_fbo.color_textures[0]->toViewport();

		//GB1 normal
		glViewport(width * 0.5, 0, width * 0.5, height * 0.5);
		gbuffers_fbo.color_textures[1]->toViewport();

		//GB2 material. properties
		glViewport( width*0.5, height * 0.5, width * 0.5, height * 0.5);
		gbuffers_fbo.color_textures[2]->toViewport();

		//GB3 depth_buffer
		glViewport(0, height * 0.5, width * 0.5, height * 0.5);
		//need to pass a linear with shader depth, to be able to see. No se ve-> pq necesita parm
		// para linealizar necesito el near and far de la camara.
		Shader* depth_sh = Shader::Get("depth");
		depth_sh->enable();
		depth_sh->setUniform("u_camera_nearfar", Vector2(camera->near_plane, camera->far_plane));
		//depth_sh->disable();
		gbuffers_fbo.depth_texture->toViewport(depth_sh);

		//Volver a poner el tamaño de VPort. 0,0 en una textura esta abajo iz!
		glViewport(0, 0, width, height);
	}





}

//renders all the prefab
void Renderer::getRCsfromPrefab(const Matrix44& model, GTR::Prefab* prefab, Camera* camera)
{
	assert(prefab && "PREFAB IS NULL");
	//assign the model to the root node

	getRCsfromNode(model, &prefab->root, camera);
	
}

//renders a node of the prefab and its children
void Renderer::getRCsfromNode(const Matrix44& prefab_model, GTR::Node* node, Camera* camera)
{
	if (!node->visible)
		return;

	//compute global matrix
	Matrix44 node_model = node->getGlobalMatrix(true) * prefab_model;

	
	//does this node have a mesh? then we must render it
	if (node->mesh && node->material)
	{
	
		//compute the bounding box of the object in world space (by using the mesh bounding box transformed to world space)
		BoundingBox world_bounding = transformBoundingBox(node_model,node->mesh->box);
		
		//if bounding box is inside the camera frustum then the object is probably visible
		if (camera->testBoxInFrustum(world_bounding.center, world_bounding.halfsize) )
		{
			//instance each rc
			RenderCall rc;
			rc.model = node_model;
			rc.material = node->material;
			rc.mesh = node->mesh;
			rc.dist2camera = camera->eye.distance(world_bounding.center);
			this->rc_data_list.push_back(rc);
			
			//node->mesh->renderBounding(node_model, true);
		}
	}

	//iterate recursively with children
	for (int i = 0; i < node->children.size(); ++i)
		getRCsfromNode(prefab_model, node->children[i], camera);
	
}


//renders a mesh given its transform and material
void Renderer::renderMeshWithMaterial(eRenderMode mode, const Matrix44 model, Mesh* mesh, GTR::Material* material, Camera* camera)
{
	//in case there is nothing to do
	if (!mesh || !mesh->getNumVertices() || !material )
		return;
    assert(glGetError() == GL_NO_ERROR);

	//flag para deffered en materiales con transparencias.
	if (mode == GBUFFERS && material->alpha_mode == GTR::eAlphaMode::BLEND)
		return; // luego cambiar con reticula..., usar disering??

	//define locals to simplify coding
	Shader* shader = NULL;
	GTR::Scene* scene = GTR::Scene::instance;

	//create and load texture 
	Texture* texture = NULL;
	Texture* em_texture = NULL;
	Texture* mr_texture = NULL;
	Texture* oc_texture = NULL;
	Texture* n_texture = NULL;

	texture = material->color_texture.texture;
	em_texture = material->emissive_texture.texture;
	mr_texture = material->metallic_roughness_texture.texture;
	oc_texture = material->occlusion_texture.texture;
	n_texture = material->normal_texture.texture;


	if (texture == NULL)
		texture = Texture::getWhiteTexture(); //a 1x1 white texture
	if (em_texture == NULL)
		em_texture = Texture::getWhiteTexture();
	if (mr_texture == NULL)
		mr_texture = Texture::getWhiteTexture();
	if (oc_texture == NULL)
		oc_texture = Texture::getWhiteTexture();
	if (n_texture == NULL)
		n_texture = Texture::getWhiteTexture();


	//select if render both sides of the triangles
	if(material->two_sided)
		glDisable(GL_CULL_FACE); 
	else
		glEnable(GL_CULL_FACE);
    assert(glGetError() == GL_NO_ERROR); 

	//select shader with respect to the mode 
	if (mode == SHOW_TEXTURE)
		shader = Shader::Get("texture");

	else if (mode == SINGLE)
		shader = Shader::Get("light_singlepass");

	else if (mode ==  MULTI)
		shader = Shader::Get("light");

	else if (mode == SHOW_NORMAL) {
		shader = Shader::Get("sh2debug");
		shader->enable();
		shader->setUniform("u_texture_type", 0);
	}
	else if (mode == SHOW_OC) {
		shader = Shader::Get("sh2debug");
		shader->enable();
		shader->setUniform("u_texture_type", 1);
	}
	else if (mode == SHOW_UVS) {
		shader = Shader::Get("sh2debug");
		shader->enable();
		shader->setUniform("u_texture_type", 2);
	}
	
	else if (mode == GBUFFERS) 
		shader = Shader::Get("gbuffers");
	

	if (!shader)//no shader? then nothing to render
		return;

	shader->enable();

	assert(glGetError() == GL_NO_ERROR);

	//upload uniforms
	shader->setUniform("u_viewprojection", camera->viewprojection_matrix);
	shader->setUniform("u_camera_position", camera->eye);
	shader->setUniform("u_model", model );
	//float t = getTime(); shader->setUniform("u_time", t);
	shader->setUniform("u_emissive_factor", material->emissive_factor);
	shader->setUniform("u_color", material->color);

	//upload textures
	if(texture)
		shader->setTexture("u_color_texture", texture, 0);
	if(em_texture)
		shader->setTexture("u_emissive_texture", em_texture, 1);
	if(mr_texture )
		shader->setTexture("u_metallic_roughness_texture", mr_texture, 2);
	if (oc_texture)
		shader->setTexture("u_occlusion_texture", oc_texture, 3);
	if (n_texture)
		shader->setTexture("u_normal_texture", n_texture, 4);


	//this is used to say which is the alpha threshold to what we should not paint a pixel on the screen (to cut polygons according to texture alpha)
	shader->setUniform("u_alpha_cutoff", material->alpha_mode == GTR::eAlphaMode::MASK ? material->alpha_cutoff : 0);

	glDepthFunc(GL_LEQUAL); //paints the pixels if it is LESS OR EQUAL of Zdepth
	shader->setUniform("u_ambient_light", scene->ambient_light);//? ...

	//select the blending. Solo para las luces.
	if (material->alpha_mode == GTR::eAlphaMode::BLEND)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	else
		glDisable(GL_BLEND);
	
	/*if (mode != GTR::eRenderMode::SINGLE || mode != GTR::eRenderMode::MULTI) {
		mesh->render(GL_TRIANGLES);
		std::cout << "Hello World!";
		return; 
	}*/
	
	if (mode == GTR::eRenderMode::SINGLE || mode == GTR::eRenderMode::MULTI) {
		
		renderlights(mode, shader, mesh, material);
		
		shader->disable();
		glDisable(GL_BLEND);
		
		return;
	}

	mesh->render(GL_TRIANGLES);
	
	shader->disable();
	//set the render state as it was before to avoid problems with future renders
	glDisable(GL_BLEND);

}


void Renderer::renderlights(eRenderMode mode, Shader* shader, Mesh* mesh, GTR::Material* material) {
	
	if (!shader)
		return;
	
	if (mode == eRenderMode::MULTI) {

		for (int i = 0; i < this->light_entities.size(); ++i) {

			LightEntity* light = this->light_entities[i];

			// first pass we don't use blending
			if (i == 0 && material->alpha_mode != BLEND)
			{
				glDisable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			}
			else {
				glEnable(GL_BLEND);//enable blending and add the pixels to the previous ones
				glDepthFunc(GL_LEQUAL);//paints the pixels if it is LESS OR EQUAL of Zdepth
				glBlendFunc(GL_ONE, GL_ONE);

			}
			//pass the lights data to the shader
			light->uploadToShader(shader);
			mesh->render(GL_TRIANGLES);
			
			//only one pass ambient light and emissive light
			shader->setUniform("u_ambient_light", Vector3(0, 0, 0));
			shader->setUniform("u_emissive_factor", Vector3(0, 0, 0));

		} // loop of multipass

		glDisable(GL_BLEND);
		glDepthFunc(GL_LESS);
		
		return; //we put return, to go out when it finish!
	} // flag of multipass
	
	if (mode == eRenderMode::SINGLE) {
		std::vector <LightEntity*> lights = this->light_entities;
		const int num_lights = 8;

		int light_type[num_lights];
		Vector3 light_color[num_lights];
		Vector3 light_position[num_lights];
		Vector3 light_direction[num_lights];
		float light_intensity[num_lights];
		float light_maxdist[num_lights];
		Vector3 light_target[num_lights];
		float light_spot_exp[num_lights];
		float light_spot_cutoff[num_lights];
		Vector3 light_fronts[num_lights];

		//fill the elements of the lists
		for (int i = 0; i < lights.size(); i++)
		{
			if (i > num_lights - 1 )
				break; //finish when the have used all posibles lights of the list
			// This is because we could have more lights [lights.size] than the ones that we want to render. Then to avoid IndexError.

			light_type[i] = lights[i]->light_type;
			light_color[i] = lights[i]->color;
			light_position[i] = lights[i]->model.getTranslation();

			light_direction[i] =  lights[i]->model.rotateVector(Vector3(0, 0, -1));

			light_intensity[i] = lights[i]->intensity;
			light_maxdist[i] = lights[i]->max_dist;

			light_fronts[i] = lights[i]->target;
			light_spot_exp[i] = lights[i]->spot_exp;
			light_spot_cutoff[i] = cosf(lights[i]->spot_cutoff * DEG2RAD);

		}
				
		shader->setUniform1("u_num_lights", num_lights);
		shader->setUniform1Array("u_light_types", (int*)&light_type, num_lights);
		shader->setUniform3Array("u_light_colors", (float*)&light_color, num_lights);
		shader->setUniform3Array("u_light_positions", (float*)&light_position, num_lights);
		shader->setUniform3Array("u_light_vectors", (float*)&light_direction, num_lights);
		shader->setUniform1Array("u_light_intensitis", (float*)&light_intensity, num_lights);
		shader->setUniform1Array("u_light_maxdists", (float*)&light_maxdist, num_lights);

		shader->setUniform3Array("u_light_fronts", (float*)&light_fronts, num_lights);
		shader->setUniform1Array("u_light_spot_exps", (float*)&light_spot_exp, num_lights);
		shader->setUniform1Array("u_light_spot_cutoffs", (float*)&light_spot_cutoff, num_lights);
		
		mesh->render(GL_TRIANGLES);


		return;
	}

}


void Renderer::render2depthbuffer(GTR::Material* material, Camera* camera) {

	/*LightEntity* light;
	for (int i = 0; i < this->light_entities.size(); ++i)
	{
		if (this->light_entities[i]->light_type == SPOT) {
			light = this->light_entities[i];
		}
	}

	//first time we create the FBO
	if (!light->shadow_fbo)
	{
		
		FBO* fbo = new FBO();
		light->shadow_fbo = fbo;
		light->shadow_fbo->setDepthOnly(1024, 1024);
	}

	//enable it to render inside the texture
	light->shadow_fbo->bind();

	//you can disable writing to the color buffer to speed up the rendering as we do not need it
	glColorMask(false, false, false, false);

	//clear the depth buffer only (don't care of color)
	glClear(GL_DEPTH_BUFFER_BIT);

	//whatever we render here will be stored inside a texture, we don't need to do anything fanzy
	
	renderPriority(this->rc_data_list, camera);


	if (material->alpha_mode == BLEND)
	{
		glDisable(GL_BLEND);
	}

	//disable it to render back to the screen
	light->shadow_fbo->unbind();

	//allow to render back to the color buffer
	glColorMask(true, true, true, true);
	
	*/
}


Texture* GTR::CubemapFromHDRE(const char* filename)
{
	HDRE* hdre = new HDRE();
	if (!hdre->load(filename))
	{
		delete hdre;
		return NULL;
	}

	/*
	Texture* texture = new Texture();
	texture->createCubemap(hdre->width, hdre->height, (Uint8**)hdre->getFaces(0), hdre->header.numChannels == 3 ? GL_RGB : GL_RGBA, GL_FLOAT );
	for(int i = 1; i < 6; ++i)
		texture->uploadCubemap(texture->format, texture->type, false, (Uint8**)hdre->getFaces(i), GL_RGBA32F, i);
	return texture;
	*/
	return NULL;
}


