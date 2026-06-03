#define VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include <vulkan/vulkan_raii.hpp>
#include <glm/glm.hpp>
#include <array>
#include <vector>
#include <fstream>

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
	glm::vec2 positions;
	glm::vec3 color;

	static vk::VertexInputBindingDescription getBindingDescription()
	{
		return { .binding = 0, .stride = sizeof(Vertex), .inputRate = vk::VertexInputRate::eVertex };
	};
	static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescriptions()
	{
		return {{
				   {.location = 0, .binding = 0, .format = vk::Format::eR32G32Sfloat,	 .offset = offsetof(Vertex, positions)},
				   {.location = 1, .binding = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = offsetof(Vertex, color)}
			   }};
	};
};

const std::vector<Vertex> vertices = 
{
	{{-0.5f,-0.5f }, {1.0f, 0.0f, 0.0f }},
	{{-0.5f, 0.5f }, {1.0f, 1.0f, 1.0f }},
	{{ 0.5f, 0.5f }, {0.0f, 0.0f, 1.0f }},
	{{ 0.5f,-0.5f }, {0.0f, 1.0f, 0.0f }}
};

const std::vector<uint16_t> indices =
{
	0,1,2,2,3,0
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
	vk::raii::PipelineLayout			pipelineLayout = nullptr;
	vk::raii::Pipeline					graphicsPipeline = nullptr;
	vk::raii::CommandPool				commandPool = nullptr;
	vk::raii::Buffer					vertexBuffer = nullptr;
	vk::raii::DeviceMemory				vertexBufferMemory = nullptr;
	vk::raii::Buffer					indexBuffer = nullptr;
	vk::raii::DeviceMemory				indexBufferMemory = nullptr;
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
	void createGraphicsPipeline();
	void createCommandPool();
	void createCommandBuffers();
	void createVertexBuffer();
	void createIndexBuffer();
	std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
	void copyBuffer(vk::raii::Buffer& srcBuffer, vk::raii::Buffer& dstBuffer, vk::DeviceSize size);
	uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
	void transition_image_layout(
		uint32_t	imageIndex,
		vk::ImageLayout old_layout,
		vk::ImageLayout new_layout,
		vk::AccessFlags2	src_access_mask,
		vk::AccessFlags2	dst_access_mask,
		vk::PipelineStageFlags2 src_stage_mask,
		vk::PipelineStageFlags2 dst_stage_mask
	);
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
};