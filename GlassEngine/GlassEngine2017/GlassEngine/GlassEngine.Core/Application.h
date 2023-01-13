#pragma once

struct GLFWwindow;

namespace GlassEngine
{
	class Debug;
}

namespace GlassEngine::Core
{
	class Screen;
		

	class Application
	{
	public:
		Application();
		~Application();
		void Run();

	private:
		void InitWindow();

		GLFWwindow* m_window;
		std::unique_ptr<Screen> m_screen;
		std::unique_ptr<Debug> m_debug;
	};

	
}
