#include "core.h"

#include "input.h"
#include "task.h"
#include "ui.h"

#include "../gfx/gfx.h" //check errors
#include "../gfx/texture.h" //??
#include "../utils/utils.h" //cleanPath

#ifdef WIN32
	#include <Commdlg.h>
	#include <atlstr.h>
	#include <windows.h> //time
#else
	#include <sys/time.h>
#endif

SDL_GLContext glcontext;
SDL_Window* current_window = nullptr;
long last_time = 0; //this is used to calcule the elapsed time between frames
std::string CORE::base_path;

CORE::BaseApplication* CORE::BaseApplication::instance = nullptr;

CORE::BaseApplication::BaseApplication()
{
	window = nullptr;
	assert(instance == nullptr); //another app created??
	instance = this;
	render_ui = true;

	//some globals
	frame = 0;
	time = 0;
	elapsed_time = 0;
	fps = 0;
	must_exit = false;

	vec2 window_size = CORE::getWindowSize();
	this->window_width = (int)window_size.x;
	this->window_height = (int)window_size.y;
}

void CORE::init()
{
	//prepare SDL
	SDL_Init(SDL_INIT_EVERYTHING);
	Input::init();
	TaskManager::background.startThread();
}

//create a window using SDL
CORE::Window* CORE::createWindow(const char* caption, int width, int height, bool fullscreen)
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
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, OPENGL_VERSION_MAJOR);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, OPENGL_VERSION_MINOR);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif

	//antialiasing (disable this lines if it goes too slow)
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, multisample); //increase to have smoother polygons

	// Initialize the joystick subsystem
	SDL_InitSubSystem(SDL_INIT_JOYSTICK);

	//create the window
	SDL_Window* sdl_window = SDL_CreateWindow(caption, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
		(retina ? SDL_WINDOW_ALLOW_HIGHDPI : 0) |
		(fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0));
	if (!sdl_window)
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

	SDL_EventState(SDL_DROPFILE, SDL_ENABLE);

	//launch glew to extract the opengl extensions functions from the DLL
#ifdef USE_GLEW
	glewInit();
#endif

	int window_width, window_height;
	SDL_GetWindowSize(sdl_window, &window_width, &window_height);
	base_path = cleanPath( getPath() );
	std::cout << " * Window size: " << window_width << " x " << window_height << std::endl;
	std::cout << " * OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
	std::cout << " * Path: " << base_path  << std::endl;
	std::cout << std::endl;

	current_window = sdl_window;

	//init imgui
	initUI();

	return sdl_window;
}

void CORE::initUI()
{
// Setup Dear ImGui context
#ifndef SKIP_IMGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	// Setup Platform/Renderer bindings
	const char* glsl_version = "#version 130";
	ImGui_ImplSDL2_InitForOpenGL(current_window, glcontext);
	ImGui_ImplOpenGL3_Init(glsl_version);
#endif

	UI::init();
}


void CORE::mainLoop(CORE::Window* window, BaseApplication* app)
{
	SDL_Event sdlEvent;

	long start_time = CORE::getTime();
	long now = start_time;
	long frames_this_second = 0;
	int history_pos = 0;

	memset(GFX::gpu_frame_microseconds_history, 0, sizeof(GFX::gpu_frame_microseconds_history));
	GFX::GPUQuery gputime(GL_TIME_ELAPSED);
	GFX::checkGLErrors();

	while (!app->must_exit)
	{
		//add timer to check gpu frame time with precission instead of using CPU
		if (gputime.isReady())
		{
			GFX::gpu_frame_microseconds = gputime.value / 1000;
			GFX::gpu_frame_microseconds_history[history_pos] = GFX::gpu_frame_microseconds;
			history_pos = (history_pos + 1) % GPU_FRAME_HISTORY_SIZE;
		}
		gputime.start();

		//render frame
		GFX::startGPULabel("Frame");
			GFX::checkGLErrors();
			app->render();
			GFX::checkGLErrors();
		GFX::endGPULabel();

		//render graphical user interface
		if (app->render_ui)
			renderUI(window, app);

		GFX::checkGLErrors();
		gputime.finish();

		// swap between front buffer and back buffer
		SDL_GL_SwapWindow(window);

		//update events
		while (SDL_PollEvent(&sdlEvent))
		{
			bool ui_mouse_want_capture = false;
			bool ui_keyboard_want_capture = false;
			#ifndef SKIP_IMGUI
				ImGui_ImplSDL2_ProcessEvent(&sdlEvent);
				ImGuiIO& io = ImGui::GetIO();
				ui_mouse_want_capture = io.WantCaptureMouse;
				ui_keyboard_want_capture = io.WantCaptureKeyboard;
			#endif
			
			switch (sdlEvent.type)
			{
			case SDL_QUIT: return; break; //EVENT for when the user clicks the [x] in the corner
			case SDL_MOUSEBUTTONDOWN: //EXAMPLE OF sync mouse input
				Input::mouse_state |= SDL_BUTTON(sdlEvent.button.button);
				if(!ui_mouse_want_capture)
					app->onMouseButtonDown(sdlEvent.button);
				break;
			case SDL_MOUSEBUTTONUP:
				Input::mouse_state &= ~SDL_BUTTON(sdlEvent.button.button);
				if (!ui_mouse_want_capture)
					app->onMouseButtonUp(sdlEvent.button);
				break;
			case SDL_MOUSEWHEEL:
				Input::mouse_wheel += sdlEvent.wheel.y;
				Input::mouse_wheel_delta = sdlEvent.wheel.y;
				app->onMouseWheel(sdlEvent.wheel);
				break;
			case SDL_KEYDOWN:
				if (!ui_keyboard_want_capture)
					app->onKeyDown(sdlEvent.key);
				break;
			case SDL_KEYUP:
				if(!ui_keyboard_want_capture)
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
				break;
			case SDL_DROPFILE:
				std::string filename = cleanPath(sdlEvent.drop.file);
				std::string relpath = makePathRelative(filename);
				std::cout << "File Drop: " << relpath << std::endl;
				app->onFileDrop(filename, relpath, sdlEvent);
				SDL_free(sdlEvent.drop.file);
				break;
			}
		}

		Input::update();

		//compute delta time
		long last_time = now;
		now = CORE::getTime();
		double elapsed_time = (double)(now - last_time) * 0.001; //0.001 converts from milliseconds to seconds
		double last_time_seconds = app->time;
		app->time = float(now * 0.001);
		app->elapsed_time = (float)elapsed_time;
		app->frame++;
		frames_this_second++;
		if (int(last_time_seconds * 2) != int(app->time * 2)) //next half second
		{
			app->fps = frames_this_second * 2;
			frames_this_second = 0;
		}

		//update app logic
		app->update(elapsed_time);

		//execute a task in the main task manager (blocking)
		TaskManager::foreground.fetchTask();

		//check errors in opengl only when working in debug
#ifdef _DEBUG
		GFX::checkGLErrors();
#endif
	}

	return;
}

void CORE::destroy()
{
	// Cleanup
#ifndef SKIP_IMGUI
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
#endif

	SDL_GL_DeleteContext(glcontext);
	SDL_DestroyWindow(current_window);

	SDL_Quit();
}

void CORE::renderUI(CORE::Window* window, BaseApplication* app)
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

	app->renderUI();

	// Render toasts on top of everything, at the end of your code!
	// You should push style vars here
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.f);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(43.f / 255.f, 43.f / 255.f, 43.f / 255.f, 100.f / 255.f));
	UI::drawNotifications();
	ImGui::PopStyleVar(1); // Don't forget to Pop()
	ImGui::PopStyleColor(1);

	// Rendering
	ImGui::EndFrame();
	ImGui::Render();
	glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	glGetError();
#endif
}

Vector2ui CORE::getWindowSize()
{
	assert(current_window && "Window must be initialized first");
	int window_width, window_height;
	SDL_GetWindowSize(current_window, &window_width, &window_height);
	return Vector2ui(window_width, window_height);
}

Vector2ui CORE::getDesktopSize(int display_index)
{
	SDL_DisplayMode current;
	// Get current display mode of all displays.
	int should_be_zero = SDL_GetCurrentDisplayMode(display_index, &current);
	return Vector2ui(current.w, current.h);
}

void CORE::showCursor(bool v)
{
	SDL_ShowCursor(v); //hide or show the mouse
}

void CORE::setCursorPosition(int x, int y)
{
	SDL_WarpMouseInWindow(current_window, x, y);
}

long CORE::getTime()
{
	return (long)SDL_GetTicks();
}

//Retrieve the current path of the application
#ifdef __APPLE__
#include "CoreFoundation/CoreFoundation.h"
#endif

#ifdef WIN32
#include <direct.h>
#define GetCurrentDir _getcwd
#else
#include <unistd.h>
#define GetCurrentDir getcwd
#endif

std::string CORE::getPath()
{
	std::string fullpath;
	// ----------------------------------------------------------------------------
	// This makes relative paths work in C++ in Xcode by changing directory to the Resources folder inside the .app bundle
#ifdef __APPLE__
	CFBundleRef mainBundle = CFBundleGetMainBundle();
	CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
	char path[PATH_MAX];
	if (!CFURLGetFileSystemRepresentation(resourcesURL, TRUE, (UInt8*)path, PATH_MAX))
	{
		// error!
	}
	CFRelease(resourcesURL);
	chdir(path);
	fullpath = path;
#else
	char cCurrentPath[1024];
	if (!GetCurrentDir(cCurrentPath, sizeof(cCurrentPath)))
		return "";

	cCurrentPath[sizeof(cCurrentPath) - 1] = '\0';
	fullpath = cCurrentPath;

#endif    
	return cleanPath(fullpath);
}


std::string CORE::openFileDialog(const char* default_path)
{
	
#ifdef WIN32
	OPENFILENAMEA ofn;
	char szFile[255] = { '\0' };
	//strcpy(szFile, getPath().c_str());

	// open a file name
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = "All\0*.*\0";
	ofn.lpstrTitle = "Open File";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = default_path ? default_path  : "data/";
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	std::string current_path = getPath();

	std::cout << "opening file dialog" << std::endl;
	if (GetOpenFileNameA(&ofn))
	{
		SetCurrentDirectoryA(current_path.c_str());
		std::cout << "file selected: " << ofn.lpstrFile << std::endl;
		std::cout << "Current Path: " << getPath() << std::endl;
		return ofn.lpstrFile;
	}

	SetCurrentDirectoryA(current_path.c_str());
	std::cout << "no file selected" << std::endl;
	std::cout << "Current Path: " << getPath() << std::endl;
	//return CW2A(ofn.lpstrFile);
	return "";
#else
	return "";
#endif
}

