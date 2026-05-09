#include "GLFW/glfw3.h"
#include <vulkan/vulkan_raii.hpp>

#include <vector>
#include <fstream>

constexpr uint32_t SCREEN_WIDTH = 800.0f;
constexpr uint32_t SCREEN_HEIGHT = 600.0f;


const std::vector<char const*> validationLayers =
{
	"VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

#define GLFW_INCLUDE_VULKAN

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
	vk::raii::SwapchainKHR				swapChain = nullptr;
	std::vector<vk::Image>				swapChainImages;
	vk::SurfaceFormatKHR				swapChainSurfaceFormat;
	vk::Extent2D						swapChainExtent;
	std::vector<vk::raii::ImageView>			swapChainImageViews;
	// add the device extension for checking
	std::vector<const char* > requiredDeviceExtensions = { vk::KHRSwapchainExtensionName };
	void initWindow();
	void initVulkan();
	void mainLoop();
	void cleanUp();
	void createInstance();
	void setupDebugMessenger();
	void pickPhysicalDevice();
	void createLogicalDevice();
	bool isDeviceSuitable(const vk::raii::PhysicalDevice& physicalDevice);
	void createSurface();
	void createSwapChain();
	void createImageViews();
	void createGraphicsPipeline();
	vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
	vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
	vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);
	vk::raii::ShaderModule createShaderModule(const std::vector<char>& code) const;
	uint32_t chooseSwapMinImageCount(const vk::SurfaceCapabilitiesKHR& capabilities);
	std::vector<const char*> getRequiredInstanceExtensions();
};