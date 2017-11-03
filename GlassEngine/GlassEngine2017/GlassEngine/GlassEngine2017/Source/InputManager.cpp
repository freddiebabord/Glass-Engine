#include "PCH/stdafx.h"
#include "InputManager.h"

namespace GlassEngine
{
	struct Joystick
	{
		int joystickID;
		bool connected;
		const char* name;
		unsigned char* buttonStates;
		float* axisStates;
		const char* joystickName;
		SDL_GameController* controller;
		SDL_Joystick* joystick;
		SDL_Haptic *haptic;
	};

	struct Keyboard
	{
		/// Saves SDL internal keyboard state.
		const uint8_t* keyboard;
		// Saves which keys are currently down.
		///
		/// @note *KEYBOARD_SIZE* is defined on *SDL.hpp*.
		bool keyDown[KEYBOARD_SIZE];

		/// Saves which keys are currently up.
		///
		/// @note *KEYBOARD_SIZE* is defined on *SDL.hpp*.
		bool keyUp[KEYBOARD_SIZE];
	};

	struct Mouse
	{
		glm::vec2 currentPosition, previousPosition;
		float currentScrollPosition, previousScrollPosition;

		float MouseScrollDelta() const { return currentScrollPosition - previousScrollPosition; }
		glm::vec2 MouseDelta() const { return currentPosition - previousPosition; }

		/// Saves which mouse buttons are currently down.
		///
		/// @note *MOUSE_SIZE* is defined on *SDL.hpp*.
		bool mouseDown[MOUSE_MAX];

		/// Saves which mouse buttons are currently up.
		///
		/// @note *MOUSE_SIZE* is defined on *SDL.hpp*.
		bool mouseUp[MOUSE_MAX];

		/// Saves SDL internal mouse state.
		uint32_t mouse;

		int mouseX = 0;
		int mouseY = 0;

		int* MouseX()
		{
			mouseX = static_cast<int>(currentPosition.x);
			return &mouseX;
		}
		int* MouseY()
		{
			mouseY = static_cast<int>(currentPosition.y);
			return &mouseY;
		}
	};

	struct Cursor
	{
		enum State
		{
			Visible,
			HiddenAndUnConfined,
			HiddenAndConfined
		} currentState;

		void SetState(State newState)
		{
			switch (newState)
			{
			case Visible: glfwSetInputMode(InputManager::Instance().GetWindowHandle(), GLFW_CURSOR, GLFW_CURSOR_NORMAL); break;
			case HiddenAndUnConfined: glfwSetInputMode(InputManager::Instance().GetWindowHandle(), GLFW_CURSOR, GLFW_CURSOR_HIDDEN); break;
			case HiddenAndConfined: glfwSetInputMode(InputManager::Instance().GetWindowHandle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED); break;
			default:;
			}
			currentState = newState;
		}
	};

	InputManager* InputManager::_instance = { nullptr };

	void InputManager::Init(GLFWwindow& window)
	{
		this->currentWindow = &window;
		this->keyboard = new Keyboard;
		this->mouse = new Mouse;
		_instance = this;
		glfwSetCursorPosCallback(&window, this->CursorPositionUpdate);
		glfwSetScrollCallback(&window, this->MouseScrollCallback);

		SDL_Init(SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC | SDL_INIT_JOYSTICK | SDL_INIT_EVENTS);
	}

	void InputManager::Update()
	{
		memset(keyboard->keyDown, false, KEYBOARD_SIZE);
		memset(keyboard->keyUp, false, KEYBOARD_SIZE);
		memset(mouse->mouseUp, false, KEYBOARD_SIZE);
		memset(mouse->mouseDown, false, KEYBOARD_SIZE);

		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				// SDL2's new way of handling input
			case SDL_TEXTINPUT:
				// WHAT
				break;

			case SDL_KEYDOWN:
			{
				this->keyboard->keyboard = SDL_GetKeyboardState(nullptr);

				int index = event.key.keysym.scancode;

				this->keyboard->keyDown[index] = true;
			}
			break;

			case SDL_KEYUP:
			{
				this->keyboard->keyboard = SDL_GetKeyboardState(nullptr);

				int index = event.key.keysym.scancode;
				this->keyboard->keyUp[index] = true;
			}
			break;

			case SDL_MOUSEMOTION:
				this->mouse->previousPosition = this->mouse->currentPosition;
				this->mouse->currentPosition = glm::vec2(event.motion.x, event.motion.y);
				break;

			case SDL_MOUSEBUTTONDOWN:
				this->mouse->mouse = SDL_GetMouseState(this->mouse->MouseX(), this->mouse->MouseY());

				if (event.button.button == SDL_BUTTON_LEFT)
					this->mouse->mouseDown[MOUSE_LEFT] = true;

				else if (event.button.button == SDL_BUTTON_MIDDLE)
					this->mouse->mouseDown[MOUSE_MIDDLE] = true;

				else if (event.button.button == SDL_BUTTON_RIGHT)
					this->mouse->mouseDown[MOUSE_RIGHT] = true;
				break;

			case SDL_MOUSEBUTTONUP:
				this->mouse->mouse = SDL_GetMouseState(this->mouse->MouseX(), this->mouse->MouseY());

				if (event.button.button == SDL_BUTTON_LEFT)
					this->mouse->mouseUp[MOUSE_LEFT] = true;

				else if (event.button.button == SDL_BUTTON_MIDDLE)
					this->mouse->mouseUp[MOUSE_MIDDLE] = true;

				else if (event.button.button == SDL_BUTTON_RIGHT)
					this->mouse->mouseUp[MOUSE_RIGHT] = true;
				break;

				// Brand new SDL2 event.
			case SDL_MOUSEWHEEL:
				// event.x; // Ammount scrolled horizontally
				// // If negative, scrolled to the right
				// // If positive, scrolled to the left

				// event.y; // Ammount scrolled vertically
				// // If negative, scrolled down
				// // If positive, scrolled up
				break;

			case SDL_CONTROLLERDEVICEADDED:
				AddController(event.cdevice.which);
				break;

			case SDL_CONTROLLERDEVICEREMOVED:
				RemoveController(event.cdevice.which);
				break;

			case SDL_CONTROLLERBUTTONDOWN:
			case SDL_CONTROLLERBUTTONUP:
				OnControllerButton(event.cbutton);
				break;

			case SDL_CONTROLLERAXISMOTION:
				OnControllerAxis(event.caxis);
				break;

			default:
				break;
			}
		}
	}

	float InputManager::GetAxis(const char* axis)
	{
		return 0;
	}

	bool InputManager::GetButton(const char* name)
	{
		return 0;
	}

	bool InputManager::GetButtonUp(const char* name)
	{
		return 0;
	}

	bool InputManager::GetButtonDown(const char* name)
	{
		return 0;
	}

	bool InputManager::GetButton(MouseButton button)
	{
		if (button == MOUSE_MAX)
			return false;

		return (this->mouse->mouse != 0) & SDL_BUTTON(static_cast<int>(button));
	}

	bool InputManager::GetButtonUp(MouseButton button)
	{
		if (button == MOUSE_MAX)
			return false;

		return this->mouse->mouseUp[button];
	}

	bool InputManager::GetButtonDown(MouseButton button)
	{
		if (button == MOUSE_MAX)
			return false;

		return this->mouse->mouseDown[button];
	}

	bool InputManager::GetKey(Key key)
	{
		int sdl_key = static_cast<int>(key);

		return this->keyboard->keyboard[sdl_key] != 0;
	}

	bool InputManager::GetKeyUp(Key key)
	{
		if (key < 0 || key >= KEYBOARD_SIZE)
			return false;

		return (this->keyboard->keyUp[key]);
	}

	bool InputManager::GetKeyDown(Key key)
	{
		if (key < 0 || key >= KEYBOARD_SIZE)
			return false;

		return (this->keyboard->keyDown[key]);
	}

	void InputManager::AddController(int id)
	{
		if (SDL_IsGameController(id))
		{
			SDL_GameController* pad = SDL_GameControllerOpen(id);

			Joystick* joystickInternal = new Joystick;

			if (pad)
			{
				
				joystickInternal->joystick = SDL_GameControllerGetJoystick(pad);
				joystickInternal->joystickID = SDL_JoystickInstanceID(joystickInternal->joystick);

				

				// You can add to your own map of joystick IDs to controllers here.
				//YOUR_FUNCTION_THAT_CREATES_A_MAPPING(id, pad);


				
			}

			if (SDL_JoystickIsHaptic(joystickInternal->joystick)) {
				joystickInternal->haptic = SDL_HapticOpenFromJoystick(joystickInternal->joystick);
				printf("Haptic Effects: %d\n", SDL_HapticNumEffects(joystickInternal->haptic));
				printf("Haptic Query: %x\n", SDL_HapticQuery(joystickInternal->haptic));
				if (SDL_HapticRumbleSupported(joystickInternal->haptic)) {
					if (SDL_HapticRumbleInit(joystickInternal->haptic) != 0) {
						printf("Haptic Rumble Init Error: %s\n", SDL_GetError());
						SDL_HapticClose(joystickInternal->haptic);
						joystickInternal->haptic = 0;
					}
				}
				else {
					SDL_HapticClose(joystickInternal->haptic);
					joystickInternal->haptic = 0;
				}
			}

			if(id < joysticks.size())
			{
				delete joysticks[id];
				joysticks[id] = joystickInternal;
			}
			else
				joysticks.push_back(joystickInternal);
		}
	}

	void InputManager::RemoveController(int id)
	{
		SDL_GameController* pad = GetControllerWithID(id);
		SDL_GameControllerClose(pad);
	}

	void InputManager::OnControllerButton(const SDL_ControllerButtonEvent sdlEvent)
	{
		joysticks[int(sdlEvent.which)]->buttonStates[sdlEvent.button] = sdlEvent.state;
	}

	void InputManager::OnControllerAxis(const SDL_ControllerAxisEvent sdlEvent)
	{
		joysticks[int(sdlEvent.which)]->buttonStates[sdlEvent.axis] = static_cast<unsigned char>(sdlEvent.value);
	}

	SDL_GameController* InputManager::GetControllerWithID(int idx)
	{
		return joysticks[idx]->controller;
	}


	void InputManager::CursorPositionUpdate(GLFWwindow* window, double xPos, double yPos)
	{
		_instance->mouse->previousPosition = _instance->mouse->currentPosition;
		_instance->mouse->currentPosition = glm::vec2(float(xPos), float(yPos));
	}


	void InputManager::MouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
	{
		_instance->mouse->previousScrollPosition = _instance->mouse->currentScrollPosition;
		_instance->mouse->currentScrollPosition += float(yoffset);
	}
}
