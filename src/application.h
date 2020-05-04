/*  by Javi Agenjo 2013 UPF  javi.agenjo@gmail.com
	This class encapsulates the game, is in charge of creating the game, getting the user input, process the update and render.
*/

#ifndef APPLICATION_H
#define APPLICATION_H

#include "includes.h"
#include "camera.h"
#include "utils.h"

class Application
{
public:
	static Application* instance;

	//window
	SDL_Window* window;
	int window_width;
	int window_height;

	//some globals
	long frame;
    float time;
	float elapsed_time;
	int fps;
	bool must_exit;
	bool render_debug;
	bool render_gui;

	//some vars
	bool mouse_locked; //tells if the mouse is locked (blocked in the center and not visible)
	bool render_wireframe; //in case we want to render everything in wireframe mode

	Application( int window_width, int window_height, SDL_Window* window );

	//main functions
	void render( void );
	void update( double dt );

	void renderDebugGUI(void);
	void renderDebugGizmo();

	//events
	void onKeyDown( SDL_KeyboardEvent event );
	void onKeyUp(SDL_KeyboardEvent event);
	void onMouseButtonDown( SDL_MouseButtonEvent event );
	void onMouseButtonUp(SDL_MouseButtonEvent event);
	void onMouseWheel(SDL_MouseWheelEvent event);
	void onGamepadButtonDown(SDL_JoyButtonEvent event);
	void onGamepadButtonUp(SDL_JoyButtonEvent event);
	void onResize(int width, int height);

};


#endif 