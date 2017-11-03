#pragma once
#include "InputDefinitions.h"

namespace GlassEngine
{

	struct Joystick;
	struct Keyboard;
	struct Mouse;
	struct Cursor;

	class InputManager
	{
	public:
		

		static InputManager& Instance()
		{
			return *_instance;
		}

		void Init(GLFWwindow& window);

		GLFWwindow* GetWindowHandle()
		{
			return currentWindow;
		}
		

		void Update();

		float GetAxis(const char* axis);
		
		bool GetButton(const char* name);
		bool GetButtonUp(const char* name);
		bool GetButtonDown(const char* name);

		bool GetButton(MouseButton button);
		bool GetButtonUp(MouseButton button);
		bool GetButtonDown(MouseButton button);

		bool GetKey(Key key);
		bool GetKeyUp(Key key);
		bool GetKeyDown(Key key);

		void AddController(int id);

		void RemoveController(int id);

		void OnControllerButton(const SDL_ControllerButtonEvent sdlEvent);

		void OnControllerAxis(const SDL_ControllerAxisEvent sdlEvent);

	private:
		
		SDL_GameController* GetControllerWithID(int idx);



		static void CursorPositionUpdate(GLFWwindow *window, double xPos, double yPos);
		static void MouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

		static InputManager* _instance;

		std::vector<Joystick*> joysticks;
		Keyboard* keyboard = nullptr;
		Mouse* mouse = nullptr;
		GLFWwindow* currentWindow = nullptr;
	};

	

	
}
