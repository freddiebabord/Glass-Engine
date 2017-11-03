#pragma once
#include <GLFW/glfw3.h>

namespace GlassEngine
{

	typedef struct
	{
		VkImage image;
		VkCommandBuffer commandBuffer[2];
		VkCommandBuffer graphicsToPresentCommand;
		VkImageView view;
	} SwapchainBuffers;

	typedef struct
	{
		VkImage image;
		VkFormat format;
		VkMemoryAllocateInfo memoryAllocationInfo;
		VkDeviceMemory memory;
		VkImageView view;
	} Depth;

	typedef struct
	{
		VkSampler sampler;
		VkImage image;
		VkImageLayout layout{ VK_IMAGE_LAYOUT_UNDEFINED };
		VkMemoryAllocateInfo memoryAllocation;
		VkDeviceMemory memory;
		VkImageView view;
		int32_t textureWidth{ 0 };
		int32_t textureHeight{ 0 };
	} Texture;

	typedef struct
	{
		VkBuffer buffer;
		VkMemoryAllocateInfo memoryAllocateInfo;
		VkDeviceMemory memory;
		VkDescriptorBufferInfo bufferInfo;
	} UniformData;

	struct UniformBufferObject {
		glm::mat4 model;
		glm::mat4 viewProjection;
		int currentMesh;
		glm::vec3 eyePosition;
	};

	struct Vertex {
		glm::vec3 pos;
		glm::vec3 color;
		glm::vec2 texCoord;

		static VkVertexInputBindingDescription GetBindingDescription() {
			VkVertexInputBindingDescription bindingDescription = {};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(Vertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptions() {
			std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};

			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(Vertex, pos);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(Vertex, color);

			attributeDescriptions[2].binding = 0;
			attributeDescriptions[2].location = 2;
			attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

			return attributeDescriptions;
		}

		bool operator==(const Vertex& other) const {
			return pos == other.pos && color == other.color && texCoord == other.texCoord;
		}
	};


	struct Mesh
	{
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;

		VkBuffer vertexBuffer;
		VkDeviceMemory vertexBufferMemory;
		VkBuffer indexBuffer;
		VkDeviceMemory indexBufferMemory;
	};

}
