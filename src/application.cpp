#include "application.h"
#include "utils.h"
#include "mesh.h"
#include "texture.h"

#include "fbo.h"
#include "shader.h"
#include "input.h"
#include "includes.h"
#include "prefab.h"
#include "gltf_loader.h"
#include "renderer.h"

#include <cmath>
#include <string>
#include <cstdio>

Application* Application::instance = nullptr;

Camera* camera = nullptr;
GTR::Scene* scene = nullptr;
GTR::Prefab* prefab = nullptr;
GTR::Renderer* renderer = nullptr;
GTR::BaseEntity* selected_entity = nullptr;
FBO* fbo = nullptr;
Texture* texture = nullptr;

float cam_speed = 10;////

Mesh mesh;

Application::Application(int window_width, int window_height, SDL_Window* window)
{
	this->window_width = window_width;
	this->window_height = window_height;
	this->window = window;
	instance = this;
	must_exit = false;
	render_debug = true;
	render_gui = true;

	render_wireframe = false;

	fps = 0;
	frame = 0;
	time = 0.0f;
	elapsed_time = 0.0f;
	mouse_locked = false;

	//loads and compiles several shaders from one single file

    //change to "data/shader_atlas_osx.txt" if you are in XCODE
	//if(!Shader::LoadAtlas("data/shader_atlas.txt")) // contiene muchos subficheros

    //change to "data/shader_atlas_osx.txt" if you are in XCODE
#ifdef __APPLE__
    const char* shader_atlas_filename = "data/shader_atlas_osx.txt";
#else
	const char* shader_atlas_filename = "data/shader_atlas_osx.txt"; //"data/shader_atlas.txt";
#endif
	if(!Shader::LoadAtlas(shader_atlas_filename))
        exit(1);
    checkGLErrors();



	// We create a global camera and set a position and projection properties
	camera = new Camera();
	camera->lookAt(Vector3(-150.f, 150.0f, 250.f), Vector3(0.f, 0.0f, 0.f), Vector3(0.f, 1.f, 0.f));
	camera->setPerspective( 45.f, window_width/(float)window_height, 1.0f, 10000.f);


	//This class will be the one in charge of rendering all 
	//renderer = new GTR::Renderer(); //here so we have opengl ready in constructor!

	//Example of loading a prefab
	//prefab = GTR::Prefab::Get("data/prefabs/gmc/scene.gltf");


	scene = new GTR::Scene();
	if (!scene->load("data/scene.json"))
		exit(1);

	camera->lookAt(scene->main_camera.eye, scene->main_camera.center, Vector3(0, 1, 0));
	camera->fov = scene->main_camera.fov;

	//This class will be the one in charge of rendering all 
	renderer = new GTR::Renderer(); //here so we have opengl ready in constructor

	//hide the cursor
	SDL_ShowCursor(!mouse_locked); //hide or show the mouse
}

/*
std::vector<Vector3> generateSpherePoints2(int num, float radius, bool hemi)
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
*/

//what to do when the image has to be draw
void Application::render(void)
{
	//be sure no errors present in opengl before start
	checkGLErrors();

	//set the camera as default (used by some functions in the framework)
	camera->enable();

	//set default flags
	glDisable(GL_BLEND);
    
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	if(render_wireframe)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	//lets render something
	//Matrix44 model;
	//renderer->renderPrefab( model, prefab, camera );

	//renderer->renderScene(scene, camera);
	renderer->render2FBO(scene, camera);

	Vector3 dim(8, 6, 12);
	Vector3 start_pos(-55, 10, -170);
	Vector3 end_pos(180, 150, 80);
	
	renderer->createProbeGrid(dim, start_pos, end_pos);

	/*
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	if (!mesh.vertices.size())
		mesh.vertices = generateSpherePoints2(1000, 100, false);
	Shader* sh = Shader::Get("flat");
	sh->enable();
	sh->setUniform("u_model", Matrix44());
	sh->setUniform("u_viewprojection", camera->viewprojection_matrix);
	sh->setUniform("u_color", Vector4(1,1,1,1));
	glPointSize(2);
	mesh.render(GL_POINTS);
	
	*/

	//Draw the floor grid, helpful to have a reference point
	//if(render_debug)
	//	drawGrid();

    glDisable(GL_DEPTH_TEST);
    //render anything in the gui after this

	//the swap buffers is done in the main loop after this function
}

void Application::update(double seconds_elapsed)
{
	float speed = seconds_elapsed * cam_speed; //the speed is defined by the seconds_elapsed so it goes constant
	float orbit_speed = seconds_elapsed * 0.5;
	
	//async input to move the camera around
	if (Input::isKeyPressed(SDL_SCANCODE_LSHIFT)) speed *= 10; //move faster with left shift
	if (Input::isKeyPressed(SDL_SCANCODE_W) || Input::isKeyPressed(SDL_SCANCODE_UP)) camera->move(Vector3(0.0f, 0.0f, 1.0f) * speed);
	if (Input::isKeyPressed(SDL_SCANCODE_S) || Input::isKeyPressed(SDL_SCANCODE_DOWN)) camera->move(Vector3(0.0f, 0.0f,-1.0f) * speed);
	if (Input::isKeyPressed(SDL_SCANCODE_A) || Input::isKeyPressed(SDL_SCANCODE_LEFT)) camera->move(Vector3(1.0f, 0.0f, 0.0f) * speed);
	if (Input::isKeyPressed(SDL_SCANCODE_D) || Input::isKeyPressed(SDL_SCANCODE_RIGHT)) camera->move(Vector3(-1.0f, 0.0f, 0.0f) * speed);

	


	//mouse input to rotate the cam
	#ifndef SKIP_IMGUI
	if (!ImGuizmo::IsUsing())
	#endif
	{
		if (mouse_locked || Input::mouse_state & SDL_BUTTON(SDL_BUTTON_RIGHT)) //move in first person view
		{
			camera->rotate(-Input::mouse_delta.x * orbit_speed * 0.5, Vector3(0, 1, 0));
			Vector3 right = camera->getLocalVector(Vector3(1, 0, 0));
			camera->rotate(-Input::mouse_delta.y * orbit_speed * 0.5, right);
		}
		else //orbit around center
		{
			bool mouse_blocked = false;
			#ifndef SKIP_IMGUI
						mouse_blocked = ImGui::IsAnyWindowHovered() || ImGui::IsAnyItemHovered() || ImGui::IsAnyItemActive();
			#endif
			if (Input::mouse_state & SDL_BUTTON(SDL_BUTTON_LEFT) && !mouse_blocked) //is left button pressed?
			{
				camera->orbit(-Input::mouse_delta.x * orbit_speed, Input::mouse_delta.y * orbit_speed);
			}
		}
	}
	
	//move up or down the camera using Q and E
	if (Input::isKeyPressed(SDL_SCANCODE_Q)) camera->moveGlobal(Vector3(0.0f, -1.0f, 0.0f) * speed);
	if (Input::isKeyPressed(SDL_SCANCODE_E)) camera->moveGlobal(Vector3(0.0f, 1.0f, 0.0f) * speed);

	//to navigate with the mouse fixed in the middle
	SDL_ShowCursor(!mouse_locked);
	#ifndef SKIP_IMGUI
		ImGui::SetMouseCursor(mouse_locked ? ImGuiMouseCursor_None : ImGuiMouseCursor_Arrow);
	#endif
	if (mouse_locked)
	{
		Input::centerMouse();
		//ImGui::SetCursorPos(ImVec2(Input::mouse_position.x, Input::mouse_position.y));
	}
}

void Application::renderDebugGizmo()
{
	if (!selected_entity || !render_debug)
		return;

	//example of matrix we want to edit, change this to the matrix of your entity
	Matrix44& matrix = selected_entity->model;

	#ifndef SKIP_IMGUI

	static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::TRANSLATE);
	static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::WORLD);
	if (ImGui::IsKeyPressed(90))
		mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
	if (ImGui::IsKeyPressed(69))
		mCurrentGizmoOperation = ImGuizmo::ROTATE;
	if (ImGui::IsKeyPressed(82)) // r Key
		mCurrentGizmoOperation = ImGuizmo::SCALE;
	if (ImGui::RadioButton("Translate", mCurrentGizmoOperation == ImGuizmo::TRANSLATE))
		mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
	ImGui::SameLine();
	if (ImGui::RadioButton("Rotate", mCurrentGizmoOperation == ImGuizmo::ROTATE))
		mCurrentGizmoOperation = ImGuizmo::ROTATE;
	ImGui::SameLine();
	if (ImGui::RadioButton("Scale", mCurrentGizmoOperation == ImGuizmo::SCALE))
		mCurrentGizmoOperation = ImGuizmo::SCALE;
	float matrixTranslation[3], matrixRotation[3], matrixScale[3];
	ImGuizmo::DecomposeMatrixToComponents(matrix.m, matrixTranslation, matrixRotation, matrixScale);
	ImGui::InputFloat3("Tr", matrixTranslation, 3);
	ImGui::InputFloat3("Rt", matrixRotation, 3);
	ImGui::InputFloat3("Sc", matrixScale, 3);
	ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, matrix.m);

	if (mCurrentGizmoOperation != ImGuizmo::SCALE)
	{
		if (ImGui::RadioButton("Local", mCurrentGizmoMode == ImGuizmo::LOCAL))
			mCurrentGizmoMode = ImGuizmo::LOCAL;
		ImGui::SameLine();
		if (ImGui::RadioButton("World", mCurrentGizmoMode == ImGuizmo::WORLD))
			mCurrentGizmoMode = ImGuizmo::WORLD;
	}
	static bool useSnap(false);
	if (ImGui::IsKeyPressed(83))
		useSnap = !useSnap;
	ImGui::Checkbox("", &useSnap);
	ImGui::SameLine();
	static Vector3 snap;
	switch (mCurrentGizmoOperation)
	{
	case ImGuizmo::TRANSLATE:
		//snap = config.mSnapTranslation;
		ImGui::InputFloat3("Snap", &snap.x);
		break;
	case ImGuizmo::ROTATE:
		//snap = config.mSnapRotation;
		ImGui::InputFloat("Angle Snap", &snap.x);
		break;
	case ImGuizmo::SCALE:
		//snap = config.mSnapScale;
		ImGui::InputFloat("Scale Snap", &snap.x);
		break;
	}
	ImGuiIO& io = ImGui::GetIO();
	ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
	ImGuizmo::Manipulate(camera->view_matrix.m, camera->projection_matrix.m, mCurrentGizmoOperation, mCurrentGizmoMode, matrix.m, NULL, useSnap ? &snap.x : NULL);
	#endif
}


//called to render the GUI from
void Application::renderDebugGUI(void)
{
#ifndef SKIP_IMGUI //to block this code from compiling if we want

	//System stats
	ImGui::Text(getGPUStats().c_str());					   // Display some text (you can use a format strings too)

	ImGui::Checkbox("Wireframe", &render_wireframe);
	ImGui::ColorEdit3("BG color", scene->background_color.v);
	ImGui::ColorEdit3("Ambient Light", scene->ambient_light.v);
	ImGui::Combo("Pipeline", (int*) &renderer->pipeline_mode, "FORWARD\0DEFERRED\0", 2);
	ImGui::Checkbox("UPDATE_SHADOWMAPS", &renderer->update_shadowmaps);
	ImGui::Checkbox("SHOW_GBUFFERS", &renderer->show_gbuffers);
	ImGui::Checkbox("SHOW_AO", &renderer->show_ao);
	ImGui::Checkbox("SHOW_AO_DEFERRED", &renderer->show_ao_deferred);
	

	//add info to the debug panel about the camera
	if (ImGui::TreeNode(camera, "Camera")) {
		camera->renderInMenu();
		ImGui::TreePop();
	}

	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.75f, 0.75f, 0.75f, 1.0f));

	//example to show prefab info: first param must be unique!
	for (int i = 0; i < scene->entities.size(); ++i)
	{
		GTR::BaseEntity* entity = scene->entities[i];

		

		if(selected_entity == entity)
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.0f));

		if (ImGui::TreeNode(entity, entity->name.c_str()))
		{
			entity->renderInMenu();
			ImGui::TreePop();
		}

		if (selected_entity == entity)
			ImGui::PopStyleColor();

		if (ImGui::IsItemClicked(0))
			selected_entity = entity;
	}

	ImGui::PopStyleColor();
#endif
}

//Keyboard event handler (sync input)
void Application::onKeyDown( SDL_KeyboardEvent event )
{
	switch(event.keysym.sym)
	{
		case SDLK_ESCAPE: must_exit = true; break; //ESC key, kill the app
		case SDLK_F1: render_debug = !render_debug; break;
		case SDLK_f: camera->center.set(0, 0, 0); camera->updateViewMatrix(); break;
		case SDLK_F5: Shader::ReloadAll(); break;
		case SDLK_F6: 
			scene->clear(); 
			scene->load(scene->filename.c_str());
			selected_entity = NULL;  
			camera->lookAt(scene->main_camera.eye, scene->main_camera.center, Vector3(0, 1, 0));
			camera->fov = scene->main_camera.fov;
			break;

	
		case SDLK_t: renderer->render_mode = GTR::eRenderMode::SHOW_TEXTURE; break;
		case SDLK_o: renderer->render_mode = GTR::eRenderMode::SINGLE; break;
		case SDLK_p: renderer->render_mode = GTR::eRenderMode::MULTI; break;
		case SDLK_i: renderer->update_shadowmaps = !renderer->update_shadowmaps; break;
	
		case SDLK_m: renderer->render_mode = GTR::eRenderMode::SHOW_NORMAL; break;
		case SDLK_n: renderer->render_mode = GTR::eRenderMode::SHOW_OC; break;
		case SDLK_b: renderer->render_mode = GTR::eRenderMode::SHOW_UVS; break;
        case SDLK_0: renderer->pipeline_mode = GTR::ePipelineMode::FORWARD; break;
        case SDLK_1: renderer->pipeline_mode = GTR::ePipelineMode::DEFERRED; break;

		
		case SDLK_g: renderer->show_gbuffers = !renderer->show_gbuffers; break;
		case SDLK_v: renderer->show_ao = !renderer->show_ao; break;
		case SDLK_c: renderer->show_ao_deferred = !renderer->show_ao_deferred; break;

		case SDLK_x: renderer->updateIrradianceCache(scene); break;

	}



}

void Application::onKeyUp(SDL_KeyboardEvent event)
{
}

void Application::onGamepadButtonDown(SDL_JoyButtonEvent event)
{

}

void Application::onGamepadButtonUp(SDL_JoyButtonEvent event)
{

}

void Application::onMouseButtonDown( SDL_MouseButtonEvent event )
{
	if (event.button == SDL_BUTTON_MIDDLE) //middle mouse
	{
		//Input::centerMouse();
		mouse_locked = !mouse_locked;
		SDL_ShowCursor(!mouse_locked);
	}
}

void Application::onMouseButtonUp(SDL_MouseButtonEvent event)
{
}

void Application::onMouseWheel(SDL_MouseWheelEvent event)
{
	bool mouse_blocked = false;

	#ifndef SKIP_IMGUI
		ImGuiIO& io = ImGui::GetIO();
		if(!mouse_locked)
		switch (event.type)
		{
			case SDL_MOUSEWHEEL:
			{
				if (event.x > 0) io.MouseWheelH += 1;
				if (event.x < 0) io.MouseWheelH -= 1;
				if (event.y > 0) io.MouseWheel += 1;
				if (event.y < 0) io.MouseWheel -= 1;
			}
		}
		mouse_blocked = ImGui::IsAnyWindowHovered();
	#endif

	if (!mouse_blocked && event.y)
	{
		if (mouse_locked)
			cam_speed *= 1 + (event.y * 0.1);
		else
			camera->changeDistance(event.y * 0.5);
	}
}

void Application::onResize(int width, int height)
{
    std::cout << "window resized: " << width << "," << height << std::endl;
	glViewport( 0,0, width, height );
	camera->aspect =  width / (float)height;
	window_width = width;
	window_height = height;
}

