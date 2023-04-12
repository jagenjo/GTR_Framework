#pragma once
#include "includes.h"
#include "math.h"
#include "../pipeline/camera.h"

namespace CORE {

	typedef SDL_Window Window;

	extern std::string base_path;

	class BaseApplication
	{
	public:
		Window* window;
		static BaseApplication* instance;

		int window_width;
		int window_height;
		bool render_ui;

		//some globals
		long frame;
		float time;
		float elapsed_time;
		int fps;
		bool must_exit;

		BaseApplication();

		virtual void render(void) {}
		virtual void update(double dt) {}

		virtual void renderUI(void) {}

		//events
		virtual void onKeyDown(SDL_KeyboardEvent event) {}
		virtual void onKeyUp(SDL_KeyboardEvent event) {}
		virtual void onMouseButtonDown(SDL_MouseButtonEvent event) {}
		virtual void onMouseButtonUp(SDL_MouseButtonEvent event) {}
		virtual void onMouseWheel(SDL_MouseWheelEvent event) {}
		virtual void onGamepadButtonDown(SDL_JoyButtonEvent event) {}
		virtual void onGamepadButtonUp(SDL_JoyButtonEvent event) {}
		virtual void onResize(int width, int height) {}
		virtual void onFileDrop(std::string filename, std::string relative, SDL_Event event) {};
	};

	void init();
	void initUI();
	Window* createWindow(const char* caption, int width, int height, bool fullscreen = false);
	void mainLoop(CORE::Window* window, BaseApplication* app);

	void renderUI(CORE::Window* window, BaseApplication* app);
	void destroy();

	long getTime();
	Vector2ui getWindowSize();
	Vector2ui getDesktopSize(int display_index);
	void setCursorPosition(int x, int y);
	void showCursor(bool);

	inline long getTime();
	std::string getPath(); //get root path where the app is running
	std::string openFileDialog(const char* default_path = nullptr);
};

	

