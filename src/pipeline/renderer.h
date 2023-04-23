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

	// Class that contains all information related to a render call.
	class RenderCall {
	public:
		Matrix44 model;
		GFX::Mesh* mesh;
		Material* material;
		float distance_to_camera;
	};

	// This class is in charge of rendering anything in our system.
	// Separating the render from anything else makes the code cleaner
	class Renderer
	{
	private:
		// private member that will store all render_calls when rendering a scene
		std::vector<RenderCall> render_calls;

	public:
		bool render_wireframe;
		bool render_boundaries;

		GFX::Texture* skybox_cubemap;

		SCN::Scene* scene;

		//updated every frame
		Renderer(const char* shaders_atlas_filename );

		//just to be sure we have everything ready for the rendering
		void setupScene();

		//add here your functions
		//create a single render call and store it
		void createRenderCall(Matrix44 model, GFX::Mesh* mesh, SCN::Material* material);

		//renders several elements of the scene
		void renderScene(SCN::Scene* scene, Camera* camera);

		//render the skybox
		void renderSkybox(GFX::Texture* cubemap);
	
		//to render one node from the prefab and its children
		void renderNode(SCN::Node* node, Camera* camera);

		//to render one mesh given its material and transformation matrix
		void renderMeshWithMaterial(const Matrix44 model, GFX::Mesh* mesh, SCN::Material* material);

		void showUI();

		void cameraToShader(Camera* camera, GFX::Shader* shader); //sends camera uniforms to shader
	};

};