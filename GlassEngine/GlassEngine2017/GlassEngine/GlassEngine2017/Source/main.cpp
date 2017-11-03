#include "PCH/stdafx.h"
#include "Application.h"

using namespace GlassEngine;


int main(int argc, char *args[])
{
	Application application;

	try
	{
		application.Run();
	}
	catch (const std::runtime_error& error)
	{
		std::cerr << error.what() << std::endl;
		return EXIT_FAILURE;
	}


    return EXIT_SUCCESS;
}
