#pragma once

namespace GlassEngine
{
	class Debug
	{
	public:
		void Log(const char* message, void* target = nullptr);
		void LogWarning(const char* message, void* target = nullptr);
		void LogError(const char* message, void* target = nullptr);
		void LogException(const std::exception& message, void* target = nullptr);
	};
}