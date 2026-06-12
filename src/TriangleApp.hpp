#pragma once
#define VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS
#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include "macros.h"
#include "GLFW/glfw3.h"
#include <vulkan/vulkan_raii.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
#include <chrono>
#include <array>
#include <vector>
#include <fstream>
#include <stb_image.h>
#include "tiny_obj_loader.h"
#include <unordered_map>

constexpr uint32_t SCREEN_WIDTH = 800.0f;
constexpr uint32_t SCREEN_HEIGHT = 600.0f;
constexpr uint32_t IN_FLIGHT_FRAMES = 2;



const std::vector<char const*> validationLayers =
{
	"VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

struct Vertex
{
	glm::vec3 positions;
	glm::vec2 texCoord;
	glm::vec3 color;

	static vk::VertexInputBindingDescription getBindingDescription()
	{
		return { .binding = 0, .stride = sizeof(Vertex), .inputRate = vk::VertexInputRate::eVertex };
	};
	static std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions()
	{
		return {{
				   {.location = 0, .binding = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = offsetof(Vertex, positions)},
				   {.location = 1, .binding = 0, .format = vk::Format::eR32G32Sfloat, .offset = offsetof(Vertex, texCoord)},
				   {.location = 2, .binding = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = offsetof(Vertex, color)}
			   }};
	};
	bool operator==(const Vertex& other) const
	{
		return other.color == color && positions == other.positions && texCoord == other.texCoord;
	}
};

template <>
struct std::hash<Vertex>
{
	size_t operator()(Vertex const& vertex) const noexcept
	{
		return ((hash<glm::vec3>()(vertex.positions) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
	}
};

//const std::vector<Vertex> vertices = 
//{
//	{{-0.5f,-0.5f,0.0f}, {0.0f,1.0f}, {1.0f, 0.0f, 0.0f }},
//	{{-0.5f, 0.5f,0.0f}, {0.0f,0.0f}, {1.0f, 1.0f, 1.0f }},
//	{{ 0.5f, 0.5f,0.0f}, {1.0f,0.0f}, {0.0f, 0.0f, 1.0f }},
//	{{ 0.5f,-0.5f,0.0f}, {1.0f,1.0f}, {0.0f, 1.0f, 0.0f }},
//
//	{{-0.5f,-0.5f,0.5f}, {0.0f,1.0f}, {1.0f, 0.0f, 0.0f }},
//	{{-0.5f, 0.5f,0.5f}, {0.0f,0.0f}, {1.0f, 1.0f, 1.0f }},
//	{{ 0.5f, 0.5f,0.5f}, {1.0f,0.0f}, {0.0f, 0.0f, 1.0f }},
//	{{ 0.5f,-0.5f,0.5f}, {1.0f,1.0f}, {0.0f, 1.0f, 0.0f }}
//};
//
//const std::vector<uint32_t> indices =
//{
//	0,1,2,2,3,0,
//	4,5,6,6,7,4
//};

struct UniformBufferObject
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 projection;
};



static std::vector<char> readFile(const std::string& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);
	if (!file)
	{
		throw std::runtime_error("failed to open a file in: " + filename);
	}
	size_t size = file.tellg();
	std::vector<char> buffer(size);
	file.seekg(0, std::ios::beg);
	file.read(buffer.data(), static_cast<std::streamsize>(size));
	file.close();
	return buffer;
};

class TriangleApp
{
public:
	void run()
	{
		initWindow();
		initVulkan();
		mainLoop();
		cleanUp();
	}

private:
	GLFWwindow* window = nullptr;
	vk::raii::Context					context;
	vk::raii::Instance					instance = nullptr;
	vk::raii::DebugUtilsMessengerEXT	debugMessenger = nullptr;
	vk::raii::SurfaceKHR				surface = nullptr;
	vk::raii::PhysicalDevice			physicalDevice = nullptr;
	vk::raii::Device					device = nullptr;
	vk::raii::Queue						graphicsQueue = nullptr;
	uint32_t							queueIndex = ~0;
	vk::raii::SwapchainKHR				swapChain = nullptr;
	std::vector<vk::Image>				swapChainImages;
	vk::SurfaceFormatKHR				swapChainSurfaceFormat;
	vk::Extent2D						swapChainExtent;
	std::vector<vk::raii::ImageView>	swapChainImageViews;
	vk::raii::DescriptorSetLayout		descriptorSetLayout = nullptr;
	vk::raii::PipelineLayout			pipelineLayout = nullptr;
	vk::raii::Pipeline					graphicsPipeline = nullptr;
	vk::raii::CommandPool				commandPool = nullptr;

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	vk::raii::Buffer					vertexBuffer = nullptr;
	vk::raii::DeviceMemory				vertexBufferMemory = nullptr;
	vk::raii::Buffer					indexBuffer = nullptr;
	vk::raii::DeviceMemory				indexBufferMemory = nullptr;

	vk::raii::DescriptorPool			descriptorPool = nullptr;
	std::vector<vk::raii::DescriptorSet>descriptorSets;

	std::vector<vk::raii::Buffer>		uniformBuffers;
	std::vector<vk::raii::DeviceMemory>	uniformBuffersMemory;
	std::vector<void*>					uniformBuffersMapped;

	vk::raii::Image						textureImage = nullptr;
	vk::raii::DeviceMemory				textureImageMemory = nullptr;
	vk::raii::ImageView					textureImageView = nullptr;
	vk::raii::Sampler					textureSampler = nullptr;

	vk::raii::Image						depthImage = nullptr;
	vk::raii::DeviceMemory				depthImageMemory = nullptr;
	vk::raii::ImageView					depthImageView = nullptr;
	vk::Format							depthFormat;



	std::vector<vk::raii::CommandBuffer>commandBuffers;
	std::vector<vk::raii::Semaphore>	renderFinishedSemaphores;
	std::vector<vk::raii::Semaphore>	presentCompleteSemaphores;
	std::vector<vk::raii::Fence>		drawFences;
	uint32_t frameIndex = 0;
	bool frameBufferResized = false;
	// add the device extension for checking
	std::vector<const char* > requiredDeviceExtensions = { vk::KHRSwapchainExtensionName };
	void initWindow();
	void initVulkan();
	void mainLoop();
	void cleanUp();
	void drawFrame();
	void createInstance();
	void setupDebugMessenger();
	void pickPhysicalDevice();
	void createLogicalDevice();
	bool isDeviceSuitable(const vk::raii::PhysicalDevice& physicalDevice);
	void createSurface();
	void createSwapChain();
	void createImageViews();
	void createDescriptorSetLayout();
	void createGraphicsPipeline();
	void createTextureImage();
	void createCommandPool();
	void createDepthResources();
	void createCommandBuffers();
	void createVertexBuffer();
	void createIndexBuffer();
	void createUniformBuffers();
	void updateUniformBuffer(uint32_t frameIndex);
	void createDescriptorPool();
	void createDescriptorSets();
	void loadModel();
	vk::raii::CommandBuffer beginSingleTimeCommands();
	vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);
	void endSingleTimeCommands(vk::raii::CommandBuffer&& commandBuffer);
	std::pair<vk::raii::Image, vk::raii::DeviceMemory> createImage(uint32_t width, uint32_t height, vk::Format format,
		vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties);
	std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
	void copyBuffer(vk::raii::Buffer& srcBuffer, vk::raii::Buffer& dstBuffer, vk::DeviceSize size);
	uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
	void transition_image_layout(
		vk::Image		image,
		vk::ImageLayout old_layout,
		vk::ImageLayout new_layout,
		vk::AccessFlags2	src_access_mask,
		vk::AccessFlags2	dst_access_mask,
		vk::PipelineStageFlags2 src_stage_mask,
		vk::PipelineStageFlags2 dst_stage_mask,
		vk::ImageAspectFlagBits image_aspect_flags
	);
	void copyBufferToImage(vk::raii::CommandBuffer& commandBuffer, const vk::raii::Buffer& buffer, vk::raii::Image& image, uint32_t width, uint32_t height);
	void transitionImageLayout(vk::raii::CommandBuffer& commandBuffer, const vk::raii::Image& image, const vk::ImageLayout& oldLayout, const vk::ImageLayout& newLayout);
	void recordCommandBuffer(uint32_t imageIndex);
	void createSyncObjects();
	void recreateSwapChain();
	void cleanupSwapChain();
	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
	vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
	vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
	vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);
	vk::raii::ShaderModule createShaderModule(const std::vector<char>& code) const;
	uint32_t chooseSwapMinImageCount(const vk::SurfaceCapabilitiesKHR& capabilities);
	std::vector<const char*> getRequiredInstanceExtensions();
	vk::raii::ImageView createImageView(const vk::Image& image, const vk::Format& format, vk::ImageAspectFlags aspectFlags);
	void createTextureImageView();
	void createTextureSampler();
};