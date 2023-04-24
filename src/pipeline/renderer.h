#pragma once
#include "scene.h"
#include "prefab.h"

#include "light.h"

// Make sure that MAX_LIGHTS in the CPU code <= shader, otherwise it will break
#define MAX_LIGHTS 4
#define SHADOWMAP_RES_X 512
#define SHADOWMAP_RES_Y 512

//forward declarations
class Camera;
class Skeleton;
namespace GFX {
	class Shader;
	class Mesh;
	class FBO;
}

namespace SCN {

	class Prefab;
	class Material;

	enum eRenderMode {
		FLAT, TEXTURED, LIGHTS
	};

	enum eLightsMode {
		MULTI, SINGLE
	};

	enum eNormalMapMode {
		WITHOUT_NMAP, WITH_NMAP
	};

	enum eSpecMode {
		WITHOUT_SPEC, WITH_SPEC
	};

	// Class that contains all information related to a render call.
	class RenderCall {
	public:
		Matrix44 model;
		GFX::Mesh* mesh;
		Material* material;
		float distance_to_camera;
		std::vector<LightEntity*> lights_affecting;

	};

	// This class is in charge of rendering anything in our system.
	// Separating the render from anything else makes the code cleaner
	class Renderer
	{
	private:
		// private member that will store all render_calls when rendering a scene
		std::vector<RenderCall> render_calls;

		// Sorter for the RenderCall class
		struct rc_sorter
		{
			inline bool operator () (const RenderCall& rc1, const RenderCall& rc2)
			{
				eAlphaMode mode1 = rc1.material->alpha_mode;
				eAlphaMode mode2 = rc2.material->alpha_mode;

				// if their modes are the same, we want to sort the render calls from furthest to closest to camera
				if (mode1 == mode2) {
					return rc1.distance_to_camera > rc2.distance_to_camera;
				}// if their modes are not the same, we want to render opaque (no_alpha) nodes first (no_alpha = 0, alpha_mask = 1, alpha-blend = 2, thus < operator)
				else {
					return mode1 < mode2;
				}
			}
		};

		// white texture to be used in the mesh renderer
		GFX::Texture* white_texture;

	public:
		eLightsMode lights_mode;
		eRenderMode render_mode;
		eNormalMapMode nmap_mode;
		eSpecMode spec_mode;

		bool sort_alpha;
		bool use_shadowmaps;
		bool render_wireframe;
		bool render_boundaries;
		bool show_shadowmaps;

		// shadowmap atlas
		GFX::FBO* shadowmap_atlas_fbo;
		GFX::Texture* shadowmap_atlas;


		GFX::Texture* skybox_cubemap;

		SCN::Scene* scene;

		std::vector<LightEntity*> lights;

		//updated every frame
		Renderer(const char* shaders_atlas_filename );
		~Renderer();

		//just to be sure we have everything ready for the rendering
		void setupScene(Camera* camera);

		//******************************** MY FUNCTIONS ********************************
		// create render call associated with a node
		void createNodeRC(SCN::Node* node, Camera* camera);

		//create a single render call and store it
		void createRenderCall(Matrix44 model, GFX::Mesh* mesh, SCN::Material* material);
		void updateRCLights();

		// preps the shader and passes it the necessary textures
		GFX::Shader* prep_shader(const RenderCall rc, const char* shader_name);

		// auxiliary function of render mesh with material multi to send the information of a light to a specific shader
		void sendLightInfoMulti(LightEntity* light, GFX::Shader* shader);

		void renderMeshWithMaterialFlat(const RenderCall rc);

		//to render one mesh given its material and transformation matrix taking lights into account, single pass
		void renderMeshWithMaterialLightSingle(const RenderCall rc);

		void generateShadowmaps(Camera* camera);
		

		//******************************** END ********************************
	
		//renders several elements of the scene
		void renderScene(SCN::Scene* scene, Camera* camera);
		void renderFrame(SCN::Scene* scene, Camera* camera);

		//render the skybox
		void renderSkybox(GFX::Texture* cubemap);
	
		//to render one mesh given its material and transformation matrix
		void renderMeshWithMaterial(const RenderCall rc);

		//to render one mesh given its material and transformation matrix taking lights into account, multi-pass
		void renderMeshWithMaterialLightMulti(const RenderCall rc);

		void showUI();

		void cameraToShader(Camera* camera, GFX::Shader* shader); //sends camera uniforms to shader

		//debug
		void debugShadowMaps();
	};

};