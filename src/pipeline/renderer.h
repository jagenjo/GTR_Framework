#pragma once
#include "scene.h"
#include "prefab.h"

#include "light.h"

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
		TEXTURED,
		LIGHTS,
		DEFERRED,
		FLAT
	};
	
	// This class is in charge of rendering anything in our system.
	// Separating the render from anything else makes the code cleaner
	class Renderer
	{
	public:
		bool render_wireframe;
		bool render_boundaries;
		bool show_shadowmaps;
		bool show_gbuffers;

		eRenderMode render_mode;

		GFX::Texture* skybox_cubemap;

		SCN::Scene* scene;

		//vector of all the lights of the scene
		std::vector<LightEntity*> lights;
		std::vector<LightEntity*> visible_lights;
		std::vector<BaseEntity*> base_ents;

		//updated every frame
		Renderer(const char* shaders_atlas_filename );

		//just to be sure we have everything ready for the rendering
		void setupScene();

		//add here your functions
		//...
		float distance(vec3* v1, vec3* v2);

		//renders several elements of the scene
		void renderScene(SCN::Scene* scene, Camera* camera);
		void renderForward(SCN::Scene* scene, Camera* camera);
		void renderSceneNodes(SCN::Scene* scene, Camera* camera);
		void renderDeferred(SCN::Scene* scene, Camera* camera);
		void renderFrame(SCN::Scene* scene, Camera* camera);

		void generateShadowMaps();

		//render the skybox
		void renderSkybox(GFX::Texture* cubemap);
	
		//to render one node from the prefab and its children
		void renderNode(SCN::Node* node, Camera* camera);

		//to render one mesh given its material and transformation matrix
		void renderMeshWithMaterialFlat(const Matrix44 model, GFX::Mesh* mesh, SCN::Material* material);
		void renderMeshWithMaterial(const Matrix44 model, GFX::Mesh* mesh, SCN::Material* material);
		void renderMeshWithMaterialLight(const Matrix44 model, GFX::Mesh* mesh, SCN::Material* material);
		void renderMeshWithMaterialGBuffers(const Matrix44 model, GFX::Mesh* mesh, SCN::Material* material);


		void showUI();

		void cameraToShader(Camera* camera, GFX::Shader* shader); //sends camera uniforms to shader
		void lightToShader(LightEntity* light, GFX::Shader* shader); //sends light uniforms to shader

		void debugShadowMaps();
	};

};