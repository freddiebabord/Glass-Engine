#pragma once

#include "Vector2Int.h"
#include "Display.h"
#include "GLFW/glfw3.h"

namespace GlassEngine::Core
{
	class Screen
	{
	public:
		void InitDisplay(GLFWmonitor* monitor, const GLFWvidmode* modes, int modeCount);
		void SetResolution(int width, int height);
		Vector2Int GetResolution() const;
		
	private:
		int m_width = 1280;
		int m_height = 720;
		std::vector<Display> m_displays;
	};

	
}
