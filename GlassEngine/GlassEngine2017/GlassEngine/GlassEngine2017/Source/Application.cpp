#include "PCH/stdafx.h"
#include "Application.h"
#define STB_IMAGE_IMPLEMENTATION
#include <STB/stb_image.h>
#include "FileReader.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <INIReader.h>

#include "InputManager.h"
#include "VRSystem.h"
#include "AudioManager.h"
#include "Utils.h"

namespace GlassEngine
{

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

	void FlipPixelsVertically(unsigned char *pixels, const size_t width, const size_t height, const size_t bytes_per_pixel)
	{
		const size_t stride = width * bytes_per_pixel;
		unsigned char *row = (unsigned char*)malloc(stride);
		unsigned char *low = pixels;
		unsigned char *high = &pixels[(height - 1) * stride];

		for (; low < high; low += stride, high -= stride) {
			memcpy(row, low, stride);
			memcpy(low, high, stride);
			memcpy(high, row, stride);
		}
		free(row);
	}

	VkBool32 CheckLayers(uint32_t check_count, char const *const *const check_names, uint32_t layer_count, VkLayerProperties *layers) {
		for (uint32_t i = 0; i < check_count; i++) {
			VkBool32 found = VK_FALSE;
			for (uint32_t j = 0; j < layer_count; j++) {
				if (!strcmp(check_names[i], layers[j].layerName)) {
					found = VK_TRUE;
					break;
				}
			}
			if (!found) {
				fprintf(stderr, "Cannot find layer: %s\n", check_names[i]);
				return 0;
			}
		}
		return VK_TRUE;
	}
	
	bool HasDepthStencilComponent(VkFormat format)
	{
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

	bool Application::MemoryTypeFromProperties(uint32_t typeBits, VkMemoryPropertyFlags requirementsMask, uint32_t *typeIndex) const
	{
		// Search memtypes to find first index with those properties
		for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++) {
			if ((typeBits & 1) == 1) {
				// Type is available, does it match user properties?
				if ((physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & requirementsMask) == requirementsMask) {
					*typeIndex = i;
					return true;
				}
			}
			typeBits >>= 1;
		}

		// No memory types matched, return failure
		return false;
	}

	void Application::Cleanup()
	{

		initFinished = false;
		assert(vkDeviceWaitIdle(device) == VK_SUCCESS);

		if(vrSystem->EnableVR())
			vrSystem->Shutdown();

		for(uint32_t i = 0; i < FRAME_LAG; ++i)
		{
			vkWaitForFences(device, 1, &fences[i], VK_TRUE, UINT64_MAX);
			vkDestroyFence(device, fences[i], nullptr);

			vkDestroySemaphore(device, imageAquiredSemaphore[i], nullptr);
			vkDestroySemaphore(device, drawCompleteSemaphore[i], nullptr);

			if(seperatePresentQueue)
				vkDestroySemaphore(device, imageOwnershipSemaphore[i], nullptr);
		}

		vkDestroyFramebuffer(device, framebuffer, nullptr);
		vkDestroyDescriptorPool(device, descriptorPool, nullptr);

		vkDestroyPipeline(device, pipeline, nullptr);
		vkDestroyPipelineCache(device, pipelineCache, nullptr);
		vkDestroyRenderPass(device, renderPass, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

		for (int i = 0; i < sceneLoader.GetTextureCount(); ++i)
		{
			vkDestroyImageView(device, textureImageView[i], nullptr);
			vkDestroyImage(device, textureImage[i], nullptr);
			vkFreeMemory(device, textureImageMemory[i], nullptr);
			vkDestroySampler(device, textureSampler[i], nullptr);
		}

		vkDestroyImageView(device, eyeRenderTarget.view, nullptr);
		vkDestroyImage(device, eyeRenderTarget.image, nullptr);
		vkFreeMemory(device, eyeRenderTarget.memory, nullptr);

		vkDestroyImageView(device, depth.view, nullptr);
		vkDestroyImage(device, depth.image, nullptr);
		vkFreeMemory(device, depth.memory, nullptr);
		
		vkDestroyBuffer(device, uniformData[vr::Eye_Left].buffer, nullptr);
		vkDestroyBuffer(device, uniformData[vr::Eye_Right].buffer, nullptr);
		vkFreeMemory(device, uniformData[vr::Eye_Left].memory, nullptr);
		vkFreeMemory(device, uniformData[vr::Eye_Right].memory, nullptr);

		for (int m = 0; m < meshWithBuffers.size(); ++m)
		{
			for (uint32_t i = 0; i < swapchainImageCount; ++i)
			{
				vkDestroyImageView(device, meshWithBuffers[m][i].view, nullptr);
				vkFreeCommandBuffers(device, commandPool, 1, &meshWithBuffers[m][i].commandBuffer[vr::Eye_Left]);
				vkFreeCommandBuffers(device, commandPool, 1, &meshWithBuffers[m][i].commandBuffer[vr::Eye_Right]);
			}
			vkDestroyBuffer(device, meshes[m].indexBuffer, nullptr);
			vkDestroyBuffer(device, meshes[m].vertexBuffer, nullptr);
			vkFreeMemory(device, meshes[m].indexBufferMemory, nullptr);
			vkFreeMemory(device, meshes[m].vertexBufferMemory, nullptr);
			meshWithBuffers[m].release();
		}

		
		vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);

		vkDestroyCommandPool(device, commandPool, nullptr);

		if (seperatePresentQueue)
			vkDestroyCommandPool(device, presentCommandPool, nullptr);
		
		vkDestroySwapchainKHR(device, swapchain, nullptr);

		vkDeviceWaitIdle(device);

		vkDestroyDevice(device, nullptr);
		vkDestroySurfaceKHR(instance, surface, nullptr);
		DestroyDebugReportCallbackEXT(instance, callback, nullptr);
		vkDestroyInstance(instance, nullptr);

	}

	void Application::Resize()
	{

		if (!initFinished) return;

		initFinished = false;
		assert(vkDeviceWaitIdle(device) == VK_SUCCESS);

		vkDestroyFramebuffer(device, framebuffer, nullptr);
		vkDestroyDescriptorPool(device, descriptorPool, nullptr);

		vkDestroyPipeline(device, pipeline, nullptr);
		vkDestroyPipelineCache(device, pipelineCache, nullptr);
		vkDestroyRenderPass(device, renderPass, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		
		vkDestroyImageView(device, eyeRenderTarget.view, nullptr);
		vkDestroyImage(device, eyeRenderTarget.image, nullptr);
		vkFreeMemory(device, eyeRenderTarget.memory, nullptr);

		vkDestroyImageView(device, depth.view, nullptr);
		vkDestroyImage(device, depth.image, nullptr);
		vkFreeMemory(device, depth.memory, nullptr);

		vkDestroyBuffer(device, uniformData[vr::Eye_Left].buffer, nullptr);
		vkDestroyBuffer(device, uniformData[vr::Eye_Right].buffer, nullptr);
		vkDestroyBuffer(device, uniformData[2].buffer, nullptr);
		vkFreeMemory(device, uniformData[vr::Eye_Left].memory, nullptr);
		vkFreeMemory(device, uniformData[vr::Eye_Right].memory, nullptr);
		vkFreeMemory(device, uniformData[2].memory, nullptr);

		

		for (int m = 0; m < meshWithBuffers.size(); ++m)
		{
			for (uint32_t i = 0; i < swapchainImageCount; ++i)
			{
				vkDestroyImageView(device, meshWithBuffers[m][i].view, nullptr);
				vkFreeCommandBuffers(device, commandPool, 1, &meshWithBuffers[m][i].commandBuffer[vr::Eye_Left]);
				vkFreeCommandBuffers(device, commandPool, 1, &meshWithBuffers[m][i].commandBuffer[vr::Eye_Right]);
				vkFreeCommandBuffers(device, commandPool, 1, &meshWithBuffers[m][i].commandBuffer[2]);
			}
		}
		vkDestroyCommandPool(device, commandPool, nullptr);

		if (seperatePresentQueue)
			vkDestroyCommandPool(device, presentCommandPool, nullptr);

		applicationSettings.graphics.aspectRatio = float(applicationSettings.graphics.width / applicationSettings.graphics.height);

		Prepare();

	}

	VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback)
	{
		auto func = PFN_vkCreateDebugReportCallbackEXT(vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT"));
		if (func != nullptr)
		{
			return func(instance, pCreateInfo, pAllocator, pCallback);
		}
		else
		{
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator)
	{
		auto func = PFN_vkDestroyDebugReportCallbackEXT(vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT"));
		if (func != nullptr)
		{
			func(instance, callback, pAllocator);
		}
	}

	Application::Application():
		window(nullptr), splashscreenWindow(nullptr), splashscreenWindowTexture(0),
		vrSystem(nullptr), audioManager(nullptr), inputManager(nullptr),
		quit(false), initFinished(false), presentMode(),
		frameCount(0), instance(VK_NULL_HANDLE), callback(VK_NULL_HANDLE),
		physicalDevice(VK_NULL_HANDLE), queueFamilyCount(0),
		surface(VK_NULL_HANDLE), device(VK_NULL_HANDLE), graphicsQueue(VK_NULL_HANDLE),
		presentQueue(VK_NULL_HANDLE), format(), colorSpace(), commandPool(VK_NULL_HANDLE),
		commandBuffer(VK_NULL_HANDLE), presentCommandPool(VK_NULL_HANDLE), swapchain(VK_NULL_HANDLE),
		descriptorLayout(VK_NULL_HANDLE), pipelineLayout(VK_NULL_HANDLE),
		pipelineCache(VK_NULL_HANDLE), pipeline(VK_NULL_HANDLE), descriptorPool(VK_NULL_HANDLE),
		renderPass(VK_NULL_HANDLE), framebuffer(VK_NULL_HANDLE), textures(nullptr), textureImage(),
		textureImageMemory(), textureImageView(), textureSampler()
	{
	}

	Application::~Application()
	{
	}

	void Application::Run()
	{
		LoadApplicationConfig();

		InitSplashscreen();

		inputManager = new InputManager;
		audioManager = new AudioManager;
		vrSystem = new Internal::VRSystem;

		// TODO: Dont hardcode this <- needs to go in config file
		if (!sceneLoader.LoadSceneAssets("Assets/Scenes/DemoScene.glassScene"))
			abort();

		InitVk();
		InitWindow();
		InitSwapchain();

		Prepare();
		
		CleanupSplashscreen();


		if(vrSystem->EnableVR())
			vrSystem->UpdateHMDMatrixPose();
		
		sf::Music music;
		
		if (!music.openFromFile(sceneLoader.GetAudioFile(0)))
			return;
		music.setLoop(true);

		music.play();

		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
			for (int i = 0; i < GLFW_JOYSTICK_LAST; ++i)
			{
				if (glfwJoystickPresent(GLFW_JOYSTICK_1 + i) == 1)
				{
					int axisCount;
					const float *axes = glfwGetJoystickAxes(GLFW_JOYSTICK_1 + i, &axisCount);
					if (axisCount >= 6)
					{
						positions[currentMeshToDraw].x += axes[0] * applicationSettings.input.joystickSensitivity;
						positions[currentMeshToDraw].y += axes[1] * applicationSettings.input.joystickSensitivity;
						positions[currentMeshToDraw].z += axes[3] * applicationSettings.input.joystickSensitivity;
						scales[currentMeshToDraw] += glm::vec3(axes[5]) * applicationSettings.input.joystickSensitivity;
						if (scales[currentMeshToDraw].x > 0.15f)
							scales[currentMeshToDraw] -= glm::vec3(axes[4]) * applicationSettings.input.joystickSensitivity;
					}
					else
						printf("GAME :: JOYSTICK AXIS COUNT (%d) INSUFFICIANT\n", axisCount);

					int buttonCount;
					const unsigned char *buttons = glfwGetJoystickButtons(GLFW_JOYSTICK_1 + i, &buttonCount);

					if (GLFW_PRESS == buttons[0]) sendPressedEvent = true;
					if (GLFW_RELEASE == buttons[0] && sendPressedEvent)
					{
						currentMeshToDraw = currentMeshToDraw == 0 ? 1 : 0;
						sendPressedEvent = false;
					}

				}
			}

			UpdateUniformData();
			Draw();
			currentFrame++;
		}

		vkDeviceWaitIdle(device);

		glfwDestroyWindow(window);

		glfwTerminate();

		Cleanup();

		delete inputManager;
		delete audioManager;
		delete vrSystem;
	}

	void Application::Draw()
	{

		vkWaitForFences(device, 1, &fences[frameIndex], VK_TRUE, UINT64_MAX);
		vkResetFences(device, 1, &fences[frameIndex]);


		VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAquiredSemaphore[frameIndex], fences[frameIndex], &currentBuffer);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			Resize();
			Draw();
			return;
		}
		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("Failed to acquire swap chain image!");
		}


		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &imageAquiredSemaphore[frameIndex];
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &meshWithBuffers[currentMeshToDraw][currentBuffer].commandBuffer[vr::Eye_Left];
		submitInfo.signalSemaphoreCount = 0;
		submitInfo.pSignalSemaphores = nullptr;

		if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VkFence()) != VK_SUCCESS) {
			throw std::runtime_error("Failed to submit draw command buffer!");
		}

		vr::VRTextureBounds_t textureBounds;
		vr::VRVulkanTextureData_t vulkanData;
		vr::Texture_t texture;

		if (vrSystem->EnableVR())
		{
			//-----------------------------------------------------------------------------------------
			// OpenVR BEGIN: Submit eyes to compositor, left eye just rendered
			//-----------------------------------------------------------------------------------------
			
			textureBounds.uMin = 0.0f;
			textureBounds.uMax = 1.0f;
			textureBounds.vMin = 0.0f;
			textureBounds.vMax = 1.0f;
			
			vulkanData.m_nImage = (uint64_t)(VkImage)eyeRenderTarget.image;
			vulkanData.m_pDevice = (VkDevice_T *)device;
			vulkanData.m_pPhysicalDevice = (VkPhysicalDevice_T *)physicalDevice;
			vulkanData.m_pInstance = (VkInstance_T *)instance;
			vulkanData.m_pQueue = (VkQueue_T *)graphicsQueue;
			vulkanData.m_nQueueFamilyIndex = graphicsQueueFamilyIndex;

			vulkanData.m_nWidth = eyeRenderTarget.textureWidth;
			vulkanData.m_nHeight = eyeRenderTarget.textureHeight;
			vulkanData.m_nFormat = (uint32_t)format;
			vulkanData.m_nSampleCount = uint32_t(applicationSettings.graphics.msaaSampleCount);
			
			texture = { &vulkanData, vr::TextureType_Vulkan, vr::ColorSpace_Auto };
			auto sub = vr::VRCompositor()->Submit(vr::Eye_Left, &texture, &textureBounds);
			assert(sub == vr::VRCompositorError_None);
		}


		// Submit right eye
		VkSubmitInfo rightSubmitInfo = {};
		rightSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		rightSubmitInfo.waitSemaphoreCount = 0;
		rightSubmitInfo.pWaitSemaphores = nullptr;
		rightSubmitInfo.pWaitDstStageMask = waitStages;
		rightSubmitInfo.commandBufferCount = 1;
		rightSubmitInfo.pCommandBuffers = &meshWithBuffers[currentMeshToDraw][currentBuffer].commandBuffer[vr::Eye_Right];
		rightSubmitInfo.signalSemaphoreCount = 0;
		rightSubmitInfo.pSignalSemaphores = nullptr;

		result = vkQueueSubmit(graphicsQueue, 1, &rightSubmitInfo, VkFence());
		if (result != VK_SUCCESS) {
			throw std::runtime_error("Failed to submit draw command buffer!");
		}

		if (vrSystem->EnableVR())
		{
			auto sub = vr::VRCompositor()->Submit(vr::Eye_Right, &texture, &textureBounds);
			assert(sub == vr::VRCompositorError_None);
		}


		VkSubmitInfo viewportSubmitInfo = {};
		viewportSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		viewportSubmitInfo.waitSemaphoreCount = 0;
		viewportSubmitInfo.pWaitSemaphores = nullptr;
		viewportSubmitInfo.pWaitDstStageMask = waitStages;
		viewportSubmitInfo.commandBufferCount = 1;
		viewportSubmitInfo.pCommandBuffers = &meshWithBuffers[currentMeshToDraw][currentBuffer].commandBuffer[1];
		viewportSubmitInfo.signalSemaphoreCount = 1;
		viewportSubmitInfo.pSignalSemaphores = &drawCompleteSemaphore[frameIndex];

		result = vkQueueSubmit(graphicsQueue, 1, &viewportSubmitInfo, VkFence());
		if (result != VK_SUCCESS) {
			throw std::runtime_error("Failed to submit draw command buffer!");
		}

		if (seperatePresentQueue)
		{
			VkSubmitInfo presentSubmitInfo = {};
			presentSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			presentSubmitInfo.pWaitDstStageMask = waitStages;
			presentSubmitInfo.waitSemaphoreCount = 1;
			presentSubmitInfo.pWaitSemaphores = &drawCompleteSemaphore[frameIndex];
			presentSubmitInfo.commandBufferCount = 1;
			presentSubmitInfo.pCommandBuffers = &meshWithBuffers[currentMeshToDraw][currentBuffer].graphicsToPresentCommand;
			presentSubmitInfo.signalSemaphoreCount = 1;
			presentSubmitInfo.pSignalSemaphores = &imageOwnershipSemaphore[frameIndex];

			assert(vkQueueSubmit(presentQueue, 1, &presentSubmitInfo, VkFence()) == VK_SUCCESS);
		}



		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = seperatePresentQueue ? &imageOwnershipSemaphore[frameIndex] : &drawCompleteSemaphore[frameIndex];
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapchain;
		presentInfo.pImageIndices = &currentBuffer;
		presentInfo.pResults = nullptr;


		result = vkQueuePresentKHR(presentQueue, &presentInfo);
		frameIndex++;
		frameIndex %= FRAME_LAG;
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
			Resize();
		else if (result != VK_SUCCESS)
			throw std::runtime_error("failed to present swap chain image!");

		if(vrSystem->EnableVR())
			vrSystem->UpdateHMDMatrixPose();

	}

	glm::vec3 GetForwardVector(glm::mat4 matrix_)
	{
		return glm::normalize(glm::vec3(matrix_[0][2], matrix_[1][2], matrix_[2][2]));
	}

	void Application::UpdateUniformData()
	{
		uniformBufferObject.model = glm::scale(glm::translate(glm::rotate(glm::mat4(1.0), glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0)), positions[currentMeshToDraw]), scales[currentMeshToDraw]);

		uniformBufferObject.currentMesh = currentMeshToDraw;
		uniformBufferObject.eyePosition = glm::vec3(vrSystem->GetHMDPose()[3]);

		for (int32_t nEye = 0; nEye < 2; nEye++)
		{
			
			uniformBufferObject.viewProjection = vrSystem->GetCurrentViewProjectionMatrix((vr::Hmd_Eye) nEye);
						

			void* data;
			vkMapMemory(device, uniformData[nEye].memory, 0, sizeof(uniformBufferObject), 0, &data);
			memcpy(data, &uniformBufferObject, sizeof(uniformBufferObject));
			vkUnmapMemory(device, uniformData[nEye].memory);
		}

		{
		
			projectionMatrix = glm::perspective(glm::radians(applicationSettings.camera.fieldOfView), applicationSettings.graphics.aspectRatio, applicationSettings.camera.nearClipPlane, applicationSettings.camera.farClipPlane);

			// NOTE: Flip y for Vulkan and scale z fopr clip space
			glm::mat4 vkCorrectedProjectionMatrix =  glm::mat4x4(
				projectionMatrix[0][0],  projectionMatrix[1][0], projectionMatrix[2][0], projectionMatrix[3][0],
				projectionMatrix[0][1], -projectionMatrix[1][1], projectionMatrix[2][1], projectionMatrix[3][1],
				projectionMatrix[0][2],  projectionMatrix[1][2], projectionMatrix[2][2] / 2, projectionMatrix[3][2],
				projectionMatrix[0][3],  projectionMatrix[1][3], projectionMatrix[2][3] / 2, projectionMatrix[3][3]
			);

			uniformBufferObject.viewProjection = vkCorrectedProjectionMatrix * vrSystem->GetCurrentViewMatrix((vr::Hmd_Eye)0);

			void* data;
			vkMapMemory(device, uniformData[2].memory, 0, sizeof(uniformBufferObject), 0, &data);
			memcpy(data, &uniformBufferObject, sizeof(uniformBufferObject));
			vkUnmapMemory(device, uniformData[2].memory);
		}
		
	}

	void Application::LoadApplicationConfig()
	{
		INIReader reader(configurationFile);

		if (reader.ParseError() < 0) {
			std::cout << "Can't load configuration file\n";
			abort();
		}

		applicationSettings.Init();

		applicationSettings.core.appNameS = reader.Get("Core", "applicationName", "UNKNOWN");
		applicationSettings.core.appName = applicationSettings.core.appNameS.c_str();
		applicationSettings.core.appDeveloperS = reader.Get("Core", "applicationDeveloper", "UNKNOWN");
		applicationSettings.core.appDeveloper = applicationSettings.core.appDeveloperS.c_str();
		auto ss = Split(reader.Get("Core", "applicationVersion", "UNKNOWN"), '.');
		applicationSettings.core.appVersionMajor = atoi(ss[0].c_str());
		applicationSettings.core.appVersionMinor = atoi(ss[1].c_str());
		applicationSettings.core.appVersionPatch = atoi(ss[2].c_str());
		applicationSettings.core.splashImageS = reader.Get("Core", "splashScreenImage", "UNKNOWN");
		applicationSettings.core.splashScreenImage = applicationSettings.core.splashImageS.c_str();

		applicationSettings.graphics.width = reader.GetInteger("Graphics", "width", 100);
		applicationSettings.graphics.height = reader.GetInteger("Graphics", "height", 100);
		applicationSettings.graphics.fullscreen = reader.GetBoolean("Graphcis", "fullscreen", false);
		applicationSettings.graphics.msaaSampleCount = reader.GetInteger("Graphics", "msaaSampleCount", 1);
		applicationSettings.graphics.resolutionScale = float(reader.GetReal("Graphics", "resolutionScale", 1));
		applicationSettings.graphics.isVRApplciation = reader.GetBoolean("Graphics", "isVRApplication", false);

		applicationSettings.camera.nearClipPlane = float(reader.GetReal("GlobalCamera", "nearClipPlane", 1));
		applicationSettings.camera.farClipPlane = float(reader.GetReal("GlobalCamera", "farClipPlane", 100));
		applicationSettings.camera.fieldOfView = float(reader.GetReal("GlobalCamera", "fieldOfView", 10));

		applicationSettings.input.joystickSensitivity = float(reader.GetReal("Input", "joystickSensitivity", 10));

	}

	void Application::InitSplashscreen()
	{
		glfwInit();

		int texWidth, texHeight, texChannels;

		stbi_uc* pixels = stbi_load(applicationSettings.core.splashScreenImage, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		FlipPixelsVertically(pixels, texWidth, texHeight, 4);
		glfwWindowHint(GLFW_SAMPLES, 0); // 4x antialiasing
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // We want OpenGL 3.3
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); //We don't want the old OpenGL 

		glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		splashscreenWindow = glfwCreateWindow(texWidth, texHeight, "Glass Engine 2017", NULL, window);

		HICON hIcon = (HICON)LoadImage(NULL, "Assets/Core/GlassEngineIcon.ico", IMAGE_ICON, 32, 32, LR_LOADFROMFILE);
		SendMessage(glfwGetWin32Window(splashscreenWindow), WM_SETICON, ICON_BIG, (LPARAM)hIcon);

		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);
		glfwSetWindowPos(splashscreenWindow, (mode->width - texWidth) / 2, (mode->height - texHeight) / 2);
		
		glfwMakeContextCurrent(splashscreenWindow); // Initialize GLEW
		glewExperimental = true; // Needed in core profile
		if (glewInit() != GLEW_OK) {
			fprintf(stderr, "Failed to initialize GLEW\n");
		}
		
		
		
		glGenTextures(1, &splashscreenWindowTexture);


		// "Bind" the newly created texture : all future texture functions will modify this texture
		glBindTexture(GL_TEXTURE_2D, splashscreenWindowTexture);

		// Give the image to OpenGL
		if (texChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth, texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
		else if (texChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texWidth, texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		stbi_image_free(pixels);

		glEnable(GL_TEXTURE_2D);

		
		glGenFramebuffers(1, &splashscreenFramebuffer);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, splashscreenFramebuffer);
		glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, splashscreenWindowTexture, 0);

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);  // if not already bound
		glBlitFramebuffer(0, 0, texWidth, texHeight, 0, 0, texWidth, texHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		
		glfwSwapBuffers(splashscreenWindow);

		glBlitFramebuffer(0, 0, texWidth, texHeight, 0, 0, texWidth, texHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);

	}

	void Application::CleanupSplashscreen()
	{
		glDeleteTextures(1, &splashscreenWindowTexture);
		glDeleteFramebuffers(1, &splashscreenFramebuffer);
		glfwDestroyWindow(splashscreenWindow);
		splashscreenWindow = nullptr;
		glfwShowWindow(window);
	}

	void Application::InitVk()
	{
		presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
		frameCount = UINT32_MAX;

		{
			uint32_t instanceExtensionCount = 0;
			uint32_t instanceLayerCount = 0;
			uint32_t validationLayerCount;
			char const *const *instanceValidationLayers;
			enabledExtensionCount = 0;
			enabledLayerCount = 0;

			std::vector<std::string> openvrInstanceExtensions;

			if (applicationSettings.graphics.isVRApplciation && vrSystem->VRSystemAvailable())
			{
				vrSystem->Init();

				if (!vrSystem->GetVulkanInstanceExtensionsRequired(openvrInstanceExtensions))
					throw std::exception("VR: Could not determin OpenVR Vulkan instance extensions");
			}

			std::vector<const char*> extensions;

			// Get required extensions
			{
				unsigned int glfwExtensionCount = 0;
				const char** glfwExtensions;
				glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

				for (unsigned int i = 0; i < glfwExtensionCount; i++) {
					extensions.push_back(glfwExtensions[i]);
				}
				for (unsigned int i = 0; i < openvrInstanceExtensions.size(); i++) {
					extensions.push_back(openvrInstanceExtensions[i].c_str());
				}
				//validate = false;
				if (validate) {
					extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
					char const* const instanceValidationLayersAlt1[] = {
						"VK_LAYER_KHRONOS_validation" };

					VkBool32 validationFound = VK_FALSE;

					assert(vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr) == VK_SUCCESS);
					instanceValidationLayers = instanceValidationLayersAlt1;
					if(instanceLayerCount > 0)
					{
						std::unique_ptr<VkLayerProperties[]> instanceLayers(new VkLayerProperties[instanceLayerCount]);
						assert(vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayers.get()) == VK_SUCCESS);

						validationFound = CheckLayers(ARRAY_SIZE(instanceValidationLayersAlt1), instanceValidationLayers, instanceLayerCount, instanceLayers.get());
						if(validationFound)
						{
							instanceValidationLayers = instanceValidationLayersAlt1;
							enabledLayerCount = ARRAY_SIZE(instanceValidationLayersAlt1);
							validationFound = CheckLayers(ARRAY_SIZE(instanceValidationLayersAlt1), instanceValidationLayers, instanceLayerCount, instanceLayers.get());
							validationLayerCount = ARRAY_SIZE(instanceValidationLayersAlt1);
							for (uint32_t i = 0; i < validationLayerCount; ++i)
								enabledLayers[i] = instanceValidationLayers[i];
						}

					}

					if(!validationFound)
						throw std::exception("VK: Failed to find required validation layers");
				}
			}

			// Look for instance extensions
			/*{
				VkBool32 surfaceFound = VK_FALSE;
				VkBool32 platformSurfaceFound = VK_FALSE;
				

				memset(extensionNames, 0, sizeof(extensionNames));
				

				assert(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr) == VK_SUCCESS);

				if(instanceExtensionCount > 0)
				{
					std::unique_ptr<VkExtensionProperties[]> instanceExtensions(new VkExtensionProperties[instanceExtensionCount]);
					assert(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, instanceExtensions.get()) == VK_SUCCESS);

					for(uint32_t i = 0; i < instanceExtensionCount; ++i)
					{
						if (!strcmp(VK_KHR_SURFACE_EXTENSION_NAME, instanceExtensions[i].extensionName)) {
							surfaceFound = 1;
							extensionNames[enabledExtensionCount++] = VK_KHR_SURFACE_EXTENSION_NAME;
						}
						if (validate && !strcmp(VK_EXT_DEBUG_REPORT_EXTENSION_NAME, instanceExtensions[i].extensionName)) {
							extensionNames[enabledExtensionCount++] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
							debugReport = true;
						}
						if (!strcmp(VK_KHR_WIN32_SURFACE_EXTENSION_NAME, instanceExtensions[i].extensionName)) {
							platformSurfaceFound = 1;
							extensionNames[enabledExtensionCount++] = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
						}

						assert(enabledExtensionCount < 64);
					}
				}

				{
					for (size_t vrExtID = 0; vrExtID < openvrInstanceExtensions.size(); ++vrExtID)
						extensionNames[enabledExtensionCount++] = openvrInstanceExtensions[vrExtID].c_str();
				}

				if(!surfaceFound || !platformSurfaceFound)
					throw std::exception("VK: Failed to find Vulkan surface formats");

			}*/

			// Create app instance and debug callbacks
			{
				
				VkApplicationInfo appInfo = {};
				appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
				appInfo.pApplicationName = applicationSettings.Instance()->core.appName;
				appInfo.applicationVersion = VK_MAKE_VERSION(applicationSettings.Instance()->core.appVersionMajor, applicationSettings.Instance()->core.appVersionMinor, applicationSettings.Instance()->core.appVersionPatch);
				appInfo.pEngineName = "Glass Engine";
				appInfo.engineVersion = VK_MAKE_VERSION(3, 0, 0);
				appInfo.apiVersion = VK_API_VERSION_1_3;

				VkInstanceCreateInfo instanceCreateInfo = {};
				instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
				instanceCreateInfo.pApplicationInfo = &appInfo;

				instanceCreateInfo.enabledExtensionCount = extensions.size();
				instanceCreateInfo.ppEnabledExtensionNames = extensions.data();
				instanceCreateInfo.enabledLayerCount = enabledLayerCount;
				instanceCreateInfo.ppEnabledLayerNames = enabledLayers;
				
				assert(vkCreateInstance(&instanceCreateInfo, nullptr, &instance) == VK_SUCCESS);

				#ifdef _DEBUG
					debugReport = true;
				#endif
				if(debugReport)
				{
					VkDebugReportCallbackCreateInfoEXT createInfo = {};
					createInfo.pNext = nullptr;
					createInfo.pUserData = nullptr;
					createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
					createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT |
#ifdef DEBUG_VERBOSE
						VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
						VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
						VK_DEBUG_REPORT_DEBUG_BIT_EXT |
#endif
						VK_DEBUG_REPORT_WARNING_BIT_EXT;
					createInfo.pfnCallback = DEBUG_CALLBACK;
					assert (CreateDebugReportCallbackEXT(instance, &createInfo, nullptr, &callback) == VK_SUCCESS);
				}
			}

			// Find suitable physical device
			{
				
				uint32_t gpuCount;
				assert(vkEnumeratePhysicalDevices(instance, &gpuCount, nullptr) == VK_SUCCESS);
				assert(gpuCount > 0);

				std::unique_ptr<VkPhysicalDevice[]> physicalDevices(new VkPhysicalDevice[gpuCount]);
				assert(vkEnumeratePhysicalDevices(instance, &gpuCount, physicalDevices.get()) == VK_SUCCESS);

				physicalDevice = physicalDevices[0];

			}

			// Find device extensions
			{
				uint32_t deviceExtensionCount = 0;
				VkBool32 swapchainFound = VK_FALSE;

				enabledExtensionCount = 0;
				memset(extensionNames, 0, sizeof(extensionNames));

				assert(vkEnumerateDeviceExtensionProperties((physicalDevice), nullptr, &deviceExtensionCount, nullptr) == VK_SUCCESS);

				if(deviceExtensionCount > 0)
				{
					std::unique_ptr<VkExtensionProperties[]> deviceExtensions(new VkExtensionProperties[deviceExtensionCount]);
					assert(vkEnumerateDeviceExtensionProperties((physicalDevice), nullptr, &deviceExtensionCount, deviceExtensions.get()) == VK_SUCCESS);

					for(uint32_t i = 0; i < deviceExtensionCount; ++i)
					{
						if (!strcmp(VK_KHR_SWAPCHAIN_EXTENSION_NAME, deviceExtensions[i].extensionName)) {
							swapchainFound = VK_TRUE;
							extensionNames[enabledExtensionCount++] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
						}
						assert(enabledExtensionCount < 64);
					}
				}

				if (!swapchainFound)
					throw std::exception("VK:: failed to find swapchain extension");

				std::vector<std::string> openvrDeviceExtensions;
				if (vrSystem->EnableVR())
				{
					if (!vrSystem->GetVulkanDeviceExtensionsRequired((physicalDevice), openvrDeviceExtensions))
						throw std::exception("Unable to find openvr extension");

					for (size_t nExt = 0; nExt < openvrDeviceExtensions.size(); nExt++)
					{
						char *pExtString = new char[openvrDeviceExtensions[nExt].length() + 1];
						strcpy(pExtString, openvrDeviceExtensions[nExt].c_str());
						extensionNames[enabledExtensionCount++] = pExtString;
					}
				}
				vkGetPhysicalDeviceProperties((physicalDevice), &physicalDeviceProperties);
				vkGetPhysicalDeviceQueueFamilyProperties((physicalDevice), &queueFamilyCount, nullptr);
				assert(queueFamilyCount >= 1);
				queueProperties.reset(new VkQueueFamilyProperties[queueFamilyCount]);

				vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueProperties.get());

				vkGetPhysicalDeviceFeatures((physicalDevice), &physicalDeviceFeatures);
			}

		}

		
		projectionMatrix = glm::perspective(glm::radians(applicationSettings.camera.fieldOfView), applicationSettings.graphics.aspectRatio, applicationSettings.camera.nearClipPlane, applicationSettings.camera.farClipPlane);
		viewMatrix = glm::lookAt(eye, origin, up);
		modelMatrix = glm::mat4(1.0);
		projectionMatrix[1][1] *= -1;
	}

	void Application::InitWindow()
	{
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
		glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

		window = glfwCreateWindow(applicationSettings.graphics.width, applicationSettings.graphics.height, applicationSettings.Instance()->core.appName, nullptr, nullptr);
		
		HICON hIcon = (HICON)LoadImage(NULL, "Assets/Core/GlassEngineIcon.ico", IMAGE_ICON, 32, 32, LR_LOADFROMFILE);
		SendMessage(glfwGetWin32Window(window), WM_SETICON, ICON_BIG, (LPARAM)hIcon);
		
		glfwHideWindow(window);

		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);
		glfwSetWindowPos(window, (mode->width - applicationSettings.graphics.width) / 2, (mode->height - applicationSettings.graphics.height) / 2);

		glfwSetWindowUserPointer(window, this);
		glfwSetWindowSizeCallback(window, OnWindowResized);
		glfwSetKeyCallback(window, KeyInputCallback);
		
	}

	void Application::InitSwapchain()
	{
		// Create Vulkan surface
		{
			if (glfwCreateWindowSurface((instance), window, nullptr, &surface) != VK_SUCCESS) {
				throw std::runtime_error("failed to create window surface!");
			}
		}

		// Get graphics and presentqueues
		{

			std::unique_ptr<VkBool32[]> supportsPresent(new VkBool32[queueFamilyCount]);
			for (uint32_t i = 0; i < queueFamilyCount; ++i)
				vkGetPhysicalDeviceSurfaceSupportKHR((physicalDevice), i, (surface), &supportsPresent[i]);

			graphicsQueueFamilyIndex = UINT32_MAX;
			presentQueueFamilyIndex = UINT32_MAX;

			for(uint32_t i = 0; i < queueFamilyCount; ++i)
			{
				if (queueProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
				{
					if (graphicsQueueFamilyIndex == UINT32_MAX) {
						graphicsQueueFamilyIndex = i;
					}

					if (supportsPresent[i] == VK_TRUE) {
						graphicsQueueFamilyIndex = i;
						presentQueueFamilyIndex = i;
						break;
					}
				}
			}

			if (presentQueueFamilyIndex == UINT32_MAX) {
				for (uint32_t i = 0; i < queueFamilyCount; ++i) {
					if (supportsPresent[i] == VK_TRUE) {
						presentQueueFamilyIndex = i;
						break;
					}
				}
			}

			if (graphicsQueueFamilyIndex == UINT32_MAX || presentQueueFamilyIndex == UINT32_MAX) {
				throw std::exception("Could not find both graphics and present queues");
			}

			seperatePresentQueue = graphicsQueueFamilyIndex != presentQueueFamilyIndex;

		}

		// Create logical device
		{
			const float priorities[1] = { 0.0 };

			VkDeviceQueueCreateInfo queues[2];
			for (uint32_t i = 0; i < 2; ++i)
			{
				queues[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queues[i].pNext = nullptr;
				queues[i].flags = 0;
			}
			
			queues[0].queueFamilyIndex = graphicsQueueFamilyIndex;
			queues[0].queueCount = 1;
			queues[0].pQueuePriorities = priorities;

			if(seperatePresentQueue)
			{
				queues[1].queueFamilyIndex = presentQueueFamilyIndex;
				queues[1].queueCount = 1;
				queues[1].pQueuePriorities = priorities;
			}

			VkDeviceCreateInfo deviceCreate = {};
			deviceCreate.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			deviceCreate.pNext = nullptr;
			deviceCreate.enabledExtensionCount = enabledExtensionCount;
			deviceCreate.ppEnabledExtensionNames = static_cast<const char *const *>(extensionNames);
			deviceCreate.enabledLayerCount = 0;
			deviceCreate.ppEnabledLayerNames = nullptr;
			deviceCreate.pEnabledFeatures = nullptr;
			deviceCreate.queueCreateInfoCount = seperatePresentQueue ? 2 : 1;
			deviceCreate.pQueueCreateInfos = queues;

			assert(vkCreateDevice(physicalDevice, &deviceCreate, nullptr, &device) == VK_SUCCESS);

		}

		// Get queues
		{
			
			vkGetDeviceQueue((device), graphicsQueueFamilyIndex, 0, &graphicsQueue);
			if (seperatePresentQueue)
				presentQueue = graphicsQueue;
			else
				vkGetDeviceQueue((device), presentQueueFamilyIndex, 0, &presentQueue);

		}

		// Get surface format
		{
			
			uint32_t formatCount;
			assert(vkGetPhysicalDeviceSurfaceFormatsKHR((physicalDevice), (surface), &formatCount, nullptr) == VK_SUCCESS);
			
			std::unique_ptr<VkSurfaceFormatKHR[]> surfaceFormats(new VkSurfaceFormatKHR[formatCount]);
			assert(vkGetPhysicalDeviceSurfaceFormatsKHR((physicalDevice), (surface), &formatCount, surfaceFormats.get()) == VK_SUCCESS);

			uint32_t formatIndex = 0;
			if (formatCount == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED) {
				format = VK_FORMAT_R8G8B8A8_UNORM;
			}
			else {
				assert(formatCount >= 1);
				// Favor sRGB if it's available
				for (; formatIndex < formatCount; formatIndex++)
				{
					if (surfaceFormats[formatIndex].format == VK_FORMAT_B8G8R8A8_SRGB ||
						surfaceFormats[formatIndex].format == VK_FORMAT_R8G8B8A8_SRGB)
					{
						break;
					}
				}
				if (formatIndex == formatCount)
				{
					// Default to the first one if no sRGB
					formatIndex = 0;
				}

				format = surfaceFormats[formatIndex].format;
			}
			colorSpace = surfaceFormats[formatIndex].colorSpace;

			quit = false;
			currentFrame = 0;

		}

		// Create semaphores
		{
			
			VkSemaphoreCreateInfo semaphoreCreateInfo = {};
			semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			semaphoreCreateInfo.pNext = nullptr;
			
			VkFenceCreateInfo fenceCreateInfo = {};
			fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceCreateInfo.pNext = nullptr;
			fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

			for (uint32_t i = 0; i < FRAME_LAG; i++) 
			{
				assert(vkCreateFence(device, &fenceCreateInfo, nullptr, &fences[i]) == VK_SUCCESS);

				assert(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &imageAquiredSemaphore[i]) == VK_SUCCESS);
				
				assert(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &drawCompleteSemaphore[i]) == VK_SUCCESS);

				if (seperatePresentQueue)
					assert(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &imageOwnershipSemaphore[i]) == VK_SUCCESS);
			}

			frameIndex = 0;
			vkGetPhysicalDeviceMemoryProperties((physicalDevice), &physicalDeviceMemoryProperties);

		}

	}

	void Application::Prepare()
	{
		
		VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
		
		// Create command buffer pool, command buffer and begin command recording
		{

			VkCommandPoolCreateInfo commandPoolCreateInfo = {};
			commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			commandPoolCreateInfo.pNext = nullptr;
			commandPoolCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex;

			assert(vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool) == VK_SUCCESS);
						
			commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			commandBufferAllocateInfo.pNext = nullptr;
			commandBufferAllocateInfo.commandPool = commandPool;
			commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			commandBufferAllocateInfo.commandBufferCount = 1;

			assert(vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBuffer) == VK_SUCCESS);

			VkCommandBufferBeginInfo commandBeginInfo = {};
			commandBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			commandBeginInfo.pNext = nullptr;
			commandBeginInfo.pInheritanceInfo = nullptr;

			assert(vkBeginCommandBuffer(commandBuffer, &commandBeginInfo) == VK_SUCCESS);

		}

		{
			
			PrepareOpenVR();
			PrepareBuffers();
			PrepareDepth();
			//PrepareTexture();
			CreateTextureImage();
			CreateTextureImageView();
			CreateTextureSampler();
			LoadModel();
			PrepareDescriptorLayouts();
			PrepareRenderPass();
			PreparePipeline();

		}

		{
			
			for(uint32_t i = 0; i < swapchainImageCount; ++i)
			{
				for (int m = 0; m < meshWithBuffers.size(); ++m)
				{
					assert(vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &meshWithBuffers[m][i].commandBuffer[vr::Eye_Left]) == VK_SUCCESS);
					assert(vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &meshWithBuffers[m][i].commandBuffer[vr::Eye_Right]) == VK_SUCCESS);
					assert(vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &meshWithBuffers[m][i].commandBuffer[2]) == VK_SUCCESS);
				}
			}

			if(seperatePresentQueue)
			{
				VkCommandPoolCreateInfo presentCommandPoolCreateInfo = {};
				presentCommandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
				presentCommandPoolCreateInfo.pNext = nullptr;
				presentCommandPoolCreateInfo.queueFamilyIndex = presentQueueFamilyIndex;

				assert(vkCreateCommandPool((device), &presentCommandPoolCreateInfo, nullptr, &presentCommandPool) == VK_SUCCESS);

				VkCommandBufferAllocateInfo presentCommand = {};
				presentCommand.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				presentCommand.pNext = nullptr;
				presentCommand.commandPool = (presentCommandPool);
				presentCommand.commandBufferCount = 1;
				presentCommand.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

				for(uint32_t i = 0; i < swapchainImageCount; ++i)
				{
					for (int m = 0; m < meshWithBuffers.size(); ++m)
					{
						assert(vkAllocateCommandBuffers((device), &presentCommand, &meshWithBuffers[m][i].graphicsToPresentCommand) == VK_SUCCESS);

						BuildImageOwnershipCmd(i, m);
					}
				}
				
			}

		}

		{
			
			PrepareDescriptorPool();
			PrepareDescriptorSet();
			PrepareFramebuffers();

		}

		{
			
			for(uint32_t i = 0; i < swapchainImageCount; ++i)
			{
				currentBuffer = i;
				for(int m = 0; m < meshWithBuffers.size(); ++m)
				{
					DrawBuildCmd(meshWithBuffers[m][i].commandBuffer[vr::Eye_Left], vr::Eye_Left, m);
					DrawBuildCmd(meshWithBuffers[m][i].commandBuffer[vr::Eye_Right], vr::Eye_Right, m);
					BuildDrawToViewportCmd(meshWithBuffers[m][i].commandBuffer[2], m);
				}
			}

			FlushInitCommand();

			currentBuffer = 0;

		}

		initFinished = true;
	}

	void Application::PrepareOpenVR()
	{
		if (vrSystem->EnableVR())
		{
			eyeRenderTarget.textureWidth = int32_t(vrSystem->GetRenderWidth() * applicationSettings.graphics.resolutionScale);
			eyeRenderTarget.textureHeight = int32_t(vrSystem->GetRenderHeight() * applicationSettings.graphics.resolutionScale);
		}
		else
		{
			eyeRenderTarget.textureWidth = int32_t(applicationSettings.graphics.width * applicationSettings.graphics.resolutionScale);
			eyeRenderTarget.textureHeight = int32_t(applicationSettings.graphics.height * applicationSettings.graphics.resolutionScale);
		}

		VkImageCreateInfo imageCreateInfo = {};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.pNext = nullptr;
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format = format;
		imageCreateInfo.extent = { uint32_t(eyeRenderTarget.textureWidth), uint32_t(eyeRenderTarget.textureHeight), 1 };
		imageCreateInfo.mipLevels = 1;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.samples = VkSampleCountFlagBits(uint32_t(applicationSettings.graphics.msaaSampleCount));
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.queueFamilyIndexCount = 0;
		imageCreateInfo.pQueueFamilyIndices = nullptr;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		assert(vkCreateImage(device, &imageCreateInfo, nullptr, &eyeRenderTarget.image) == VK_SUCCESS);

		VkMemoryRequirements memoryRequirements = {};
		vkGetImageMemoryRequirements(device, eyeRenderTarget.image, &memoryRequirements);

		eyeRenderTarget.memoryAllocation.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		eyeRenderTarget.memoryAllocation.pNext = nullptr;
		eyeRenderTarget.memoryAllocation.allocationSize = memoryRequirements.size;
		eyeRenderTarget.memoryAllocation.memoryTypeIndex = 0;

		assert(MemoryTypeFromProperties(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &eyeRenderTarget.memoryAllocation.memoryTypeIndex) == true);

		assert(vkAllocateMemory(device, &eyeRenderTarget.memoryAllocation, nullptr, &eyeRenderTarget.memory) == VK_SUCCESS);
		assert(vkBindImageMemory(device, eyeRenderTarget.image, eyeRenderTarget.memory, 0) == VK_SUCCESS);

		eyeRenderTarget.layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		
		SetImageLayout(commandBuffer, eyeRenderTarget.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);

		VkImageViewCreateInfo colorImageView = {};
		colorImageView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		colorImageView.pNext = nullptr;
		colorImageView.image = eyeRenderTarget.image;
		colorImageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
		colorImageView.format = format;
		colorImageView.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

		assert(vkCreateImageView(device, &colorImageView, nullptr, &eyeRenderTarget.view) == VK_SUCCESS);

	}

	void Application::PrepareBuffers()
	{

		VkSwapchainKHR oldSwapchain = hasSwapchain ? swapchain : nullptr;
		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		assert(vkGetPhysicalDeviceSurfaceCapabilitiesKHR((physicalDevice), (surface), &surfaceCapabilities) == VK_SUCCESS);

		uint32_t presentModeCount;
		assert(vkGetPhysicalDeviceSurfacePresentModesKHR((physicalDevice), (surface), &presentModeCount, nullptr) == VK_SUCCESS);

		std::unique_ptr<VkPresentModeKHR[]> presentModes(new VkPresentModeKHR[presentModeCount]);
		assert(vkGetPhysicalDeviceSurfacePresentModesKHR((physicalDevice), (surface), &presentModeCount, presentModes.get()) == VK_SUCCESS);

		VkExtent2D swapchainExtent;
		// width and height are either both -1, or both not -1.
		if (surfaceCapabilities.currentExtent.width == (uint32_t)-1) {
			// If the surface size is undefined, the size is set to
			// the size of the images requested.
			swapchainExtent.width = applicationSettings.graphics.width;
			swapchainExtent.height = applicationSettings.graphics.height;
		}
		else {
			// If the surface size is defined, the swap chain size must match
			swapchainExtent = surfaceCapabilities.currentExtent;
			applicationSettings.graphics.width = surfaceCapabilities.currentExtent.width;
			applicationSettings.graphics.height = surfaceCapabilities.currentExtent.height;
		}

		VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
		if(presentMode != swapchainPresentMode)
		{
			for(size_t i = 0; i < presentModeCount; ++i)
			{
				if(presentModes[i] == presentMode)
				{
					swapchainPresentMode = presentMode;
					break;
				}
			}
		}

		if (swapchainPresentMode != presentMode)
			throw std::exception("VK :: Specified present mode not specified\n");

		uint32_t desiredNumberOfSwapchainImages = surfaceCapabilities.minImageCount + 1;
		if ((surfaceCapabilities.maxImageCount > 0) && (desiredNumberOfSwapchainImages > surfaceCapabilities.maxImageCount)) 
			desiredNumberOfSwapchainImages = surfaceCapabilities.maxImageCount;
		
		VkSurfaceTransformFlagBitsKHR preTransform;
		if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
			preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		else
			preTransform = surfaceCapabilities.currentTransform;

		VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
		swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchainCreateInfo.pNext = nullptr;
		swapchainCreateInfo.surface = (surface);
		swapchainCreateInfo.minImageCount = desiredNumberOfSwapchainImages;
		swapchainCreateInfo.imageFormat = format;
		swapchainCreateInfo.imageColorSpace = colorSpace;
		swapchainCreateInfo.imageExtent = swapchainExtent;
		swapchainCreateInfo.imageArrayLayers = 1;
		swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainCreateInfo.queueFamilyIndexCount = 0;
		swapchainCreateInfo.pQueueFamilyIndices = nullptr;
		swapchainCreateInfo.preTransform = preTransform;
		swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapchainCreateInfo.presentMode = swapchainPresentMode;
		swapchainCreateInfo.clipped = VK_TRUE;
		swapchainCreateInfo.oldSwapchain = oldSwapchain;

		assert(vkCreateSwapchainKHR((device), &swapchainCreateInfo, nullptr, &swapchain) == VK_SUCCESS);
		
		hasSwapchain = true;

		assert(vkGetSwapchainImagesKHR((device), (swapchain), &swapchainImageCount, nullptr) == VK_SUCCESS);
		std::unique_ptr<VkImage[]> swapchainImages(new VkImage[swapchainImageCount]);
		assert(vkGetSwapchainImagesKHR((device), (swapchain), &swapchainImageCount, swapchainImages.get()) == VK_SUCCESS);

		
		meshWithBuffers.resize(sceneLoader.GetModelCount());
		for(int m = 0; m < sceneLoader.GetModelCount(); ++m)
			meshWithBuffers[m].reset(new SwapchainBuffers[swapchainImageCount]);

		for (int m = 0; m < sceneLoader.GetModelCount(); ++m)
		{
			for (uint32_t i = 0; i < swapchainImageCount; ++i)
			{
				VkImageViewCreateInfo colorImageView = {};
				colorImageView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				colorImageView.pNext = nullptr;
				colorImageView.image = swapchainImages[i];
				colorImageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
				colorImageView.format = format;
				colorImageView.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

				meshWithBuffers[m][i].image = swapchainImages[i];

				assert(vkCreateImageView((device), &colorImageView, nullptr, &meshWithBuffers[m][i].view) == VK_SUCCESS);

				SetImageLayout((commandBuffer), swapchainImages[i], VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
			}
		}
	}

	VkFormat Application::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const
	{
		for (VkFormat format : candidates) {
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
				return format;
			if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
				return format;
		}

		throw std::runtime_error("failed to find supported format!");
	}

	VkFormat Application::FindDepthFormat() const
	{
		return FindSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	}

	void Application::PrepareDepth()
	{
		VkFormat depthFormat = FindDepthFormat();
		depth.memoryAllocationInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		depth.memoryAllocationInfo.pNext = nullptr;
		
		VkImageCreateInfo depthImageCreateInfo = {};
		depthImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		depthImageCreateInfo.pNext = nullptr;
		depthImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		depthImageCreateInfo.format = depthFormat;
		depthImageCreateInfo.extent = { uint32_t(eyeRenderTarget.textureWidth), uint32_t(eyeRenderTarget.textureHeight), 1 };
		depthImageCreateInfo.mipLevels = 1;
		depthImageCreateInfo.arrayLayers = 1;
		depthImageCreateInfo.samples = VkSampleCountFlagBits(uint32_t(applicationSettings.graphics.msaaSampleCount));
		depthImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		depthImageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		depthImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		depthImageCreateInfo.queueFamilyIndexCount = 0;
		depthImageCreateInfo.pQueueFamilyIndices = nullptr;
		depthImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		assert(vkCreateImage(device, &depthImageCreateInfo, nullptr, &depth.image) == VK_SUCCESS);

		depth.format = depthFormat;
		
		VkMemoryRequirements memoryRequirements = {};
		vkGetImageMemoryRequirements(device, depth.image, &memoryRequirements);
		depth.memoryAllocationInfo.allocationSize = memoryRequirements.size;
		depth.memoryAllocationInfo.memoryTypeIndex = 0;
		
		MemoryTypeFromProperties(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &depth.memoryAllocationInfo.memoryTypeIndex);

		assert(vkAllocateMemory(device, &depth.memoryAllocationInfo, nullptr, &depth.memory) == VK_SUCCESS);
		assert(vkBindImageMemory(device, depth.image, depth.memory, 0) == VK_SUCCESS);

		VkImageViewCreateInfo depthView = {};
		depthView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		depthView.pNext = nullptr;
		depthView.image = depth.image;
		depthView.viewType = VK_IMAGE_VIEW_TYPE_2D;
		depthView.format = depth.format;
		depthView.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };
		
		assert(vkCreateImageView(device, &depthView, nullptr, &depth.view) == VK_SUCCESS);



		SetImageLayout(commandBuffer, depth.image, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);

		
	}

	void Application::PrepareTexture()
	{

		const VkFormat tex_format = VK_FORMAT_R8G8B8A8_UNORM;
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, tex_format, &props);

		for (int32_t i = 0; i < sceneLoader.GetTextureCount(); i++)
		{
			textures[i].memoryAllocation.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			textures[i].memoryAllocation.pNext = nullptr;

			if ((props.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) && !useStagingBuffer) 
			{

				PrepareTextureImage(sceneLoader.GetTextureFile(i), &textures[i],VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

				SetImageLayout(commandBuffer, textures[i].image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_PREINITIALIZED, textures[i].layout, VK_ACCESS_HOST_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
				stagingTexture.image = VkImage();
			}
			else if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) 
			{

				PrepareTextureImage(sceneLoader.GetTextureFile(i), &stagingTexture, VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

				PrepareTextureImage(sceneLoader.GetTextureFile(i), &textures[i], VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

				SetImageLayout(commandBuffer, stagingTexture.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_HOST_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

				SetImageLayout(commandBuffer, textures[i].image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_HOST_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);

				VkImageSubresourceLayers subresource = {};
				subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				subresource.mipLevel = 0;
				subresource.baseArrayLayer = 0;
				subresource.layerCount = 1;

				VkImageCopy copyRegion = {};
				copyRegion.srcOffset = { 0, 0, 0 };
				copyRegion.srcSubresource = subresource;
				copyRegion.dstOffset = { 0,0,0 };
				copyRegion.dstSubresource = subresource;
				copyRegion.extent = { uint32_t(stagingTexture.textureWidth), uint32_t(stagingTexture.textureHeight), 1 };

				vkCmdCopyImage(commandBuffer, stagingTexture.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, textures[i].image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);


				SetImageLayout(commandBuffer, textures[i].image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, textures[i].layout, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

				

			}
			else 
			{
				assert(!"No support for R8G8B8A8_UNORM as texture image format");
			}

			VkSamplerCreateInfo  samplerInfo = {};
			samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerInfo.pNext = nullptr;
			samplerInfo.magFilter = VK_FILTER_LINEAR;
			samplerInfo.minFilter = VK_FILTER_LINEAR;
			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.mipLodBias = 0.0f;
			samplerInfo.anisotropyEnable = VK_TRUE;
			samplerInfo.maxAnisotropy = 16;
			samplerInfo.compareEnable = VK_FALSE;
			samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
			samplerInfo.minLod = 0.0f;
			samplerInfo.maxLod = 0.0f;
			samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
			samplerInfo.unnormalizedCoordinates = VK_FALSE;

			assert(vkCreateSampler(device, &samplerInfo, nullptr, &textures[i].sampler) == VK_SUCCESS);
		
			VkImageViewCreateInfo viewInfo = {};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.pNext = nullptr;
			viewInfo.image = textures[i].image;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = tex_format;
			viewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

			assert(vkCreateImageView(device, &viewInfo, nullptr, &textures[i].view) == VK_SUCCESS);

			
		}

	}

	void Application::PrepareDescriptorLayouts()
	{
		VkDescriptorSetLayoutBinding *layoutBindings = new VkDescriptorSetLayoutBinding[sceneLoader.GetTextureCount() + 1];
		layoutBindings[0].binding = 0;
		layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		layoutBindings[0].descriptorCount = 1;
		layoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		layoutBindings[0].pImmutableSamplers = nullptr;

		for (int i = 0; i < sceneLoader.GetTextureCount(); ++i)
		{
			layoutBindings[i+1].binding = i+1;
			layoutBindings[i+1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			layoutBindings[i+1].descriptorCount = 1;// textureCount;
			layoutBindings[i+1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			layoutBindings[i+1].pImmutableSamplers = nullptr;
		}

		VkDescriptorSetLayoutCreateInfo descriptorLayoutCreateInfo = {};
		descriptorLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorLayoutCreateInfo.pNext = nullptr;
		descriptorLayoutCreateInfo.bindingCount = uint32_t(sceneLoader.GetTextureCount() +1);
		descriptorLayoutCreateInfo.pBindings = layoutBindings;

		assert(vkCreateDescriptorSetLayout(device, &descriptorLayoutCreateInfo, nullptr, &descriptorLayout) == VK_SUCCESS);

		VkPipelineLayoutCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineInfo.pNext = nullptr;
		pipelineInfo.setLayoutCount = 1;
		pipelineInfo.pSetLayouts = &descriptorLayout;
		
		assert(vkCreatePipelineLayout(device, &pipelineInfo, nullptr, &pipelineLayout) == VK_SUCCESS);

		delete[] layoutBindings;
	}

	void Application::PrepareRenderPass()
	{
		VkAttachmentDescription attachments[2];

		attachments[0].format = format;
		attachments[0].samples = VkSampleCountFlagBits(uint32_t(applicationSettings.graphics.msaaSampleCount));
		attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachments[0].flags = 0; // VkAttachmentDescriptionFlags();

		attachments[1].format = depth.format;
		attachments[1].samples = VkSampleCountFlagBits(uint32_t(applicationSettings.graphics.msaaSampleCount));
		attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		attachments[1].flags = 0; // VkAttachmentDescriptionFlags();

		VkAttachmentReference colorReference = {};
		colorReference.attachment = 0;
		colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthReference = {};
		depthReference.attachment = 1;
		depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;


		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.inputAttachmentCount = 0;
		subpass.pInputAttachments = nullptr;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorReference;
		subpass.pDepthStencilAttachment = &depthReference;
		subpass.pResolveAttachments = nullptr;
		subpass.preserveAttachmentCount = 0;
		subpass.pPreserveAttachments = nullptr;

		VkRenderPassCreateInfo renderPassCreateInfo = {};
		renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassCreateInfo.pNext = nullptr;
		renderPassCreateInfo.attachmentCount = 2;
		renderPassCreateInfo.pAttachments = attachments;
		renderPassCreateInfo.subpassCount = 1;
		renderPassCreateInfo.pSubpasses = &subpass;
		renderPassCreateInfo.dependencyCount = 1;
		renderPassCreateInfo.pDependencies = &dependency;

		assert(vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &renderPass) == VK_SUCCESS);

	}

	void Application::PreparePipeline()
	{

		auto vertexShaderCode = ReadFileToCharVector("Shaders/vert.spv");
		auto fragmentShaderCode = ReadFileToCharVector("Shaders/frag.spv");

		VkShaderModule vertShaderModule;
		VkShaderModule fragShaderModule;

		CreateShaderModule(vertexShaderCode, vertShaderModule);
		CreateShaderModule(fragmentShaderCode, fragShaderModule);

		VkPipelineCacheCreateInfo pipelineCacheInfo = {};
		pipelineCacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		pipelineCacheInfo.pNext = nullptr;
		
		assert(vkCreatePipelineCache(device, &pipelineCacheInfo, nullptr, &pipelineCache) == VK_SUCCESS);

		VkPipelineShaderStageCreateInfo shaderStageCreateInfo[2];
		shaderStageCreateInfo[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageCreateInfo[0].pNext = nullptr;
		shaderStageCreateInfo[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		shaderStageCreateInfo[0].pName = "main";
		shaderStageCreateInfo[0].module = vertShaderModule;
		shaderStageCreateInfo[0].flags = VkPipelineShaderStageCreateFlags();
		shaderStageCreateInfo[0].pSpecializationInfo = nullptr;

		shaderStageCreateInfo[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageCreateInfo[1].pNext = nullptr;
		shaderStageCreateInfo[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		shaderStageCreateInfo[1].pName = "main";
		shaderStageCreateInfo[1].module = fragShaderModule;
		shaderStageCreateInfo[1].flags = VkPipelineShaderStageCreateFlags();
		shaderStageCreateInfo[1].pSpecializationInfo = nullptr;

		auto bindingDescription = Vertex::GetBindingDescription();
		auto attributeDescriptions = Vertex::GetAttributeDescriptions();

		VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.pNext = nullptr;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.vertexAttributeDescriptionCount = uint32_t(attributeDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {};
		inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyInfo.pNext = nullptr;
		inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = float(applicationSettings.graphics.width);
		viewport.height = float(applicationSettings.graphics.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f; 

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = { uint32_t(eyeRenderTarget.textureWidth), uint32_t(eyeRenderTarget.textureHeight) };


		VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
		viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportStateCreateInfo.viewportCount = 1;
		viewportStateCreateInfo.pViewports = &viewport;
		viewportStateCreateInfo.scissorCount = 1;
		viewportStateCreateInfo.pScissors = &scissor;
		
		VkPipelineRasterizationStateCreateInfo resterizationInfo = {};
		resterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		resterizationInfo.pNext = nullptr;
		resterizationInfo.depthClampEnable = VK_FALSE;
		resterizationInfo.rasterizerDiscardEnable = VK_FALSE;
		resterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
		resterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
		resterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		resterizationInfo.depthBiasEnable = VK_FALSE;
		resterizationInfo.lineWidth = 1.0f;

		uint32_t sampleMask = 0xFFFFFFFF;
		VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {};
		multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleStateCreateInfo.pNext = nullptr;
		multisampleStateCreateInfo.rasterizationSamples = VkSampleCountFlagBits(uint32_t(applicationSettings.graphics.msaaSampleCount));
		multisampleStateCreateInfo.pSampleMask = &sampleMask;
		multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
		multisampleStateCreateInfo.minSampleShading = 1.0f;
		multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
		multisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;

		VkStencilOpState stencilOpState = {};
		stencilOpState.failOp = VK_STENCIL_OP_KEEP;
		stencilOpState.passOp = VK_STENCIL_OP_KEEP;
		stencilOpState.compareOp = VK_COMPARE_OP_ALWAYS;

		VkPipelineDepthStencilStateCreateInfo depthStencilInfo = {};
		depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilInfo.pNext = nullptr;
		depthStencilInfo.depthTestEnable = VK_TRUE;
		depthStencilInfo.depthWriteEnable = VK_TRUE;
		depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
		depthStencilInfo.stencilTestEnable = VK_FALSE;
		depthStencilInfo.front = stencilOpState;
		depthStencilInfo.back = stencilOpState;

		VkPipelineColorBlendAttachmentState colorBlendAttachments[1];
		colorBlendAttachments[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachments[0].blendEnable = VK_TRUE;
		colorBlendAttachments[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachments[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachments[0].colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachments[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachments[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachments[0].alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo colorBlendInfo = {};
		colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendInfo.pNext = nullptr;
		colorBlendInfo.attachmentCount = 1;
		colorBlendInfo.pAttachments = colorBlendAttachments;

		VkDynamicState dynamicStates[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
		dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateCreateInfo.pNext = nullptr;
		dynamicStateCreateInfo.pDynamicStates = dynamicStates;
		dynamicStateCreateInfo.dynamicStateCount = 2;

		VkGraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.pNext = nullptr;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStageCreateInfo;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
		pipelineInfo.pViewportState = &viewportStateCreateInfo;
		pipelineInfo.pRasterizationState = &resterizationInfo;
		pipelineInfo.pMultisampleState = &multisampleStateCreateInfo;
		pipelineInfo.pDepthStencilState = &depthStencilInfo;
		pipelineInfo.pColorBlendState = &colorBlendInfo;
		pipelineInfo.pDynamicState = &dynamicStateCreateInfo;
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = nullptr;

		assert(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineInfo, nullptr, &pipeline) == VK_SUCCESS);

		vkDestroyShaderModule(device, vertShaderModule, nullptr);
		vkDestroyShaderModule(device, fragShaderModule, nullptr);
	}

	void Application::PrepareDescriptorPool()
	{

		VkDescriptorPoolSize *poolSize = new VkDescriptorPoolSize[sceneLoader.GetTextureCount() +1];
		poolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSize[0].descriptorCount = 3;
		for (int i = 0; i < sceneLoader.GetTextureCount(); ++i)
		{
			poolSize[i+1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			poolSize[i+1].descriptorCount = uint32_t(sceneLoader.GetTextureCount() * 3);
		}
		VkDescriptorPoolCreateInfo descriptorPoolCreate = {};
		descriptorPoolCreate.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolCreate.pNext = nullptr;
		descriptorPoolCreate.maxSets = 3;
		descriptorPoolCreate.poolSizeCount = uint32_t(sceneLoader.GetTextureCount() +1);
		descriptorPoolCreate.pPoolSizes = poolSize;

		assert(vkCreateDescriptorPool(device, &descriptorPoolCreate, nullptr, &descriptorPool) == VK_SUCCESS);
		delete[] poolSize;
	}

	void Application::PrepareDescriptorSet()
	{

		for(int eye = 0; eye < 3; ++eye)
		{
			VkDescriptorSetAllocateInfo allocationInfo = {};
			allocationInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocationInfo.pNext = nullptr;
			allocationInfo.descriptorSetCount = 1;
			allocationInfo.descriptorPool = descriptorPool;
			allocationInfo.pSetLayouts = &descriptorLayout;

			assert(vkAllocateDescriptorSets(device, &allocationInfo, &descriptorSet[eye]) == VK_SUCCESS);

			std::vector<VkDescriptorImageInfo> textureDescriptors;
			for(int i = 0; i < sceneLoader.GetTextureCount(); ++i)
			{
				VkDescriptorImageInfo textureDescriptorImageInfo;
				textureDescriptorImageInfo.sampler = textureSampler[i];
				textureDescriptorImageInfo.imageView = textureImageView[i];
				textureDescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				textureDescriptors.push_back(textureDescriptorImageInfo);
			}
			
			VkWriteDescriptorSet *writes = new VkWriteDescriptorSet[sceneLoader.GetTextureCount() +1];
			writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writes[0].pNext = nullptr;
			writes[0].dstSet = descriptorSet[eye];
			writes[0].dstBinding = 0;
			writes[0].dstArrayElement = 0;
			writes[0].descriptorCount = 1;
			writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writes[0].pBufferInfo = &uniformData[eye].bufferInfo;

			for (int i = 0; i < sceneLoader.GetTextureCount(); ++i)
			{
				writes[i+1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writes[i+1].pNext = nullptr;
				writes[i+1].dstSet = descriptorSet[eye];
				writes[i+1].dstBinding = i+1;
				writes[i+1].dstArrayElement = 0;
				writes[i+1].descriptorCount = 1;
				writes[i+1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				writes[i+1].pImageInfo = &textureDescriptors[i];
			}

			vkUpdateDescriptorSets(device, uint32_t(sceneLoader.GetTextureCount() + 1), writes, 0, nullptr);

			delete[] writes;
		}

	}

	void Application::PrepareFramebuffers()
	{

		VkImageView attachments[2];
		attachments[0] = eyeRenderTarget.view;
		attachments[1] = depth.view;

		VkFramebufferCreateInfo framebufferCreate = {};
		framebufferCreate.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreate.pNext = nullptr;
		framebufferCreate.attachmentCount = 2;
		framebufferCreate.pAttachments = attachments;
		framebufferCreate.width = eyeRenderTarget.textureWidth;
		framebufferCreate.height = eyeRenderTarget.textureHeight;
		framebufferCreate.layers = 1;
		framebufferCreate.renderPass = renderPass;

		assert(vkCreateFramebuffer(device, &framebufferCreate, nullptr, &framebuffer) == VK_SUCCESS);

	}

	void Application::FlushInitCommand()
	{

		if (!commandBuffer) return;

		assert(vkEndCommandBuffer(commandBuffer) == VK_SUCCESS);

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.pNext = nullptr;
		
		VkFence fence;

		vkCreateFence(device, &fenceInfo, nullptr, &fence);

		VkCommandBuffer commandBuffers[] = { commandBuffer };
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = commandBuffers;

		assert(vkQueueSubmit(graphicsQueue, 1, &submitInfo, fence) == VK_SUCCESS);

		assert(vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX) == VK_SUCCESS);

		vkFreeCommandBuffers(device, commandPool, 1, commandBuffers);
		vkDestroyFence(device, fence, nullptr);

		commandBuffer = VkCommandBuffer();
	}

	void Application::LoadModel()
	{

		if (!modelsLoaded)
		{
			for (int i = 0; i < sceneLoader.GetModelCount(); ++i)
			{
				Mesh newMesh;
				tinyobj::attrib_t attrib;
				std::vector<tinyobj::shape_t> shapes;
				std::vector<tinyobj::material_t> materials;
				std::string err;

				if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, sceneLoader.GetModelFile(i))) {
					throw std::runtime_error(err);
				}

				std::unordered_map<Vertex, int> uniqueVertices = {};

				for (const auto& shape : shapes) {
					for (const auto& index : shape.mesh.indices) {
						Vertex vertex = {};

						vertex.pos = {
							attrib.vertices[3 * index.vertex_index + 0],
							attrib.vertices[3 * index.vertex_index + 1],
							attrib.vertices[3 * index.vertex_index + 2]
						};

						vertex.texCoord = {
							attrib.texcoords[2 * index.texcoord_index + 0],
							1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
						};
						if (attrib.normals.size() > 0)
						{
							vertex.color = {
								attrib.normals[3 * index.normal_index + 0],
								attrib.normals[3 * index.normal_index + 1],
								attrib.normals[3 * index.normal_index + 2]
							};
						}
						else
						{
							vertex.color = { 1.0f, 1.0f, 1.0f };
						}
						if (uniqueVertices.count(vertex) == 0) {
							uniqueVertices[vertex] = int(newMesh.vertices.size());
							newMesh.vertices.push_back(vertex);
						}

						newMesh.indices.push_back(uniqueVertices[vertex]);
					}
				}

				// VERTEX BUFFER

				VkDeviceSize bufferSize = sizeof(newMesh.vertices[0]) * newMesh.vertices.size();

				VkBuffer stagingBuffer;
				VkDeviceMemory stagingBufferMemory;
				CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

				void* data;
				vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
				memcpy(data, newMesh.vertices.data(), (size_t)bufferSize);
				vkUnmapMemory(device, stagingBufferMemory);

				CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, newMesh.vertexBuffer, newMesh.vertexBufferMemory);

				CopyBuffer(stagingBuffer, newMesh.vertexBuffer, bufferSize);

				// INDEX BUFFER

				bufferSize = sizeof(newMesh.indices[0]) * newMesh.indices.size();

				CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

				data = nullptr;
				vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
				memcpy(data, newMesh.indices.data(), (size_t)bufferSize);
				vkUnmapMemory(device, stagingBufferMemory);

				CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, newMesh.indexBuffer, newMesh.indexBufferMemory);

				CopyBuffer(stagingBuffer, newMesh.indexBuffer, bufferSize);
				meshes.push_back(newMesh);

			}

			modelsLoaded = true;
		}

		VkBufferCreateInfo uniBufferInfo = {};
		uniBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		uniBufferInfo.pNext = nullptr;
		uniBufferInfo.size = sizeof(uniformBufferObject);
		uniBufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;


		for(int i = 0; i < 3; ++i)
		{
			
			assert(vkCreateBuffer(device, &uniBufferInfo, nullptr, &uniformData[i].buffer) == VK_SUCCESS);
			VkMemoryRequirements memoryRequirements;
			vkGetBufferMemoryRequirements(device, uniformData[i].buffer, &memoryRequirements);

			uniformData[i].memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			uniformData[i].memoryAllocateInfo.pNext = nullptr;
			uniformData[i].memoryAllocateInfo.allocationSize = memoryRequirements.size;
			uniformData[i].memoryAllocateInfo.memoryTypeIndex = 0;

			assert(MemoryTypeFromProperties(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &uniformData[i].memoryAllocateInfo.memoryTypeIndex));

			assert(vkAllocateMemory(device, &uniformData[i].memoryAllocateInfo, nullptr, &uniformData[i].memory) == VK_SUCCESS);

			void* data;
			assert(vkMapMemory(device, uniformData[i].memory, 0, uniformData[i].memoryAllocateInfo.allocationSize, VkMemoryMapFlags(), &data) == VK_SUCCESS);
			memcpy(data, &uniformBufferObject, sizeof(uniformBufferObject));
			vkUnmapMemory(device, uniformData[i].memory);

			assert(vkBindBufferMemory(device, uniformData[i].buffer, uniformData[i].memory, 0) == VK_SUCCESS);
			
			uniformData[i].bufferInfo.buffer = uniformData[i].buffer;
			uniformData[i].bufferInfo.offset = 0;
			uniformData[i].bufferInfo.range = sizeof(uniformBufferObject);

		}



	}

	void Application::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) const
	{
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to create buffer!");
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate buffer memory!");
		}

		vkBindBufferMemory(device, buffer, bufferMemory, 0);
	}

	void Application::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const
	{
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer copyCommandBuffer;
		vkAllocateCommandBuffers(device, &allocInfo, &copyCommandBuffer);

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(copyCommandBuffer, &beginInfo);

		VkBufferCopy copyRegion = {};
		copyRegion.size = size;
		vkCmdCopyBuffer(copyCommandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		vkEndCommandBuffer(copyCommandBuffer);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &copyCommandBuffer;

		vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphicsQueue);

		vkFreeCommandBuffers(device, commandPool, 1, &copyCommandBuffer);
	}

	uint32_t Application::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		throw std::runtime_error("failed to find suitable memory type!");
	}

	void Application::BuildImageOwnershipCmd(const uint32_t& i, const uint32_t& m)
	{
		VkCommandBufferBeginInfo commandBufferBeginInfo = {};
		commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		commandBufferBeginInfo.pInheritanceInfo = nullptr;
		commandBufferBeginInfo.pNext = nullptr;

		assert(vkBeginCommandBuffer(meshWithBuffers[m][i].graphicsToPresentCommand, &commandBufferBeginInfo) == VK_SUCCESS);

		VkImageMemoryBarrier imageOwnershipBarrier = {};
		imageOwnershipBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageOwnershipBarrier.pNext = nullptr;
		imageOwnershipBarrier.srcAccessMask = {};
		imageOwnershipBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		imageOwnershipBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		imageOwnershipBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		imageOwnershipBarrier.srcQueueFamilyIndex = graphicsQueueFamilyIndex;
		imageOwnershipBarrier.dstQueueFamilyIndex = presentQueueFamilyIndex;
		imageOwnershipBarrier.image = meshWithBuffers[m][i].image;
		imageOwnershipBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

		vkCmdPipelineBarrier(meshWithBuffers[m][i].graphicsToPresentCommand, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VkDependencyFlagBits(), 0, nullptr, 0, nullptr, 1, &imageOwnershipBarrier);

		assert(vkEndCommandBuffer(meshWithBuffers[m][i].graphicsToPresentCommand) == VK_SUCCESS);


	}

	void Application::DrawBuildCmd(VkCommandBuffer commandBuffer, int eye, const uint32_t& currentMesh)
	{

		VkCommandBufferBeginInfo commandInfo = {};
		commandInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		commandInfo.pNext = nullptr;
		commandInfo.pInheritanceInfo = nullptr;
		commandInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

		VkClearValue clearValues[2] = { { 0.647f, 0.906f, 1.0f, 0.2f },{ 1.0f, 0u } };
		VkRenderPassBeginInfo passInfo = {};
		passInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		passInfo.pNext = nullptr;
		passInfo.renderPass = renderPass;
		passInfo.framebuffer = framebuffer;
		passInfo.renderArea = { 0, 0, uint32_t(eyeRenderTarget.textureWidth), uint32_t(eyeRenderTarget.textureHeight) };
		passInfo.clearValueCount = 2;
		passInfo.pClearValues = clearValues;

		assert(vkBeginCommandBuffer(commandBuffer, &commandInfo) == VK_SUCCESS);

		SetImageLayout(commandBuffer, meshWithBuffers[currentMesh][currentBuffer].image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
		SetImageLayout(commandBuffer, eyeRenderTarget.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

		vkCmdBeginRenderPass(commandBuffer, &passInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet[eye], 0, nullptr);

		viewport.width = float(eyeRenderTarget.textureWidth);
		viewport.height = float(eyeRenderTarget.textureHeight);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor;
		scissor.offset = { 0, 0 };
		scissor.extent = { uint32_t(eyeRenderTarget.textureWidth), uint32_t(eyeRenderTarget.textureHeight) };
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		
		VkBuffer vertexBuffers[] = { meshes[currentMesh].vertexBuffer };
		VkDeviceSize offsets[] = { 0 };

		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, meshes[currentMesh].indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(commandBuffer, uint32_t(meshes[currentMesh].indices.size()), 1, 0, 0, 0);

		vkCmdEndRenderPass(commandBuffer);

		VkImageSubresourceLayers subresource = {};
		subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresource.mipLevel = 0;
		subresource.baseArrayLayer = 0;
		subresource.layerCount = 1;


		if (uint32_t(applicationSettings.graphics.msaaSampleCount) <= 1)
		{
			VkImageBlit blitRegion = {};
			blitRegion.srcSubresource = subresource;
			blitRegion.srcOffsets[0] = { 0, 0, 0 };
			blitRegion.srcOffsets[1] = { eyeRenderTarget.textureWidth, eyeRenderTarget.textureHeight, 1 };
			blitRegion.dstSubresource = subresource;
			blitRegion.dstOffsets[0] = { 0, 0, 0 };
			blitRegion.dstOffsets[1] = { int32_t(applicationSettings.graphics.width), int32_t(applicationSettings.graphics.height), 1 };
			vkCmdBlitImage(commandBuffer, eyeRenderTarget.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, meshWithBuffers[currentMesh][currentBuffer].image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blitRegion, VK_FILTER_NEAREST);
		}
		else
		{
			VkImageResolve resolveRegion = {};
			resolveRegion.srcSubresource = subresource;
			resolveRegion.srcOffset = { 0, 0, 0 };
			resolveRegion.dstSubresource = subresource;
			resolveRegion.dstOffset = { 0, 0, 0 };
			resolveRegion.extent = { uint32_t(applicationSettings.graphics.width), uint32_t(applicationSettings.graphics.height), uint32_t(1) };
			vkCmdResolveImage(commandBuffer, eyeRenderTarget.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, meshWithBuffers[currentMesh][currentBuffer].image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &resolveRegion);
		}

		SetImageLayout(commandBuffer, meshWithBuffers[currentMesh][currentBuffer].image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
		SetImageLayout(commandBuffer, eyeRenderTarget.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

		if (seperatePresentQueue)
		{
			VkImageMemoryBarrier imageMemoryBarrier = {};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.srcAccessMask = VkAccessFlags();
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			imageMemoryBarrier.srcQueueFamilyIndex = graphicsQueueFamilyIndex;
			imageMemoryBarrier.dstQueueFamilyIndex = presentQueueFamilyIndex;
			imageMemoryBarrier.image = meshWithBuffers[currentMesh][currentBuffer].image;
			imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VkDependencyFlagBits(), 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
		}

		assert(vkEndCommandBuffer(commandBuffer) == VK_SUCCESS);

	}

	void Application::BuildDrawToViewportCmd(VkCommandBuffer commandBuffer, const uint32_t& currentMesh)
	{
		VkCommandBufferBeginInfo commandInfo = {};
		commandInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		commandInfo.pNext = nullptr;
		commandInfo.pInheritanceInfo = nullptr;
		commandInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

		VkClearValue clearValues[2] = { { 0.647f, 0.906f, 1.0f, 0.2f },{ 1.0f, 0u } };
		VkRenderPassBeginInfo passInfo = {};
		passInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		passInfo.pNext = nullptr;
		passInfo.renderPass = renderPass;
		passInfo.framebuffer = framebuffer;
		passInfo.renderArea = { 0, 0, uint32_t(eyeRenderTarget.textureWidth), uint32_t(eyeRenderTarget.textureHeight) };
		passInfo.clearValueCount = 2;
		passInfo.pClearValues = clearValues;

		assert(vkBeginCommandBuffer(commandBuffer, &commandInfo) == VK_SUCCESS);

		SetImageLayout(commandBuffer, meshWithBuffers[currentMesh][currentBuffer].image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
		SetImageLayout(commandBuffer, eyeRenderTarget.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

		vkCmdBeginRenderPass(commandBuffer, &passInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet[2], 0, nullptr);

		viewport.width = float(applicationSettings.graphics.width);
		viewport.height = float(applicationSettings.graphics.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor;
		scissor.offset = { 0, 0 };
		scissor.extent = { uint32_t(applicationSettings.graphics.width), uint32_t(applicationSettings.graphics.height) };
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);


		VkBuffer vertexBuffers[] = { meshes[currentMesh].vertexBuffer };
		VkDeviceSize offsets[] = { 0 };

		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, meshes[currentMesh].indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(commandBuffer, uint32_t(meshes[currentMesh].indices.size()), 1, 0, 0, 0);

		vkCmdEndRenderPass(commandBuffer);

		VkImageSubresourceLayers subresource = {};
		subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresource.mipLevel = 0;
		subresource.baseArrayLayer = 0;
		subresource.layerCount = 1;


		if (uint32_t(applicationSettings.graphics.msaaSampleCount) <= 1)
		{
			VkImageBlit blitRegion = {};
			blitRegion.srcSubresource = subresource;
			blitRegion.srcOffsets[0] = { 0, 0, 0 };
			blitRegion.srcOffsets[1] = { int32_t(applicationSettings.graphics.width), int32_t(applicationSettings.graphics.height), 1 };
			blitRegion.dstSubresource = subresource;
			blitRegion.dstOffsets[0] = { 0, 0, 0 };
			blitRegion.dstOffsets[1] = { int32_t(applicationSettings.graphics.width), int32_t(applicationSettings.graphics.height), 1 };
			vkCmdBlitImage(commandBuffer, eyeRenderTarget.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, meshWithBuffers[currentMesh][currentBuffer].image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blitRegion, VK_FILTER_NEAREST);
		}
		else
		{
			VkImageResolve resolveRegion = {};
			resolveRegion.srcSubresource = subresource;
			resolveRegion.srcOffset = { 0, 0, 0 };
			resolveRegion.dstSubresource = subresource;
			resolveRegion.dstOffset = { 0, 0, 0 };
			resolveRegion.extent = { uint32_t(applicationSettings.graphics.width), uint32_t(applicationSettings.graphics.height), uint32_t(1) };
			vkCmdResolveImage(commandBuffer, eyeRenderTarget.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, meshWithBuffers[currentMesh][currentBuffer].image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &resolveRegion);
		}

		SetImageLayout(commandBuffer, meshWithBuffers[currentMesh][currentBuffer].image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
		SetImageLayout(commandBuffer, eyeRenderTarget.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

		if (seperatePresentQueue)
		{
			VkImageMemoryBarrier imageMemoryBarrier = {};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.srcAccessMask = VkAccessFlags();
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			imageMemoryBarrier.srcQueueFamilyIndex = graphicsQueueFamilyIndex;
			imageMemoryBarrier.dstQueueFamilyIndex = presentQueueFamilyIndex;
			imageMemoryBarrier.image = meshWithBuffers[currentMesh][currentBuffer].image;
			imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VkDependencyFlagBits(), 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
		}

		assert(vkEndCommandBuffer(commandBuffer) == VK_SUCCESS);
	}

	void Application::SetImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageAspectFlags aspectMask, VkImageLayout oldLayout, VkImageLayout newLayout, VkAccessFlags srcAccessMask, VkPipelineStageFlags src_stages, VkPipelineStageFlags dest_stages) const
	{
		assert(commandBuffer);

		auto DstAccessMask = [](VkImageLayout const &layout) {
			VkAccessFlags flags;

			switch (layout) {
			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				// Make sure anything that was copying from this image has
				// completed
				flags = VK_ACCESS_TRANSFER_WRITE_BIT;
				break;
			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				flags = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;
			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				flags = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;
			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				// Make sure any Copy or CPU writes to image are flushed
				flags = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
				break;
			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				flags = VK_ACCESS_TRANSFER_READ_BIT;
				break;
			case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
				flags = VK_ACCESS_MEMORY_READ_BIT;
				break;
			default:
				break;
			}

			return flags;
		};

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.pNext = nullptr;
		barrier.srcAccessMask = srcAccessMask;
		barrier.dstAccessMask = DstAccessMask(newLayout);
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = 0;
		barrier.dstQueueFamilyIndex = 0;
		barrier.image = image;
		barrier.subresourceRange = {aspectMask, 0, 1, 0, 1 };

		vkCmdPipelineBarrier(commandBuffer, src_stages, dest_stages, VkDependencyFlagBits(), 0, nullptr, 0, nullptr, 1, &barrier);

	}

	void Application::CreateShaderModule(const std::vector<char>& code, VkShaderModule& shaderModule) const
	{
		VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
		shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderModuleCreateInfo.codeSize = code.size();
		shaderModuleCreateInfo.pCode = (uint32_t*)code.data();
		if (vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create shader module!");
		}
	}

	void Application::CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, VkImageLayout initalImageLayout) const
	{
		VkImageCreateInfo imageInfo = {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format;
		imageInfo.tiling = tiling;
		imageInfo.initialLayout = initalImageLayout;
		imageInfo.usage = usage;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
			throw std::runtime_error("failed to create image!");
		}

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(device, image, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = 0;

		MemoryTypeFromProperties(memRequirements.memoryTypeBits, properties, &allocInfo.memoryTypeIndex);

		if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate image memory!");
		}

		vkBindImageMemory(device, image, imageMemory, 0);
	}

	void Application::PrepareTextureImage(const char* filename, Texture* tex_obj, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags required_props) const
	{
		int texWidth, texHeight, texChannels;

		stbi_uc* pixels = stbi_load(filename, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		VkDeviceSize imageSize = texWidth * texHeight * 4;

		//if this somehow one of these failed (they shouldn't), return failure
		if ((pixels == 0) || (texWidth == 0) || (texHeight == 0))
			throw std::runtime_error("failed to load texture image!");
		

		VkImageCreateInfo imageCreatInfo = {};
		imageCreatInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreatInfo.pNext = nullptr;
		imageCreatInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreatInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
		imageCreatInfo.extent = { uint32_t(texWidth), uint32_t(texHeight), 1 };
		imageCreatInfo.mipLevels = 1;
		imageCreatInfo.arrayLayers = 1;
		imageCreatInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreatInfo.tiling = tiling;
		imageCreatInfo.usage = usage;
		imageCreatInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreatInfo.queueFamilyIndexCount = 0;
		imageCreatInfo.pQueueFamilyIndices = nullptr;
		imageCreatInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

		assert(vkCreateImage(device, &imageCreatInfo, nullptr, &tex_obj->image) == VK_SUCCESS);
		VkMemoryRequirements memoryRequirements = {};
		vkGetImageMemoryRequirements(device, tex_obj->image, &memoryRequirements);

		tex_obj->memoryAllocation.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		tex_obj->memoryAllocation.pNext = nullptr;
		tex_obj->memoryAllocation.allocationSize = memoryRequirements.size;
		tex_obj->memoryAllocation.memoryTypeIndex = 0;

		assert(MemoryTypeFromProperties(memoryRequirements.memoryTypeBits, required_props, &tex_obj->memoryAllocation.memoryTypeIndex));
	
		assert(vkAllocateMemory(device, &tex_obj->memoryAllocation, nullptr, &tex_obj->memory) == VK_SUCCESS);
		assert(vkBindImageMemory(device, tex_obj->image, tex_obj->memory, 0) == VK_SUCCESS);

		if(required_props & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
		{
			
			VkImageSubresource subresource = {};
			subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresource.arrayLayer = 0;
			subresource.mipLevel = 0;
			
			VkSubresourceLayout layout = {};
			vkGetImageSubresourceLayout(device, tex_obj->image, &subresource, &layout);

			void* data = nullptr;
			assert(vkMapMemory(device, tex_obj->memory, 0, tex_obj->memoryAllocation.allocationSize, VkMemoryMapFlags(), &data) == VK_SUCCESS);
			memcpy(data, pixels, static_cast<size_t>(imageSize));
			vkUnmapMemory(device, tex_obj->memory);

		}

		stbi_image_free(pixels);
		
		tex_obj->layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}

	void Application::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageView& imageView) const
	{
		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;
		viewInfo.subresourceRange.aspectMask = aspectFlags;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture image view!");
		}
	}

	void Application::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) const
	{
		
		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

			if (HasDepthStencilComponent(format)) {
				barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			}
		}
		else {
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}

		if (oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
		{
			barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		}
		else {
			throw std::invalid_argument("unsupported layout transition!");
		}


		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		else if (oldLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
		else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		else
			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	}

	void Application::CopyImage(VkImage srcImage, VkImage dstImage, uint32_t width, uint32_t height) const
	{

		VkImageSubresourceLayers subResource = {};
		subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subResource.baseArrayLayer = 0;
		subResource.mipLevel = 0;
		subResource.layerCount = 1;

		VkImageCopy region = {};
		region.srcSubresource = subResource;
		region.dstSubresource = subResource;
		region.srcOffset = { 0, 0, 0 };
		region.dstOffset = { 0, 0, 0 };
		region.extent.width = width;
		region.extent.height = height;
		region.extent.depth = 1;
		
		vkCmdCopyImage( commandBuffer, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
		
	}

	void Application::OnWindowResized(GLFWwindow* window, int width_, int height_)
	{
		if (width_ == 0 || height_ == 0) return;

		Application* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
		int newWidth, newHeight;
		glfwGetWindowSize(window, &newWidth, &newHeight);
		app->applicationSettings.graphics.width = newWidth;
		app->applicationSettings.graphics.height = newHeight;
		app->Resize();
	}

	VkCommandBuffer Application::GenerateSingleCommandBuffer() const
	{
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

		return commandBuffer;
	}

	void Application::CreateTextureImage()
	{
		for (int i = 0; i < sceneLoader.GetTextureCount(); ++i)
		{
			int texWidth, texHeight, texChannels;

			stbi_uc* pixels = stbi_load(sceneLoader.GetTextureFile(i), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
			VkDeviceSize imageSize = texWidth * texHeight * 4;

			//if this somehow one of these failed (they shouldn't), return failure
			if ((pixels == 0) || (texWidth == 0) || (texHeight == 0))
				throw std::runtime_error("failed to load texture image!");

			VkImage textureImageInternal;
			VkDeviceMemory textureImageMemoryInternal;

			VkImage stagingImage;
			VkDeviceMemory stagingImageMemory;
			CreateImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingImage, stagingImageMemory);

			VkImageSubresource subresource = {};
			subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresource.mipLevel = 0;
			subresource.arrayLayer = 0;

			VkSubresourceLayout stagingImageLayout;
			vkGetImageSubresourceLayout(device, stagingImage, &subresource, &stagingImageLayout);

			void* data;
			vkMapMemory(device, stagingImageMemory, 0, imageSize, 0, &data);

			if (stagingImageLayout.rowPitch == texWidth * 4) {
				memcpy(data, pixels, static_cast<size_t>(imageSize));
			}
			else {
				uint8_t* dataBytes = reinterpret_cast<uint8_t*>(data);

				for (int y = 0; y < texHeight; y++) {
					memcpy(&dataBytes[y * stagingImageLayout.rowPitch], &pixels[y * texWidth * 4], texWidth * 4);
				}
			}

			vkUnmapMemory(device, stagingImageMemory);

			stbi_image_free(pixels);

			CreateImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImageInternal, textureImageMemoryInternal);

			TransitionImageLayout(stagingImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
			TransitionImageLayout(textureImageInternal, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			CopyImage(stagingImage, textureImageInternal, texWidth, texHeight);

			TransitionImageLayout(textureImageInternal, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

			textureImage.push_back(textureImageInternal);
			textureImageMemory.push_back(textureImageMemoryInternal);
		}
	}

	void Application::CreateTextureImageView()
	{
		for (int i = 0; i < sceneLoader.GetTextureCount(); ++i)
		{
			textureImageView.push_back(VkImageView());
			CreateImageView(textureImage[i], VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, textureImageView[i]);
		}
	}

	void Application::CreateTextureSampler()
	{
		VkSamplerCreateInfo samplerInfo = {};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = 16;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		for (int i = 0; i < sceneLoader.GetTextureCount(); ++i)
		{
			textureSampler.push_back(VkSampler());
			if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create texture sampler!");
			}
		}
	}

	void Application::KeyInputCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		Application* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
		if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
			app->currentMeshToDraw = app->currentMeshToDraw == 0 ? 1 : 0;
		else if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			app->quit = true;
			glfwSetWindowShouldClose(window, GLFW_TRUE);
		}
	}

	VkBool32 Application::DEBUG_CALLBACK(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData)
	{
		HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
		WORD saved_attributes;
		GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
		saved_attributes = consoleInfo.wAttributes;

		char buf[4096] = {0};
		switch (flags)
		{
		case VK_DEBUG_REPORT_ERROR_BIT_EXT:
			sprintf(buf, "VK :: ERROR %s %" PRIu64 ":%d: %s\n", layerPrefix, uint64_t(location), code, msg);
			break;
		case VK_DEBUG_REPORT_WARNING_BIT_EXT:
			sprintf(buf, "VK :: WARNING %s %" PRIu64 ":%d: %s\n", layerPrefix, uint64_t(location), code, msg);
			break;
		case VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT:
			sprintf(buf, "VK :: PERF %s %" PRIu64 ":%d: %s\n", layerPrefix, uint64_t(location), code, msg);
			break;
		case VK_DEBUG_REPORT_INFORMATION_BIT_EXT:
			sprintf(buf, "VK :: INFO %s %" PRIu64 ":%d: %s\n", layerPrefix, uint64_t(location), code, msg);
			break;
		case VK_DEBUG_REPORT_DEBUG_BIT_EXT:
			sprintf(buf, "VK :: DEBUG %s %" PRIu64 ":%d: %s\n", layerPrefix, uint64_t(location), code, msg);
			break;
		default:
			break;
		}

		if (flags == VK_DEBUG_REPORT_ERROR_BIT_EXT)
			SetConsoleTextAttribute(hConsole, 12);
		if (flags == VK_DEBUG_REPORT_WARNING_BIT_EXT)
			SetConsoleTextAttribute(hConsole, 14);

		printf("%s", buf);

		if (flags == VK_DEBUG_REPORT_ERROR_BIT_EXT || flags == VK_DEBUG_REPORT_WARNING_BIT_EXT)
			SetConsoleTextAttribute(hConsole, saved_attributes);

		return VK_FALSE;
	}
}

size_t std::hash<GlassEngine::Vertex>::operator()(GlassEngine::Vertex const& vertex) const
{
	return ((hash<glm::vec3>()(vertex.pos) ^
			(hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
		(hash<glm::vec2>()(vertex.texCoord) << 1);
}
