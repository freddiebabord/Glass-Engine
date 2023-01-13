// main.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

#include "../GlassEngine.Core/Application.h"

int main()
{
	try
	{
		GlassEngine::Core::Application application;
		application.Run();
	}
	catch (const std::exception& exception)
	{
		std::cerr << exception.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

