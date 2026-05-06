#include "GLFW/glfw3.h"
#include <vulkan/vulkan_raii.hpp>

#include <vector>
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
	std::vector<vk::ImageView>			swapChainImageViews;
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
	vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
	vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
	vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);
	uint32_t chooseSwapMinImageCount(const vk::SurfaceCapabilitiesKHR& capabilities);
	std::vector<const char*> getRequiredInstanceExtensions();
};