#include "input.h"

const Uint8* Input::keystate = NULL;
Uint8 Input::prev_keystate[SDL_NUM_SCANCODES]; //previous before

//mouse state
int Input::mouse_state; //tells which buttons are pressed
Vector2 Input::mouse_position; //last mouse position
Vector2 Input::mouse_delta; //mouse movement in the last frame
float Input::mouse_wheel;
float Input::mouse_wheel_delta;

//gamepad state
GamepadState Input::gamepads[4];

//internal
SDL_Joystick* _joy[4];
SDL_Window* window = NULL;

//************************************************

void Input::init( SDL_Window* _window )
{
	int x, y;
	window = _window;
	SDL_GetMouseState(&x, &y);
	Input::mouse_position.set((float)x, (float)y);
	Input::keystate = SDL_GetKeyboardState(NULL);
	mouse_wheel = 0.0;

	for (int i = 0; i < 4; ++i)
	{
		//open gamepads
		_joy[i] = openGamepad(i);
		updateGamepadState(_joy[i], gamepads[i]);
	}
}

void Input::update()
{
	//read keyboard state and stored in keystate
	memcpy((void*)&Input::prev_keystate, Input::keystate, SDL_NUM_SCANCODES);
	Input::keystate = SDL_GetKeyboardState(NULL);

	//get mouse position and delta (do after pump events)
	int x, y;
	Input::mouse_state = SDL_GetMouseState(&x, &y);
	Input::mouse_delta.set(Input::mouse_position.x - x, Input::mouse_position.y - y);
	Input::mouse_position.set((float)x, (float)y);

	//gamepads
	for (int i = 0; i < 4; ++i)
		updateGamepadState(_joy[i], gamepads[i]);
}

void Input::centerMouse()
{
	int window_width, window_height;
	SDL_GetWindowSize(window, &window_width, &window_height);

	int center_x = (int)floor(window_width*0.5f);
	int center_y = (int)floor(window_height*0.5f);
	SDL_WarpMouseInWindow(window, center_x, center_y); //put the mouse back in the middle of the screen
	Input::mouse_position.x = (float)center_x;
	Input::mouse_position.y = (float)center_y;
}

SDL_Joystick* Input::openGamepad(int index)
{
	// Check for number of joysticks available
	if (SDL_NumJoysticks() <= index)
		return NULL;

	SDL_Joystick* j = SDL_JoystickOpen(index);
	std::cout << " * Gamepad found: " << SDL_JoystickName(j) << " Axis: " << SDL_JoystickNumAxes(j) << "  Buttons: " << SDL_JoystickNumButtons(j) << std::endl;

	// Open joystick and return it
	return j;
}

void Input::updateGamepadState(SDL_Joystick* joystick, GamepadState& state)
{
	//save old state
	int prev_direction = state.direction;
	char prev_button[16];
	memcpy(prev_button, state.button, 16);

	//reset all gamepad state
	memset(&state, 0, sizeof(GamepadState));
	state.connected = false;

	if (joystick == NULL)
		return;

	state.connected = true;
	state.model = SDL_JoystickName((::SDL_Joystick*) joystick);

	//state.axis_translator = strcmp(name, "XInput Controller #1") == 0 ? XInput : NULL;

	state.num_axis = SDL_JoystickNumAxes((::SDL_Joystick*) joystick);
	state.num_buttons = SDL_JoystickNumButtons((::SDL_Joystick*)joystick);

	if (state.num_axis > 8) state.num_axis = 8;
	if (state.num_buttons > 16) state.num_buttons = 16;

	for (int i = 0; i < state.num_axis; ++i) //axis
	{
		float axis_value = SDL_JoystickGetAxis((::SDL_Joystick*) joystick, i) / 32768.0f; //range -32768 to 32768
		Uint8 num = i;

		//windows 7 maps axis different that windows 10
		if (state.num_axis == 5) 
			num = axis5[i];
		else if (state.num_axis == 6)
			num = axis6[i];
		state.axis[num] = axis_value;
	}
	if (state.num_axis == 5)
	{
		state.axis[TRIGGER_LEFT] = -state.axis[TRIGGERS];
		state.axis[TRIGGER_RIGHT] = state.axis[TRIGGERS];
	}
	else if (state.num_axis == 6)
		state.axis[TRIGGERS] = state.axis[TRIGGER_RIGHT] - state.axis[TRIGGER_LEFT];

	for (int i = 0; i < state.num_buttons; ++i) //buttons
	{
		float value = SDL_JoystickGetButton((::SDL_Joystick*) joystick, i);
		Uint8 num = i;
		if (state.num_buttons == 15)
		{
			if (i < 4 || i > 13) //ignore HAT buttons 
				continue;
			num = buttons_15[i];
		}
		//if(value) std::cout << "B: " << int(i) << "->" << int(num) << std::endl;
		if(num >= 0)
			state.button[num] = value;
	}
	state.hat = (HATState)(SDL_JoystickGetHat((::SDL_Joystick*) joystick, 0) - SDL_HAT_CENTERED); //one hat is enough
	memcpy(state.prev_button, prev_button, 16); //copy prev buttons state

	Vector2 axis_direction(state.axis[LEFT_ANALOG_X], state.axis[LEFT_ANALOG_Y]);
	state.prev_direction = prev_direction;
	state.direction = 0;
	float limit = 0.6;
	if (axis_direction.x < -limit)
		state.direction |= PAD_LEFT;
	else if (axis_direction.x > limit)
		state.direction |= PAD_RIGHT;
	if (axis_direction.y < -limit)
		state.direction |= PAD_UP;
	else if (axis_direction.y > limit)
		state.direction |= PAD_DOWN;
}
