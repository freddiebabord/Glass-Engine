#include "stdafx.h"
#include "Debug.h"

using namespace GlassEngine;

void Debug::Log(const char* message, void* target)
{
	std::cout << "[" << typeid(target).name() << "]" << message << std::endl;
}

void Debug::LogWarning(const char* message, void* target)
{
	std::cout << "[WARN]" << "[" << typeid(target).name() << "]" << message << std::endl;
}

void Debug::LogError(const char* message, void* target)
{
	std::cout << "[ERR]" << "[" << typeid(target).name() << "]" << message << std::endl;
}

void Debug::LogException(const std::exception& exception, void* target)
{
	std::cout << "[EXP]" << "[" << typeid(target).name() << "]" << exception.what() << std::endl;
}


