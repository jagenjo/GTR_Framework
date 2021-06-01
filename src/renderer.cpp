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
	//this->rendering_shadowmap = true;
	this->pipeline_mode = ePipelineMode::DEFERRED;
	
	this->update_shadowmaps = false;

	this->color_buffer = new Texture(Application::instance->window_width, Application::instance->window_height, GL_RGB, GL_HALF_FLOAT); // 2 componentes
	this->fbo.setTexture(color_buffer); // para evitar de hacerlo en cada frame 
	this->show_gbuffers = false;

	this->show_ao = false;
	this->show_ao_deferred = false;
	this->ao_buffer = NULL;
}

// render in texture
void Renderer::render2FBO(GTR::Scene* scene, Camera* camera) {
	
	
	renderScene(scene, camera);


	if (this->show_ao && ao_buffer)
		ao_buffer->toViewport();

	int width = Application::instance->window_width;
	int height = Application::instance->window_height;

	if (this->show_gbuffers)
		showGbuffers(width, height, camera); // hay que pasar la camara y w , h...
	
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

	// hacer comprobacionees de nodo con alpha... si tiene alpha forward,,,

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

void GTR::Renderer::createGbuffers(int width, int height, std::vector <RenderCall>& rendercalls, Camera* camera) {
	

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

	for (int i = 0; i < rendercalls.size(); i++)//render all
	{
		RenderCall& rc = rendercalls[i]; 
		renderMeshWithMaterial(eRenderMode::GBUFFERS, rc.model, rc.mesh, rc.material, camera); //always in gbuffer mode
	}
	

	//stop rendering to the gbuffers
	gbuffers_fbo.unbind();

}

void GTR::Renderer::showGbuffers(int width, int height, Camera* camera) {


	//to plot just alpha component
	//gbuffers_fbo.color_textures[0]->toViewport(Shader::Get("showAlpha"));
	//if (!this->show_gbuffers)
	//	return;

	//GB0 color
	glViewport(0, 0, width * 0.5, height * 0.5); //set area of the screen and render fullscreen quad
	gbuffers_fbo.color_textures[0]->toViewport();

	//GB1 normal
	glViewport(width * 0.5, 0, width * 0.5, height * 0.5);
	gbuffers_fbo.color_textures[1]->toViewport();

	//GB2 material. properties
	glViewport(width * 0.5, height * 0.5, width * 0.5, height * 0.5);
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



void GTR::Renderer::renderDeferred(GTR::Scene* scene, std::vector <RenderCall>& rendercalls, Camera* camera)
{
	int width = Application::instance->window_width;
	int height = Application::instance->window_height;

	createGbuffers(width, height, rendercalls, camera);
	//-----End gbuffers pass

	// si existe ya la textura pero el tamaño de la textura no es el mismo que el anterior, que te la tire el de anterior y te cree uno nueva -> por resize de las ventas...
	//!!!!

	if (!ao_buffer) {
		//GL_LUMINANCE -> los dos guardan 1 canal, pero este represemta la iluminancia total. 
		//GL_RED tamb solo 1 canal pero puede representar verde o azul... 
		// los dos guardan 1 byte
		// si es red, desde la shader lee y pone solo red, mientras otro los 3 canales por el igual
		ao_buffer = new Texture(width * 0.5, height* 0.5, GL_RED, GL_UNSIGNED_BYTE); // solo usamos un canal
		// goal-> guardar inf si un pixel esta mas oscurecido o menos...
		// reducimos la resolucion de AO funcionara igual de bien
	}
	ssao.applyEffect(gbuffers_fbo.depth_texture, gbuffers_fbo.color_textures[GTR::eChannels::NORMAL], camera, ao_buffer);


	//---------Ilumination_Pass--------------
		
	if (illumination_fbo.fbo_id == 0) {

		//create 3 textures of 4 components
		illumination_fbo.create(width, 
								height,
								1, 			//three textures
								GL_RGB, 		//three channels
								GL_FLOAT, // to have more precision to accumulate light
								true);		//add depth_texture

	}

	//clone the depth buffer content to the other depth buffer so they contain the same
	//therefore, we can have the contain in the scene deth and block writing it to avoid any modification while render
	//this->gbuffers_fbo.depth_texture->copyTo(illumination_fbo.depth_texture);
	//glDepthMask(false); //now we can block writing to it	

	//now if we enable depth_test during the illumination pass it will take into account the scene depth buffer
	illumination_fbo.bind();

	// if we want to clear all in once
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	checkGLErrors();

	
	Mesh* quad = Mesh::getQuad(); 
	Shader* shader = Shader::Get("deferred");
	shader->enable();
	shader->setTexture("u_color_texture", gbuffers_fbo.color_textures[0], GTR::eChannels::ALBEDO);
	shader->setTexture("u_normal_texture", gbuffers_fbo.color_textures[1], GTR::eChannels::NORMAL);
	shader->setTexture("u_extra_texture", gbuffers_fbo.color_textures[2], GTR::eChannels::EMISSIVE);
	shader->setTexture("u_depth_texture", gbuffers_fbo.depth_texture, GTR::eChannels::DEPTH);
	shader->setUniform("u_ambient_light", degamma(scene->ambient_light));
	shader->setTexture("u_ao_texture", ao_buffer, GTR::eChannels::OCCLUSION);
	
	shader->setUniform("u_ao_show", show_ao_deferred);
	shader->setUniform("u_camera_position", camera->eye);

	Matrix44 inv_vp = camera->viewprojection_matrix;
	inv_vp.inverse();
	Vector2 iRes = Vector2(1.0 / (float)width, 1.0 / (float)height);
	shader->setUniform("u_inverse_viewprojection", inv_vp);
	shader->setUniform("u_iRes", iRes );

	//disable depth and blend
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

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

	//---------Using geometry--------------
	 
	//we can use a sphere mesh for point lights
	Mesh* sphere = Mesh::Get("data/meshes/sphere.obj", false, false);

	glDisable(GL_CULL_FACE); 
	
	//this deferred_ws shader uses the basic.vs instead of quad.vs
	shader = Shader::Get("deferred_ws");

	shader->enable();
	shader->setTexture("u_color_texture", gbuffers_fbo.color_textures[0], GTR::eChannels::ALBEDO);
	shader->setTexture("u_normal_texture", gbuffers_fbo.color_textures[1], GTR::eChannels::NORMAL);
	shader->setTexture("u_extra_texture", gbuffers_fbo.color_textures[2], GTR::eChannels::EMISSIVE);
	shader->setTexture("u_depth_texture", gbuffers_fbo.depth_texture, GTR::eChannels::DEPTH);
	
	//basic.vs will need the model and the viewproj of the camera
	shader->setUniform("u_viewprojection", camera->viewprojection_matrix);
	shader->setUniform("u_inverse_viewprojection", inv_vp);
	shader->setUniform("u_iRes", iRes);
	shader->setUniform("u_camera_position", camera->eye );
	//glEnable(GL_DEPTH_TEST);
	Matrix44 m; Vector3 pos; 
	for (int i = 0; i < this->light_entities.size(); i++)
	{
		light = this->light_entities[i];
		if (light->light_type == DIRECTIONAL)
			continue;
		
		//we must translate the model to the center of the light
		// and scale it according to the max_distance of the light
		float max_dist = light->max_dist;
		pos = light->model.getTranslation();
		m.setTranslation(pos.x, pos.y, pos.z);
		m.scale(max_dist, max_dist, max_dist);
		shader->setUniform("u_model", m); //pass the model to render the sphere

		light->uploadToShader(shader);
		glFrontFace(GL_CW);
		sphere->render(GL_TRIANGLES);
		

		//only pixels behind a surface are rendered //only draw if the pixel is behind 
		// we solve this during the depth test stage, meaning before execte .fs
		glDepthFunc(GL_GREATER);
		glBlendFunc(GL_ONE, GL_ONE);
	}

	//stop rendering to the fbo, render to screen
	illumination_fbo.unbind();


	//set to back //be sure blending is not active
	glFrontFace(GL_CCW);
	glDisable(GL_BLEND);

	//and render the texture into the screen
	//illumination_fbo.color_textures[0]->toViewport(); // aqui es cuando lo volvemos pasar a gamma usando un shader que aplica la gamma conversion

	shader = Shader::Get("applygamma");
	shader->enable();
	shader->setTexture("u_texture", illumination_fbo.color_textures[0], 9); 
	quad->render(GL_TRIANGLES);

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
		shader->enable();///---------------------------------------------
		shader->setUniform("u_texture_type", 2);
	}
	else if (mode == GBUFFERS) 
		shader = Shader::Get("gbuffers");

	

	if (!shader)//no shader? then nothing to render
		return;

	shader->enable();

	bool use_dither = false;
	//flag para deffered en materiales con transparencias.y
	if (mode == GBUFFERS && material->alpha_mode == GTR::eAlphaMode::BLEND) {
		shader->setUniform("u_use_dither", !use_dither);

	}
		
	/*if (mode == GBUFFERS && this->ao_buffer) {
		shader->setTexture("u_ao_texture", this->ao_buffer, GTR::eChannels::OCCLUSION);
	}*/


	assert(glGetError() == GL_NO_ERROR);

	
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
	shader->setUniform("u_ambient_light", degamma(scene->ambient_light));//_---------------------------------

	//select the blending. Solo para las luces.
	if (material->alpha_mode == GTR::eAlphaMode::BLEND)
	{
		glEnable(GL_BLEND);
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	}
	else
		glDisable(GL_BLEND);
	
	
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

/*

void Renderer::render2depthbuffer(GTR::Material* material, Camera* camera, std::vector<RenderCall>& rendercalls) {
	
	int width = Application::instance->window_width;
	//int height = Application::instance->window_height;

	LightEntity* light;
	for (int i = 0; i < this->light_entities.size(); ++i)
	{
		if (this->light_entities[i]->light_type == SPOT) {
			light = this->light_entities[i];
		}
	}

	//first time we create the FBO
	if (!light->shadow_fbo)
	{
		//we create FBO
		FBO* fbo = new FBO();
		
		light->shadow_fbo = fbo;
		light->shadow_fbo->setDepthOnly(width, width); // only create a depth texture
	}

	//enable it to render inside the texture
	light->shadow_fbo->bind();

	//you can disable writing to the color buffer to speed up the rendering as we do not need it
	glColorMask(false, false, false, false);

	//clear the depth buffer only (don't care of color)
	glClear(GL_DEPTH_BUFFER_BIT);

	//whatever we render here will be stored inside a texture, we don't need to do anything fanzy
	
	for (int i = 0; i < rendercalls.size(); i++)//render all
	{
		RenderCall& rc = rendercalls[i];
		renderMeshWithMaterial(eRenderMode::MULTI, rc.model, rc.mesh, rc.material, camera); //always in gbuffer mode
	}

	if (material->alpha_mode == BLEND)
	{
		glDisable(GL_BLEND);
	}

	//disable it to render back to the screen
	light->shadow_fbo->unbind();

	//allow to render back to the color buffer
	glColorMask(true, true, true, true);
	
	//color textures are inside this var
	//light->fbo->color_textures[0];

	//and the depth texture inside this one
	light->shadow_fbo->depth_texture;


}

*/

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

//---Screen Space Ambient Occlusion

//genera puntos equidistances sobre la esfera
std::vector<Vector3> generateSpherePoints(int num, float radius, bool hemi)
{
	std::vector<Vector3> points;
	points.resize(num);
	for (int i = 0; i < num; i += 3)
	{
		Vector3& p = points[i];
		float u = random();
		float v = random();
		float theta = u * 2.0 * PI;
		float phi = acos(2.0 * v - 1.0);
		//float r = cbrt(random()) * radius;
		float r = radius;
		float sinTheta = sin(theta);
		float cosTheta = cos(theta);
		float sinPhi = sin(phi);
		float cosPhi = cos(phi);
		p.x = r * sinPhi * cosTheta;
		p.y = r * sinPhi * sinTheta;
		p.z = r * cosPhi;
		if (hemi && p.z < 0)
			p.z *= -1.0;
	}
	return points;
}

SSAOFX::SSAOFX() {
	this->intensity = 1.0;
	random_points = generateSpherePoints(64*4, 1.0, true);

}

void SSAOFX::applyEffect(Texture* Zbuffer, Texture* normal_buffer, Camera* camera, Texture* outputOcc) {

	int width = Application::instance->window_width;
	int height = Application::instance->window_height;

	FBO* ssao_fbo = Texture::getGlobalFBO(outputOcc);
	//start rendering inside the ssao texture
	ssao_fbo->bind();

	//disable using mipmaps
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	//enable bilinear filtering
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	checkGLErrors();

	Mesh* quad = Mesh::getQuad();
	glDisable(GL_DEPTH_TEST); //pintar quad desactivas los flags...
	glDisable(GL_BLEND);

	//get the shader for SSAO (remember to create it using the atlas)
	Shader* shader = Shader::Get("ssao");
	shader->enable();
	//send random points so we can fetch around
	shader->setUniform3Array("u_points", random_points[0].v, random_points.size()); // numero de vectores que hay 

	shader->setTexture("u_normal_texture", normal_buffer, GTR::eChannels::NORMAL);
	shader->setTexture("u_depth_texture", Zbuffer, GTR::eChannels::DEPTH);
	
	//send info to reconstruct the world position nd iRes (pixel size) to center the samples
	//we will need the viewprojection to obtain the uv in the depthtexture of any random position of our world
	
	shader->setUniform("u_viewprojection", camera->viewprojection_matrix );
	Matrix44 invp = camera->viewprojection_matrix;
	invp.inverse();
	shader->setUniform("u_inverse_viewprojection", invp);

	shader->setUniform("u_iRes", Vector2(1.0 / (float)Zbuffer->width, 1.0 / (float)Zbuffer->height));
	shader->setUniform("u_camera_nearfar", Vector2(camera->near_plane, camera->far_plane));
	//render fullscreen quad
	quad->render(GL_TRIANGLES);

	ssao_fbo->unbind();


}
