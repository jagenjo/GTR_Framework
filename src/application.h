/*  by Javi Agenjo 2013 UPF  javi.agenjo@gmail.com
	This class encapsulates the game, is in charge of creating the game, getting the user input, process the update and render.
*/

#ifndef APPLICATION_H
#define APPLICATION_H

#include "litengine.h"

class Application : public CORE::BaseApplication
{
public:

	//some vars
	bool mouse_locked; //tells if the mouse is locked (blocked in the center and not visible)
	Camera* camera = nullptr;
	SCN::Scene* scene = nullptr;
	SCN::Renderer* renderer = nullptr;
	bool render_debug = true;

	Application();

	//main functions
	void render( void );
	void update( double dt );

	void renderUI(void);

	//events
	void onKeyDown( SDL_KeyboardEvent event );
	void onKeyUp(SDL_KeyboardEvent event);
	void onMouseButtonDown( SDL_MouseButtonEvent event );
	void onMouseButtonUp(SDL_MouseButtonEvent event);
	void onMouseWheel(SDL_MouseWheelEvent event);
	void onGamepadButtonDown(SDL_JoyButtonEvent event);
	void onGamepadButtonUp(SDL_JoyButtonEvent event);
	void onResize(int width, int height);
	void onFileDrop(std::string filename, std::string relative, SDL_Event event );
};


#endif 