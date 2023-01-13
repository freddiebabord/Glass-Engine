#include "stdafx.h"
#include "Screen.h"

using namespace GlassEngine;
using namespace GlassEngine::Core;

void Screen::InitDisplay(GLFWmonitor* monitor, const GLFWvidmode* modes, int modeCount)
{
	for(size_t i = 0; i < modeCount; ++i)
	{
		auto& displayMode = modes[i];
		Display additionalDisplay(monitor, Vector2Int(displayMode.width, displayMode.height), displayMode.refreshRate);
		m_displays.push_back(additionalDisplay);
	}
}

Vector2Int Screen::GetResolution() const
{
	return Vector2Int(m_width, m_height);
}

void Screen::SetResolution(int width, int height)
{
	if (width != m_width || height != m_height)
	{
		m_width = width;
		m_height = height;

		// TODO: Throw on screen size changed event
	}
}