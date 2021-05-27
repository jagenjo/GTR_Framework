#pragma once
#include "prefab.h"
#include "fbo.h"
#include "application.h"

//forward declarations
class Camera;

namespace GTR {

	enum eRenderMode {
		SHOW_TEXTURE,
		SHOW_NORMAL,
		SHOW_OC,
		SHOW_UVS,
		SINGLE,
		MULTI,
		GBUFFERS
	
	};

	enum ePipelineMode {
		FORWARD,
		DEFERRED
	};

	class Prefab;
	class Material;
	

	struct RenderCall 
	{
		Matrix44 model;
		Mesh* mesh;
		Material* material;
		float dist2camera;

		RenderCall() {
			mesh = NULL;
			material = NULL;
			dist2camera = NULL;
			model.setIdentity();
		}
	};


	// This class is in charge of rendering anything in our system.
	// Separating the render from anything else makes the code cleaner
	class Renderer
	{


	public:

		std::vector< RenderCall > rc_data_list;
	
		int max_num_lights;
		std::vector<LightEntity* > light_entities;

		Texture* color_buffer;
				

		FBO fbo;
		FBO gbuffers_fbo;
		FBO illumination_fbo;

		
		bool show_gbuffers;

		eRenderMode render_mode;
		ePipelineMode pipeline_mode;

		bool rendering_shadowmap;
		
		//ctor
		Renderer();
		
	
		void render2FBO(GTR::Scene* scene, Camera* camera);

		//renders several elements of the scene
		void renderScene(GTR::Scene* scene, Camera* camera);

		void collectRenderCalls(GTR::Scene* scene, Camera* camera);
	
		//to get a whole prefab (with all its nodes)
		void getRCsfromPrefab(const Matrix44& model, GTR::Prefab* prefab, Camera* camera);

		//to get node from the prefab and its children
		void getRCsfromNode(const Matrix44& model, GTR::Node* node, Camera* camera);

		//to render one mesh given its material and transformation matrix
		void renderMeshWithMaterial(eRenderMode mode, const Matrix44 model, Mesh* mesh, GTR::Material* material, Camera* camera);

		

		void renderForward(GTR::Scene* scene, std::vector<RenderCall>& rendercalls, Camera* camera);

		void renderDeferred(GTR::Scene* scene, std::vector <RenderCall>& rendercalls, Camera* camera);

		//to render lights in the scene 
		void renderlights(eRenderMode mode, Shader* shader, Mesh* mesh, GTR::Material* material);

		void render2depthbuffer(GTR::Material* material, Camera* camera);
	};

	Texture* CubemapFromHDRE(const char* filename);


};