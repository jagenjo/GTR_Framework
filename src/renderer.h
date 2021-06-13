#pragma once
#include "prefab.h"
#include "fbo.h"
#include "application.h"
#include "sphericalharmonics.h"

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
	
	class SSAOFX{
	public:
		float intensity;
		std::vector<Vector3> random_points;
		//ctor
		SSAOFX();
		void applyEffect(Texture* Zbuffer, Texture* normal_buffer, Camera* camera, Texture* outputOcc );
	
	};

	//struct to store probes
	struct sProbe {
		Vector3 pos; //where is located
		Vector3 local; //its ijk pos in the matrix
		int index; //its index in the linear array
		SphericalHarmonics sh; //coeffs
	};


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

		//RCs
		std::vector< RenderCall > rc_data_list;
		
		//Lights
		int max_num_lights;
		std::vector<LightEntity* > light_entities;

		
		//FBO & SSAO
		FBO gbuffers_fbo;
		FBO illumination_fbo;
		FBO* irr_fbo; //irradiance
		SSAOFX ssao;
		//FBO* ssao_fbo;

		//Textures
		Texture* ao_buffer;
	

		//Flags
		eRenderMode render_mode;
		ePipelineMode pipeline_mode;

		bool update_shadowmaps;
		bool rendering_shadowmap;
		bool show_ao;
		bool show_ao_deferred;
		bool show_gbuffers;

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

		void uploadTextures(Material* material, Shader* shader);

		//to render one mesh given its material and transformation matrix
		void renderMeshWithMaterial(eRenderMode mode, const Matrix44 model, Mesh* mesh, GTR::Material* material, Camera* camera);

		void renderForward(GTR::Scene* scene, std::vector<RenderCall>& rendercalls, Camera* camera, bool apply_clear);

		void createGbuffers(int width, int height, std::vector<RenderCall>& rendercalls, Camera* camera);

		void showGbuffers(int width, int height, Camera* camera);

		void renderDeferred(GTR::Scene* scene, std::vector <RenderCall>& rendercalls, Camera* camera);

		//to render lights in the scene 
		void renderlights(eRenderMode mode, Shader* shader, Mesh* mesh, GTR::Material* material);

		void createShadowmap(GTR::Scene* scene, LightEntity* light, Camera* camera);

		void showShadowmap(FBO* fbo, Camera* camera, float width, float height);
		//----

		void extractProbe(GTR::Scene* scene, sProbe& p);

		void updateIrradianceCache(GTR::Scene* scene);

		void renderProbe(Vector3 pos, float size, float* coeffs);

		void applyfinalHDR();

	};

	Texture* CubemapFromHDRE(const char* filename);



};