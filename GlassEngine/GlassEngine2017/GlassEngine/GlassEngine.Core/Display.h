#pragma once

#include "Vector2Int.h"
#include "GLFW/glfw3.h"

namespace GlassEngine
{
	class Display
	{
	public:
		Display(GLFWmonitor* monitor, Vector2Int size, int refreshRate) :
			m_monitor(monitor), m_size(size), m_refreshRate(refreshRate) {};

		const GLFWmonitor* Monitor() const
		{
			return m_monitor;
		}

		const int RefreshRate() const
		{
			return m_refreshRate;
		}

		const Vector2Int Size() const
		{
			return m_size;
		}

	private:
		Vector2Int m_size;
		int m_refreshRate;
		GLFWmonitor* m_monitor;
	};
}
