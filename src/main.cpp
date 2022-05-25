/*  by Javi Agenjo 2013 UPF  javi.agenjo@gmail.com

	MAIN:
	 + This file creates the window and the application instance. 
	 + It also contains the mainloop
	 + This is the lowest level, here we access the system to create the opengl Context
	 + It takes all the events from SDL and redirect them to the application
*/

#include "includes.h"

#include "framework.h"
#include "mesh.h"
#include "camera.h"
#include "utils.h"
#include "input.h"
#include "application.h"
#include "task.h"

#include <iostream> //to output

long last_time = 0; //this is used to calcule the elapsed time between frames

Application* app = NULL;
SDL_GLContext glcontext;

// *********************************
//create a window using SDL
SDL_Window* createWindow(const char* caption, int width, int height, bool fullscreen = false)
{
    int multisample = 8;
    bool retina = false; //change this to use a retina display

	//set attributes
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16); //or 24
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

#ifndef __APPLE__
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif
    
	//antialiasing (disable this lines if it goes too slow)
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, multisample ); //increase to have smoother polygons

	// Initialize the joystick subsystem
	SDL_InitSubSystem(SDL_INIT_JOYSTICK);

	//create the window
	SDL_Window * sdl_window = SDL_CreateWindow(caption, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE|
                                          (retina ? SDL_WINDOW_ALLOW_HIGHDPI:0) |
                                          (fullscreen?SDL_WINDOW_FULLSCREEN_DESKTOP:0) );
	if(!sdl_window)
	{
		fprintf(stderr, "Window creation error: %s\n", SDL_GetError());
		exit(-1);
	}
  
	// Create an OpenGL context associated with the window.
	glcontext = SDL_GL_CreateContext(sdl_window);

	//in case of exit, call SDL_Quit()
	atexit(SDL_Quit);

	//get events from the queue of unprocessed events
	SDL_PumpEvents(); //without this line asserts could fail on windows

	//launch glew to extract the opengl extensions functions from the DLL
	#ifdef USE_GLEW
		glewInit();
	#endif

	int window_width, window_height;
	SDL_GetWindowSize(sdl_window, &window_width, &window_height);
	std::cout << " * Window size: " << window_width << " x " << window_height << std::endl;
	std::cout << " * OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
	std::cout << " * Path: " << getPath() << std::endl;
	std::cout << std::endl;

	return sdl_window;
}

//called from mainLoop
void renderDebug(SDL_Window* window, Application * app)
{
	#ifndef SKIP_IMGUI
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.MousePos.x = Input::mouse_position.x;
	io.MousePos.y = Input::mouse_position.y;

	// Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame(window);
	ImGui::NewFrame();
	ImGuizmo::BeginFrame();

	app->renderDebugGizmo();

	if (app->render_debug)
	{
		if (ImGui::Begin("Debugger"))// Create a window
			app->renderDebugGUI();
		ImGui::End();
	}

	// Rendering
	ImGui::EndFrame();
	ImGui::Render();
	glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	glGetError();
	#endif
}

//The application main loop
void mainLoop(SDL_Window* window)
{
	SDL_Event sdlEvent;

	long start_time = SDL_GetTicks();
	long now = start_time;
	long frames_this_second = 0;

	TaskManager::background.startThread();

	while (!app->must_exit)
	{
		//render frame
		app->render();
		if (app->render_gui)
			renderDebug(window, app);
		// swap between front buffer and back buffer
		SDL_GL_SwapWindow(window);

		//update events
		while(SDL_PollEvent(&sdlEvent))
		{
			switch (sdlEvent.type)
			{
			case SDL_QUIT: return; break; //EVENT for when the user clicks the [x] in the corner
			case SDL_MOUSEBUTTONDOWN: //EXAMPLE OF sync mouse input
				Input::mouse_state |= SDL_BUTTON(sdlEvent.button.button);
				app->onMouseButtonDown(sdlEvent.button);
				break;
			case SDL_MOUSEBUTTONUP:
				Input::mouse_state &= ~SDL_BUTTON(sdlEvent.button.button);
				app->onMouseButtonUp(sdlEvent.button);
				break;
			case SDL_MOUSEWHEEL:
				Input::mouse_wheel += sdlEvent.wheel.y;
				Input::mouse_wheel_delta = sdlEvent.wheel.y;
				app->onMouseWheel(sdlEvent.wheel);
				break;
			case SDL_KEYDOWN:
				app->onKeyDown(sdlEvent.key);
				break;
			case SDL_KEYUP:
				app->onKeyUp(sdlEvent.key);
				break;
			case SDL_JOYBUTTONDOWN:
				app->onGamepadButtonDown(sdlEvent.jbutton);
				break;
			case SDL_JOYBUTTONUP:
				app->onGamepadButtonUp(sdlEvent.jbutton);
				break;
			case SDL_TEXTINPUT:
				// you can read the ASCII character from sdlEvent.text.text 
				break;
			case SDL_WINDOWEVENT:
				switch (sdlEvent.window.event) {
				case SDL_WINDOWEVENT_RESIZED: //resize opengl context
					app->onResize(sdlEvent.window.data1, sdlEvent.window.data2);
					break;
				}
			}
		}

		Input::update();

		//compute delta time
		long last_time = now;
		now = SDL_GetTicks();
		double elapsed_time = (now - last_time) * 0.001; //0.001 converts from milliseconds to seconds
		double last_time_seconds = app->time;
		app->time = float(now * 0.001);
		app->elapsed_time = elapsed_time;
		app->frame++;
		frames_this_second++;
		if (int(last_time_seconds *2) != int(app->time*2)) //next half second
		{
			app->fps = frames_this_second*2;
			frames_this_second = 0;
		}

		//update app logic
		app->update(elapsed_time);

		//execute a task in the main task manager (blocking)
		TaskManager::foreground.fetchTask();

		//check errors in opengl only when working in debug
		#ifdef _DEBUG
				checkGLErrors();
		#endif
	}

	return;
}

int main(int argc, char **argv)
{
	std::cout << "Initiating app..." << std::endl;

	//prepare SDL
	SDL_Init(SDL_INIT_EVERYTHING);

	bool fullscreen = false; //change this to go fullscreen
	Vector2 size(1024,768);

	if(fullscreen)
		size = getDesktopSize(0);

	//create the application window (WINDOW_WIDTH and WINDOW_HEIGHT are two macros defined in includes.h)
	SDL_Window*window = createWindow("GTR", (int)size.x, (int)size.y, fullscreen );
	if (!window)
		return 0;
	int window_width, window_height;
	SDL_GetWindowSize(window, &window_width, &window_height);

	// Setup Dear ImGui context
	#ifndef SKIP_IMGUI
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		//ImGui::StyleColorsClassic();

		// Setup Platform/Renderer bindings
		const char* glsl_version = "#version 130";
		ImGui_ImplSDL2_InitForOpenGL(window, glcontext);
		ImGui_ImplOpenGL3_Init(glsl_version);
	#endif

	Input::init(window);

	//launch the application (app is a global variable)
	app = new Application(window_width, window_height, window);

	//main loop, application gets inside here till user closes it
	mainLoop(window);

	//save state and free memory
	// Cleanup
	#ifndef SKIP_IMGUI
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
	#endif

	SDL_GL_DeleteContext(glcontext);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
