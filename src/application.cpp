#include "application.h"

#include <cmath>
#include <string>
#include <cstdio>

#include "editor.h"
#include "pipeline/light.h"

std::vector<vec3> debug_points; //useful

float cam_speed = 25;

SceneEditor* editor = nullptr;

Application::Application()
{
	instance = this;
	mouse_locked = false;



	//define valid entities (DO IT BEFORE LOADING ANY SCENE!!!)
	REGISTER_ENTITY_TYPE(SCN::PrefabEntity);
	//add here your own entities
	REGISTER_ENTITY_TYPE(SCN::LightEntity);
	//...

	// Create camera
	camera = new Camera();
	camera->lookAt(vec3(-150.f, 150.0f, 250.f), vec3(0.f, 0.0f, 0.f), vec3(0.f, 1.f, 0.f));
	camera->setPerspective( 45.f, window_width/(float)window_height, 1.0f, 10000.f);

	//load scene
	scene = new SCN::Scene();
	if (!scene->load("data/scene.json"))
		exit(1);

	camera->lookAt(scene->main_camera.eye, scene->main_camera.center, vec3(0, 1, 0));
	camera->fov = scene->main_camera.fov;

	//loads and compiles several shaders from one single file
	//change to "data/shader_atlas_osx.txt" if you are in XCODE
#ifdef __APPLE__
	const char* shader_atlas_filename = "data/shader_atlas_osx.txt";
#else
	const char* shader_atlas_filename = "data/shader_atlas.glsl";
#endif
	//This class will be the one in charge of rendering all 
	renderer = new SCN::Renderer(shader_atlas_filename); //here so we have opengl ready in constructor

	//our scene editor
	editor = new SceneEditor(scene, renderer);

	//hide the cursor
	CORE::showCursor(!mouse_locked); //hide or show the mouse
}

//what to do when the image has to be draw
void Application::render(void)
{
	//no need to do it here but in case...
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//set the camera as default (used by some functions in the framework)
	camera->enable();

	//render the whole scene
	renderer->renderScene(scene, camera);
	
	//Draw the floor grid, helpful to have a reference point
	if (render_debug)
	{
		GFX::drawGrid();

		//render debug points 
		glDisable(GL_DEPTH_TEST);
		renderer->renderPoints(debug_points, Vector4f(1, 1, 0, 1));
	}

    glDisable(GL_DEPTH_TEST);
    //render anything in the gui after this
}

void Application::update(double seconds_elapsed)
{
	float speed = seconds_elapsed * cam_speed; //the speed is defined by the seconds_elapsed so it goes constant
	float orbit_speed = seconds_elapsed * 0.5f;
	
	//async input to move the camera around
	if (Input::isKeyPressed(SDL_SCANCODE_LSHIFT)) speed *= 10; //move faster with left shift
	if (!Input::isKeyPressed(SDL_SCANCODE_LCTRL))
	{
		if (Input::isKeyPressed(SDL_SCANCODE_W) || Input::isKeyPressed(SDL_SCANCODE_UP)) camera->move(vec3(0.0f, 0.0f, 1.0f) * speed);
		if (Input::isKeyPressed(SDL_SCANCODE_S) || Input::isKeyPressed(SDL_SCANCODE_DOWN)) camera->move(vec3(0.0f, 0.0f, -1.0f) * speed);
		if (Input::isKeyPressed(SDL_SCANCODE_A) || Input::isKeyPressed(SDL_SCANCODE_LEFT)) camera->move(vec3(1.0f, 0.0f, 0.0f) * speed);
		if (Input::isKeyPressed(SDL_SCANCODE_D) || Input::isKeyPressed(SDL_SCANCODE_RIGHT)) camera->move(vec3(-1.0f, 0.0f, 0.0f) * speed);
	}

	//mouse input to rotate the cam
	#ifndef SKIP_IMGUI
	bool mouse_in_ui = ImGui::IsAnyItemHovered() || ImGui::IsAnyItemHovered() || ImGui::IsAnyItemActive();
	if (!ImGuizmo::IsUsing() && !mouse_in_ui)
	#endif
	{
		if (mouse_locked || Input::mouse_state & SDL_BUTTON(SDL_BUTTON_LEFT)) //move in first person view
		{
			camera->rotate(-Input::mouse_delta.x * orbit_speed * 0.5f, vec3(0, 1, 0));
			vec3 right = camera->getLocalVector(vec3(1, 0, 0));
			camera->rotate(-Input::mouse_delta.y * orbit_speed * 0.5f, right);
		}
	}
	
	//move up or down the camera using Q and E
	if (Input::isKeyPressed(SDL_SCANCODE_Q)) camera->moveGlobal(vec3(0.0f, -1.0f, 0.0f) * speed);
	if (Input::isKeyPressed(SDL_SCANCODE_E)) camera->moveGlobal(vec3(0.0f, 1.0f, 0.0f) * speed);

	//to navigate with the mouse fixed in the middle
	CORE::showCursor(!mouse_locked);
	#ifndef SKIP_IMGUI
		ImGui::SetMouseCursor(mouse_locked ? ImGuiMouseCursor_None : ImGuiMouseCursor_Arrow);
	#endif
	if (mouse_locked)
	{
		Input::centerMouse();
	}
}

//called to render the GUI from
void Application::renderUI(void)
{
	editor->render(camera);
}

//Keyboard event handler (sync input)
void Application::onKeyDown( SDL_KeyboardEvent event )
{
	if (render_ui)
	{
		//pass the event to the editor
		if (editor->onKeyDown(event))
			return;
	}

	switch(event.keysym.sym)
	{
		case SDLK_ESCAPE: must_exit = true; break; //ESC key, kill the app
		case SDLK_TAB: render_ui = !render_ui; break;
		case SDLK_F5: GFX::Shader::ReloadAll(); break;
		case SDLK_F6: //refresh
			scene->clear();
			scene->load(scene->filename.c_str());
			camera->lookAt(scene->main_camera.eye, scene->main_camera.center, Vector3f(0, 1, 0));
			camera->fov = scene->main_camera.fov;
			break;
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
	editor->onMouseButtonDown(event);

	if (event.button == SDL_BUTTON_MIDDLE) //middle mouse
	{
		//Input::centerMouse();
		mouse_locked = !mouse_locked;
		SDL_ShowCursor(!mouse_locked);
	}
}

void Application::onMouseButtonUp(SDL_MouseButtonEvent event)
{
	editor->onMouseButtonUp(event);
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
		mouse_blocked = ImGui::IsAnyItemHovered();
	#endif

	if (!mouse_blocked && event.y)
		cam_speed *= 1.0f + (event.y * 0.1f);
}

void Application::onResize(int width, int height)
{
    std::cout << "window resized: " << width << "," << height << std::endl;
	glViewport( 0,0, width, height );
	camera->aspect =  width / (float)height;
	window_width = width;
	window_height = height;
}

void Application::onFileDrop(std::string filename, std::string relative, SDL_Event event)
{
	editor->onFileDrop(filename, relative, event);
}



