#pragma once
#ifdef _DEBUG
#define DEBUG_VERBOSE 1
#else
#define DEBUG_VERBOSE 0
#endif


// Allow max of 2 outstanding presentation operations
#define FRAME_LAG 2
#include "SceneLoader.h"
#include "ApplicationConfiguration.h"
#include "VulkanStructs.h"

static const char to_red[] = "\033...";
static const char to_black[] = "\033...";


static const char* const mapNames = {"Assets/Scenes/DemoScene.glassScene"};
static const char* const configurationFile = { "Assets/Core/DemoApplication.ini" };

namespace GlassEngine
{
	namespace Internal {
		class VRSystem;
	}

	class AudioManager;
	class InputManager;

	/// Used internally to create the Vulkan debug callback. This should not be called by a developer
	inline VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback);

	/// Used internally to destroy the Vulkan debug callback. This should not be called by a developer
	inline void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator);


	class Application
	{
	public:
		/// Default constructor to initialise local variables
		Application();

		~Application();

		/// This is basically the main() funation of the application
		void Run();

		/// Draw command buffers are submitted to the GPU (and the VR system) here
		/// @note TODO: Possbily find away of doing mutithreaded command buffer generation here for performance and flexability
		void Draw();

		/// Shader required uniform variables are updated here
		/// @note TODO: Implement push constants instead of this current method for greater flexability
		void UpdateUniformData();

		/// Load the applications .ini config file
		/// @note This is handled internally and should not be needed
		void LoadApplicationConfig();

		/// Initialise an OpenGL splashsceen that renders an image once while the main application loads
		/// @note TODO: Put this in a seperate thread with its own update loop so the window can be 'moved around' or visible on top when minimiseing etc
		void InitSplashscreen();

		/// Clean up the loading splashscreen by destroying the window and cleaning up the OpenGL caommands and resources
		void CleanupSplashscreen();

		/// Creates a Vulkan Instance, initialises the VR subsystem if VR is enabled, binds a report function if in debug mode
		void InitVk();

		/// Creates a Vulkan window in WINDOWED mode, also assigns the application icon to Windows.
		/// Window resizing and key callbacks are also created here
		void InitWindow();
		
		/// Creates a Vulkan Surface for rendering, sets up the graphics and present queues, creates a Vulkan logical device
		/// get a format for the swapchain from the created surface, set up fences and semaphores for rendering later
		void InitSwapchain();
		
		/// Prepare the Vulkan renderer. This is just used to encapsulate the setup of Vulkan objects.
		/// Included in this function is the creation of command pools which contain command buffers,
		/// Creation of model, depth and shader buffers, initialisation of OpenVR (if enabled), shader descriptors,
		/// the render pass and the pipeline are also created here. The final draw commands from the models are
		/// also created here
		void Prepare();

		/// Initialise the eye render target textures to the width and height of each screen in the VR headset scaled by a requested resolution
		/// if VR enabled. Otherwise, the eye render target textures will use the size of the window for reference
		void PrepareOpenVR();

		/// Get the size of the swapchain, and how many swapchain images are required before creating the swapchain and destroying the old one
		void PrepareBuffers();

		/// Create the depth buffer (texture) that will be used by the render pass / pipeline. This starts off as undefined.
		void PrepareDepth();

		/// Load in all of the textures for this scene into a format Vulkan can use through image layout transitions and barriers
		/// To speed up texture reading a staging texture / buffer is used
		/// Vulkan imgae samplers and views are also crteated here
		void PrepareTexture();

		/// Initialise the descriptor set layout based on a uniform buffer and a single image. The pipline laytout is also created here based
		/// on the descriptor layout
		void PrepareDescriptorLayouts();

		/// Set up the colour and depth attatchements to the subpass and the render pass.
		void PrepareRenderPass();

		/// Load in SPIR-V shaders and create shader modules.
		/// TODO: Create a concept of a material which houses a vertex and fragment shader
		/// bind the vertex descriptions to the shader.
		/// Initilise the pipeline, rasteriser, multisampling, shader states and attahcments which are bound to the pipeline
		void PreparePipeline();

		/// Create a descriptor pool so the first pool is for the uniform buffer and the seccond is for the loaded textures.
		void PrepareDescriptorPool();

		/// Foreach eye (2), allocate a descriptor set and image and set its write target in the graphics pipeline
		void PrepareDescriptorSet();

		/// Create a framebuffer with a render target and depth target
		void PrepareFramebuffers();

		/// Ensure all the set-up code is pushed through teh graphics pipeline before it is initially used by the applications game state
		void FlushInitCommand();

		/// If the models for the current scene have been loaded skip loading them from a file. Otherwise Set up a mesh object for each model. 
		/// The create vertex and index buffers used for draw commands. Uniform data is also allocated and mapped at this stage.
		void LoadModel();

		/// Transitions the mesh image from the graphics queue to the present queue
		/// NOTE: This is only used when a present queue is used
		/// @param i the target swapchain buffer
		/// @param m the target mesh buffer
		void BuildImageOwnershipCmd(const uint32_t& i, const uint32_t& m);

		/// Fills up a command buffer with the command to draw a specific mesh based on the current VR eye
		/// @param commandBuffer The target command buffer to fill
		/// @param eye The target screen buffer (left eye, right eye or viewport screen)
		/// @param currentMesh the currentMesh to get representational data
		void DrawBuildCmd(VkCommandBuffer commandBuffer, int eye, const uint32_t& currentMesh);
		
		/// Fills up a command buffer with the command to draw a specific mesh based on the viewport
		/// @param commandBuffer The target command buffer to fill
		/// @param currentMesh the currentMesh to get representational data
		void BuildDrawToViewportCmd(VkCommandBuffer commandBuffer, const uint32_t& currentMesh);

		/// Transition a Vulkan image to a new layout using a pipeline barrier
		/// @param commandBuffer The target command buffer to perform the transition
		/// @param image The target image
		/// @param aspectMask a bitmask indicating which aspect(s) of the image are included in the view for the subresource range. @see VkImageAspectFlagBits
		/// @param oldLayout the old layout of the image
		/// @param newLayout the target layout of the image
		/// @param srcAccessMask a bitmask indicating which aspect(s) of the image are included in the view for the barrier definition. @see VkImageAspectFlagBits
		/// @param src_stages defines the src stage mask - see Vulkan spec
		/// @param dest_stages defines the dest stage mask - see Vulkan spec
		void SetImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageAspectFlags aspectMask, VkImageLayout oldLayout, VkImageLayout newLayout, VkAccessFlags srcAccessMask, VkPipelineStageFlags src_stages, VkPipelineStageFlags dest_stages) const;
		
		/// Search the availble memory types within the physical device memory properties to match requirement mask and type bits
		/// @param typeBits TODO - this might not be needed by the dev?
		/// @param requirementsMask TODO - this might not be needed by the dev?
		/// @param typeIndex TODO - this might not be needed by the dev?
		/// @return true if the memory type was found from the specified properties
		bool MemoryTypeFromProperties(uint32_t typeBits, VkMemoryPropertyFlags requirementsMask, uint32_t *typeIndex) const;

		/// Create a shader module with the supplied SPIR byte code
		/// @param code The shader code from a .spv file
		/// @param shaderModule The shader module the function uses to create with new data
		void CreateShaderModule(const std::vector<char>& code, VkShaderModule& shaderModule) const;

		/// Load in the requested texture file into a vulkan image
		/// @param filename the texture file from a .glassScene file
		/// @param tex_obj The final Vulkan texturte that will be used / replaced
		/// @param tiling A VkImageTiling specifying the tiling arrangement of the data elements in memory
		/// @param usage A bitmask describing the intended usage of the image. See VkImageUsageFlagBits below for a description of the supported bits.
		/// @param required_props The images requied properties @see MemoryTypeFromProperties
		void PrepareTextureImage(const char *filename, Texture *tex_obj, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags required_props) const;

		/// Cleanup Vulkan objects
		void Cleanup();

		/// This is called internally to resize internal Vulkan data for the new window size
		void Resize();

		/// The window resize callback supplied by glfw.
		/// @note This is called internally and show not be called
		/// @param window The target glfw window
		/// @param width_ the new width of the window
		/// @param height_ the new height of the window
		static void OnWindowResized(GLFWwindow* window, int width_, int height_);
		
		/// Create an Vulkan image for use internally such as for depth textures
		/// @param width The width of the image
		/// @param height The height of the image
		/// @param format The format of the image
		/// @param tiling  A VkImageTiling specifying the tiling arrangement of the data elements in memory
		/// @param usage A bitmask describing the intended usage of the image. See VkImageUsageFlagBits below for a description of the supported bits.
		/// @param properties The images requied properties @see MemoryTypeFromProperties
		/// @paraam image The target Vulkan image object to create
		/// @paraam imageMemory The target memory location thats associated with the image
		/// @param initalImageLayout The initial layout of the image
		/// @see PrepareTextureImage
		void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, VkImageLayout initalImageLayout = VK_IMAGE_LAYOUT_PREINITIALIZED) const;
		
		/// Create a Vulkan Image View so the image can be used as a displayable image such as for VR rendering or a texture
		/// @param image The Vulkan image to crteate the view from
		/// @param format The format the image view should take
		/// @param aspectFlags A bitmask indicating which aspect(s) of the image are included in the view. See VkImageAspectFlagBits.
		/// @param imageView The image view object that will be created
		void CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageView& imageView) const;
		
		/// Transition an image from an old to new layout without specifiying a command buffer using a pipeline barrier
		/// @param image The image to transition
		/// @param format The format of the image 
		/// @param oldLayout The old layout of the image e.g. VK_IMAGE_LAYOUT_PREINITIALIZED
		/// @param newLayout The old layout of the image e.g. VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
		/// @see SetImageLayout
		void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) const;

		/// Copy and image from one vulkan image object into another
		/// @param srcImage The original image to copy
		/// @param dstImage The new vulksn image to copy into
		/// @param width The width of the imgae to be copied (usually the whole src images width)
		/// @param width The height of the imgae to be copied (usually the whole src images height)
		void CopyImage(VkImage srcImage, VkImage dstImage, uint32_t width, uint32_t height) const;

		/// Create a Vulkan buffer and bind memory to it
		/// @param size The size of the buffer to create
		/// @param usage The type of buffer that is to be created through Vulkan Usage flags
		/// @param properties The properties of the memory the buffer will use
		/// @param buffer The buffer that is created
		/// @param bufferMemory The buffer memory to bind
		void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) const;
		
		/// Copy one buffer into another 
		/// @param srcBuffer The original buffer to copy from
		/// @param dstBuffer The destination buffer to copy data into
		/// @param size The amount of data the buffer to copy, usually the source buffers existing size		
		void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const;

		/// Search the availble memory types within the physical device memory properties to match requirement mask and type bits
		/// @param typeFilter TODO - this might not be needed by the dev?
		/// @param properties TODO - this might not be needed by the dev?
		/// @return The memory type index that was found
		/// @see MemoryTypeFromProperties
		uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

		/// Find the current best supported depth format
		VkFormat FindDepthFormat() const;

		/// Find a supported format from the user supplied candidates
		/// @param candidates a collection of candidate formats to check support for
		/// @param tiling A VkImageTiling specifying the tiling arrangement of the data elements in memory
		/// @param features A bitmask of features from VkFormatFeatureFlagBits
		/// @return A format thats supported on the available hardware
		VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;


		/// Generate a single use command buffer (not to be used for model data drawing commands)
		/// @return The command buffer that was generated
		VkCommandBuffer GenerateSingleCommandBuffer() const;

		/// Create Vulkan texture Images from the texture files loaded from a .glassScene file
		void CreateTextureImage();
		
		/// Create Vulkan texture Image Views from Vulkan Images created in CreateTextureImage()
		void CreateTextureImageView();

		/// Create Vulkan Image Samplers based on textures and views created from CreateTextureImage() and CreateTextureImageView()
		void CreateTextureSampler();
		

		/// Called internally by GLFW when a key is pressed
		/// @note This will be deprecated once the input manager is more fucntional
		/// @param window The target glfw window to recieve key events from
		/// @param key The key that has caused an event
		/// @param scancode This is not used but is required by the function prototype
		/// @param action The action that was just performed e.g. key pressed
		/// @param mods Any modifier keys that were triggered e.g. shift
		static void KeyInputCallback(GLFWwindow* window, int key, int scancode, int action, int mods);


	private:
		GLFWwindow* window;
		GLFWwindow* splashscreenWindow;
		GLuint splashscreenWindowTexture;
		GLuint splashscreenFramebuffer = 0;
		Internal::VRSystem* vrSystem;
		AudioManager* audioManager;
		InputManager* inputManager;

		ApplicationSettings applicationSettings;

		SceneLoader sceneLoader;

		struct
		{
			glm::vec3 eye = { 0.0f, 3.0f, 5.0f };
			glm::vec3 origin = { 0, 0, 0 };
			glm::vec3 up = { 0, 1.0f, 0 };

			glm::mat4x4 projectionMatrix;
			glm::mat4x4 viewMatrix;
			glm::mat4x4 modelMatrix;
		};

		bool quit, initFinished, modelsLoaded = false;
		
#pragma region VULKAN VARS

#ifdef _DEBUG
		bool validate = true;
#else
		bool validate = false;
#endif

		VkPresentModeKHR presentMode;
		uint32_t frameCount;
		bool debugReport = false;

		uint32_t currentFrame = 0;
		uint32_t frameIndex = 0;
		
		uint32_t enabledExtensionCount = 0;
		uint32_t enabledLayerCount = 0;
		const char* enabledLayers[64];
		const char* extensionNames[64];

		VkInstance instance;
		VkDebugReportCallbackEXT callback;
		
		VkPhysicalDevice physicalDevice;
		VkPhysicalDeviceProperties physicalDeviceProperties;
		VkPhysicalDeviceFeatures physicalDeviceFeatures;
		VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;

		uint32_t queueFamilyCount;
		std::unique_ptr<VkQueueFamilyProperties[]> queueProperties;
		uint32_t graphicsQueueFamilyIndex = UINT32_MAX;
		uint32_t presentQueueFamilyIndex = UINT32_MAX;
		bool seperatePresentQueue = false;

		VkSurfaceKHR surface;
		VkDevice device;
		
		VkQueue graphicsQueue, presentQueue;
		VkFormat format;
		VkColorSpaceKHR colorSpace;

		VkFence fences[FRAME_LAG];
		VkSemaphore imageAquiredSemaphore[FRAME_LAG], drawCompleteSemaphore[FRAME_LAG], imageOwnershipSemaphore[FRAME_LAG];

		VkCommandPool commandPool;
		VkCommandBuffer commandBuffer;
		VkCommandPool presentCommandPool;

		uint32_t swapchainImageCount = 0;
		std::vector<std::unique_ptr<SwapchainBuffers[]>> meshWithBuffers;
		uint32_t currentBuffer = 0;

		VkSwapchainKHR swapchain;
		bool hasSwapchain = false;
		Depth depth;

		Texture eyeRenderTarget;
		
		VkDescriptorSetLayout descriptorLayout;
		VkPipelineLayout pipelineLayout;
		VkPipelineCache pipelineCache;
		VkPipeline pipeline;
		VkDescriptorPool descriptorPool;
		VkDescriptorSet descriptorSet[3];

		VkRenderPass renderPass;
		VkFramebuffer framebuffer;

		Texture *textures;
		Texture stagingTexture;
		UniformData uniformData[3];
		UniformBufferObject uniformBufferObject;

		bool useStagingBuffer = true;
		
		std::vector<Mesh> meshes;

		std::vector<VkImage> textureImage;
		std::vector<VkDeviceMemory> textureImageMemory;
		std::vector<VkImageView> textureImageView;
		std::vector<VkSampler> textureSampler;

		VkViewport viewport;
		bool modelLoaded = false;
#pragma endregion 

		int currentMeshToDraw = 0;
		bool sendPressedEvent = false;


		std::vector<glm::vec3> positions = { glm::vec3(-562, -890, -27), glm::vec3(37307, -558, -50668) };
		std::vector<glm::vec3> scales = { glm::vec3(1510), glm::vec3(1510) };

		/// Called by Vulkan in debug mode to print content to the console
		/// @note This should never be called by the developer!
		static VKAPI_ATTR VkBool32 VKAPI_CALL DEBUG_CALLBACK(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType,
		                                                     uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData);
	};
}

namespace std {
	/// This is required for std hash to 'play nice' with the Vertex structure prevents C2338 error
	template<> struct hash<GlassEngine::Vertex> {
		size_t operator()(GlassEngine::Vertex const& vertex) const;
	};
}

