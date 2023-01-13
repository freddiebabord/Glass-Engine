#include "stdafx.h"
#include "Application.h"

#include "Screen.h"
#include "Vector2Int.h"
#include "Debug.h"

using namespace GlassEngine::Core;

Application::Application()
{
	m_debug = std::make_unique<Debug>();
	m_screen = std::make_unique<Screen>();
	InitWindow();	
}

Application::~Application()
{
	m_screen.release();
	m_debug.release();
}

void Application::InitWindow()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

	const Vector2Int resolution = m_screen->GetResolution();
	m_window = glfwCreateWindow(resolution.width, resolution.height, "Glass Engine", nullptr, nullptr);

	//TODO: Load icon resource from config
	HICON hIcon = (HICON)LoadImage(NULL, (LPCWCHAR)"Assets/Core/GlassEngineIcon.ico", IMAGE_ICON, 32, 32, LR_LOADFROMFILE);
	SendMessage(glfwGetWin32Window(m_window), WM_SETICON, ICON_BIG, (LPARAM)hIcon);

	int supportedMonitorCount = 5;
	GLFWmonitor** monitors = glfwGetMonitors(&supportedMonitorCount);
	GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
	for(int i = 0; i < supportedMonitorCount; ++i)
	{
		int supportedModesCount = 20;
		const GLFWvidmode* modes = glfwGetVideoModes(monitors[i], &supportedModesCount);
		m_screen->InitDisplay(monitors[i], modes, supportedModesCount);
	}
	const GLFWvidmode* currentPrimaryMonitorMode = glfwGetVideoMode(primaryMonitor);

	glfwSetWindowPos(m_window,
		currentPrimaryMonitorMode->width - resolution.width / 2,
		currentPrimaryMonitorMode->height - resolution.height / 2);

	/*if(glewInit() != GLEW_OK)
	{
		m_debug->LogError("Failed to initialize GLEW", this);
	}*/

	glfwSetWindowUserPointer(m_window, this);
	
	m_debug->Log("Created GLFW window", this);
}

void Application::Run()
{
	while(!glfwWindowShouldClose(m_window))
	{
		glfwPollEvents();
	}
}