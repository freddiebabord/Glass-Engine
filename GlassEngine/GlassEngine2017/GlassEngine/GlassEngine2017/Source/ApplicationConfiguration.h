#pragma once

namespace GlassEngine
{
	

	class ApplicationSettings
	{
	public:
		/// Initialise the app settings instance.
		/// @note This is usually already implemented as its one of the first things the enigne does itself.
		/// You should not need to call this function
		void Init();

		/// Get the current instance of the application configuration settings
		/// @return A pointer to the application settings instance
		static ApplicationSettings* Instance();

		struct Core
		{
			std::string appNameS;
			const char* appName = "Glass Engine 2017 - VR Technical Test in Vulkan";
			std::string appDeveloperS;
			const char* appDeveloper = "Frederic Babord";
			int appVersionMajor = 1;
			int appVersionMinor = 0;
			int appVersionPatch = 0;
			std::string splashImageS;
			const char* splashScreenImage = "Assets/Core/GE2017Splash.png";
		} core ;

		struct Graphics
		{
			int width = 1280;
			int height = 720;
			float aspectRatio = 16 / 9;
			bool fullscreen = false;
			int msaaSampleCount = 4;
			float resolutionScale = 2.0f;
			bool isVRApplciation = true;
		} graphics;

		struct GlobalCamera
		{
			float nearClipPlane = 0.1f;
			float farClipPlane = 1000.0f;
			float fieldOfView = 60.0f;
		} camera;

		struct Input
		{
			float joystickSensitivity = 20.0f;
		} input;

	private:
		static ApplicationSettings* instance;
	};
}
