#include "TriangleApp.hpp"
#include <iostream>

// callback function
static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT		severity,
													  vk::DebugUtilsMessageTypeFlagsEXT				type,
												const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, void*)
{
	if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError || severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
	{
		std::cerr << "validation layer: type " << to_string(type) << " msg: " << pCallbackData->pMessage << std::endl;
	}
	return vk::False;
}



bool TriangleApp::isDeviceSuitable(const vk::raii::PhysicalDevice& physicalDevice)
{
	// check if physicalDevice supports the Vulkan 1.3 API version
	bool supportsVulkan1_3 = physicalDevice.getProperties().apiVersion >= vk::ApiVersion13;

	// check if physicalDevice supports queue family with graphics operations
	auto queueFamilies = physicalDevice.getQueueFamilyProperties();
	
	bool supportsGraphics = false;
	for (const auto& queueFamily : queueFamilies)
	{
		if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
		{
			supportsGraphics = true;
			break;
		}
	}

	// check if all required physicalDevice extensions are available
	auto availableDeviceExtensions = physicalDevice.enumerateDeviceExtensionProperties();
	bool supportsAllRequiredExtensions = true;
	for (const auto& reqExtension : requiredDeviceExtensions)
	{
		bool supportsCurrentExtension = false;
		for (const auto& physDevExtension : availableDeviceExtensions)
		{
			if (strcmp(physDevExtension.extensionName, reqExtension) == 0)
			{
				supportsCurrentExtension = true;
				break;
			}
		}

		if (supportsCurrentExtension == false)
		{
			supportsAllRequiredExtensions = false;
			break;
		}
	}

	// check if the physical device supports the required features (dynamic rendering and extended dynamic state)
	auto features = physicalDevice.template getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features,
														 vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();

	bool supportsRequiredFeatures = features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
									features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;

	return supportsVulkan1_3 && supportsGraphics && supportsAllRequiredExtensions && supportsRequiredFeatures;
}

void TriangleApp::pickPhysicalDevice()
{
	std::vector<vk::raii::PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();
	auto deviceIt = physicalDevices.end();
	for (auto it = physicalDevices.begin(); it != physicalDevices.end(); ++it)
	{
		if (isDeviceSuitable(*it))
		{
			deviceIt = it;
			break;
		}
	}
	if (deviceIt == physicalDevices.end())
	{
		throw std::runtime_error("failed to find a suitable GPU!");
	}
	physicalDevice = *deviceIt;
	std::cout << "Device was chosen: " << physicalDevice.getProperties().deviceName << std::endl;
}

void TriangleApp::initWindow()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "VULKAN", nullptr, nullptr);
}

void TriangleApp::initVulkan()
{
	createInstance();
	setupDebugMessenger();
	createSurface();
	pickPhysicalDevice();
	createLogicalDevice();
	createSwapChain();
	createImageViews();
	createGraphicsPipeline();
}

void TriangleApp::createImageViews()
{
	assert(swapChainImageViews.empty());

	vk::ImageViewCreateInfo imageViewCreateInfo
	{
		.viewType = vk::ImageViewType::e2D,
		.format = swapChainSurfaceFormat.format
	};

	imageViewCreateInfo.components =
	{
		vk::ComponentSwizzle::eIdentity,
		vk::ComponentSwizzle::eIdentity,
		vk::ComponentSwizzle::eIdentity,
		vk::ComponentSwizzle::eIdentity
	};

	imageViewCreateInfo.subresourceRange =
	{
		.aspectMask = vk::ImageAspectFlagBits::eColor,
		.levelCount = 1,
		.layerCount = 1
	};

	for (auto& image : swapChainImages)
	{
		imageViewCreateInfo.image = image;
		swapChainImageViews.emplace_back(device, imageViewCreateInfo);
	}
}

void TriangleApp::createGraphicsPipeline()
{
	auto shaderCode = readFile("shaders/slang.spv");
	std::cout << "The size of the file is: " << shaderCode.size() << std::endl;

	vk::raii::ShaderModule shaderModule = createShaderModule(shaderCode);
	vk::PipelineShaderStageCreateInfo vertShaderStageInfo
	{
		.stage = vk::ShaderStageFlagBits::eVertex,
		.module = shaderModule,
		.pName = "vertMain"
	};

	vk::PipelineShaderStageCreateInfo fragShaderStageInfo
	{
		.stage = vk::ShaderStageFlagBits::eFragment,
		.module = shaderModule,
		.pName = "fragMain"
	};
	vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo , fragShaderStageInfo };

	vk::PipelineVertexInputStateCreateInfo inputVertexInfo;

	vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo
	{
		.topology = vk::PrimitiveTopology::eTriangleList
	};

	vk::PipelineViewportStateCreateInfo viewportStateCreateInfo
	{
		.viewportCount = 1,
		.scissorCount = 1
	};

	vk::PipelineRasterizationStateCreateInfo rasterizerStateInfo
	{
		.depthClampEnable = vk::False,
		.rasterizerDiscardEnable = vk::False,
		.polygonMode = vk::PolygonMode::eFill,
		.cullMode = vk::CullModeFlagBits::eBack,
		.frontFace = vk::FrontFace::eClockwise,
		.depthBiasEnable = vk::False,
		.lineWidth = 1.0f
	};

	vk::PipelineMultisampleStateCreateInfo multisamplingStateInfo
	{
		.rasterizationSamples = vk::SampleCountFlagBits::e1,
		.sampleShadingEnable = vk::False
	};

	vk::PipelineColorBlendAttachmentState colourBlendAttachmentState
	{
		.blendEnable = vk::False,
		.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
						  vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
	};

	vk::PipelineColorBlendStateCreateInfo colourBlendStateInfo
	{
		.logicOpEnable = vk::False,
		.logicOp = vk::LogicOp::eCopy,
		.attachmentCount = 1,
		.pAttachments = &colourBlendAttachmentState
	};

	std::vector<vk::DynamicState> dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
	vk::PipelineDynamicStateCreateInfo dynamicStateInfo
	{
		.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
		.pDynamicStates = dynamicStates.data()
	};

	vk::PipelineLayoutCreateInfo layoutInfo
	{
		.setLayoutCount = 0,
		.pushConstantRangeCount = 0
	};

	pipelineLayout = vk::raii::PipelineLayout(device, layoutInfo);
	
}

void TriangleApp::createSurface()
{
	VkSurfaceKHR rawSurface;
	if (glfwCreateWindowSurface(*instance, window, nullptr, &rawSurface) != 0)
	{
		throw std::runtime_error("failed to create window surface!");
	}
	surface = vk::raii::SurfaceKHR(instance, rawSurface);
}

vk::SurfaceFormatKHR TriangleApp::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
{
	// if our list of formats isn't empty we look for BGR 8
	assert(!availableFormats.empty());
	auto formatIt = std::ranges::find_if(availableFormats, [](const auto& current_format)
		{
			return current_format.format == vk::Format::eB8G8R8A8Srgb && current_format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
		});

	return formatIt != availableFormats.end() ? *formatIt : availableFormats[0];
}

vk::PresentModeKHR TriangleApp::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
{
	assert(std::ranges::any_of(availablePresentModes, [](const auto& presentMode) 
		{
			return presentMode == vk::PresentModeKHR::eFifo;
		}));
	bool isMailBoxPresent = std::ranges::any_of(availablePresentModes, [](const auto& presentMode)
		{
			return presentMode == vk::PresentModeKHR::eMailbox;
		});
	return isMailBoxPresent ? vk::PresentModeKHR::eMailbox : vk::PresentModeKHR::eFifo;
}

vk::Extent2D TriangleApp::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return capabilities.currentExtent;
	}
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);

	return
	{
		std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
		std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
	};
}

[[nodiscard]] vk::raii::ShaderModule TriangleApp::createShaderModule(const std::vector<char>& code) const
{
	vk::ShaderModuleCreateInfo shaderModuleCreateInfo
	{
		.codeSize = code.size() * sizeof(char),
		.pCode = reinterpret_cast<const uint32_t*>(code.data())
	};
	vk::raii::ShaderModule shaderModule(device, shaderModuleCreateInfo);
	return shaderModule;
}

uint32_t TriangleApp::chooseSwapMinImageCount(const vk::SurfaceCapabilitiesKHR& capabilities)
{
	auto minImageCount = std::max(3u, capabilities.minImageCount);
	if (capabilities.maxImageCount > 0 && minImageCount > capabilities.maxImageCount)
	{
		minImageCount = capabilities.maxImageCount;
	}
	return minImageCount;
}

void TriangleApp::createSwapChain()
{
	vk::SurfaceCapabilitiesKHR surfaceCapabilites = physicalDevice.getSurfaceCapabilitiesKHR(*surface);
	swapChainExtent = chooseSwapExtent(surfaceCapabilites);

	std::vector<vk::SurfaceFormatKHR> availableFormats = physicalDevice.getSurfaceFormatsKHR(*surface);
	swapChainSurfaceFormat = chooseSwapSurfaceFormat(availableFormats);

	std::vector<vk::PresentModeKHR> availablePresentFormats = physicalDevice.getSurfacePresentModesKHR(*surface);
	vk::PresentModeKHR presentMode = chooseSwapPresentMode(availablePresentFormats);

	uint32_t minImageCount = chooseSwapMinImageCount(surfaceCapabilites);

	vk::SwapchainCreateInfoKHR swapChainCreateInfo
	{
		.surface = *surface,
		.minImageCount = minImageCount,
		.imageFormat = swapChainSurfaceFormat.format,
		.imageColorSpace = swapChainSurfaceFormat.colorSpace,
		.imageExtent = swapChainExtent,
		.imageArrayLayers = 1,
		.imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
		.imageSharingMode = vk::SharingMode::eExclusive,
		.preTransform = surfaceCapabilites.currentTransform,
		.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
		.presentMode = presentMode,
		.clipped = true
	};
	swapChain = vk::raii::SwapchainKHR(device, swapChainCreateInfo);
	swapChainImages = swapChain.getImages();
}

void TriangleApp::createLogicalDevice()
{
	// looking for graphics queue family in physicalDevice and retreive an index of this queue
	// for creating vk::DeviceQueueCreateInfo
	std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
	uint32_t graphicsQueueFamilyPropertyIndex = ~0;
	for (uint32_t currIndex = 0; currIndex <  queueFamilyProperties.size(); currIndex++)
	{
		if ((queueFamilyProperties[currIndex].queueFlags & vk::QueueFlagBits::eGraphics) && 
			physicalDevice.getSurfaceSupportKHR(currIndex, *surface))
		{
			graphicsQueueFamilyPropertyIndex = currIndex;
			break;
		}
	}
	if (graphicsQueueFamilyPropertyIndex == ~0)
		throw std::runtime_error("Failed to find a Graphics and present QueueFamily");

	float queuePriority = 0.5f;

	vk::DeviceQueueCreateInfo deviceQueueCreateInfo
	{
		.queueFamilyIndex = graphicsQueueFamilyPropertyIndex,
		.queueCount = 1,
		.pQueuePriorities = &queuePriority
	};

	// Create a chain of additional device features
	vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features,
					   vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT, vk::PhysicalDeviceVulkan11Features> featureChain =
	{
		{},
		{.dynamicRendering = true},
		{.extendedDynamicState = true},
		{.shaderDrawParameters = true}
	};

	// Create the logical device
	vk::DeviceCreateInfo deviceCreateInfo
	{
		.pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &deviceQueueCreateInfo,
		.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensions.size()),
		.ppEnabledExtensionNames = requiredDeviceExtensions.data()
	};
	// get handle for our Logical Device and our Queue
	device = vk::raii::Device(physicalDevice, deviceCreateInfo);
	graphicsQueue = vk::raii::Queue(device, graphicsQueueFamilyPropertyIndex, 0);
}

void TriangleApp::mainLoop()
{
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
	}
}
void TriangleApp::cleanUp()
{
	glfwDestroyWindow(window);
	glfwTerminate();
}

void TriangleApp::createInstance()
{
	// set up application info
	constexpr vk::ApplicationInfo appInfo
	{
		.pApplicationName = "Hello Triangle",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "No Engine",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = vk::ApiVersion14
	};

	// Get the required layers
	std::vector<const char*> requiredLayers;
	if (enableValidationLayers)
	{
		requiredLayers.assign(validationLayers.begin(), validationLayers.end());
	}

	// Check if the all required layers are supported by Vulkan implementation
	auto unsupportedLayerIt = requiredLayers.end();
	auto layerProperties = context.enumerateInstanceLayerProperties();
	for (auto it = requiredLayers.begin(); it != requiredLayers.end(); ++it)
	{
		bool found = false;
		for (auto const& layerProp : layerProperties)
		{
			if (strcmp(*it, layerProp.layerName) == 0)
			{
				found = true;
				break;
			}
		}

		if (!found)
		{
			unsupportedLayerIt = it;
			break;
		}
	}
	if (unsupportedLayerIt != requiredLayers.end())
	{
		throw std::runtime_error("Required validation layer is not supported: " + std::string(*unsupportedLayerIt));
	}


	auto requiredExtensions = getRequiredInstanceExtensions();

	// Check if the all required GLFW extensions are supported by Vulkan
	auto extensionProperties = context.enumerateInstanceExtensionProperties();
	auto unsupportedExtensionIt = requiredExtensions.end();
	for (auto it = requiredExtensions.begin(); it != requiredExtensions.end(); ++it)
	{
		bool found = false;
		for (auto const& extensionProp : extensionProperties)
		{
			if (strcmp(*it, extensionProp.extensionName) == 0)
			{
				found = true;
				break;
			}
		}
		
		if (!found)
		{
			unsupportedExtensionIt = it;
			break;
		}
	}
	if (unsupportedExtensionIt != requiredExtensions.end())
	{
		throw std::runtime_error("Required GLFW extension not supported: " + std::string(*unsupportedExtensionIt));
	}
	
	// create info for instance about layers and extensions
	vk::InstanceCreateInfo createInfo
	{
		.pApplicationInfo = &appInfo,
		.enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
		.ppEnabledLayerNames = requiredLayers.data(),
		.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
		.ppEnabledExtensionNames = requiredExtensions.data()
	};
	instance = vk::raii::Instance(context, createInfo);
}

void TriangleApp::setupDebugMessenger()
{
	// disable debugMessages if not DEBUG
	if (!enableValidationLayers)
		return;
	
	vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
	vk::DebugUtilsMessageTypeFlagsEXT	  messageTypeFlags(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral  | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
														   | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
	vk::DebugUtilsMessengerCreateInfoEXT DebugUtilsMessengerCreateInfoEXT
	{
		.messageSeverity = severityFlags,
		.messageType = messageTypeFlags,
		.pfnUserCallback = &debugCallback
	};
	debugMessenger = instance.createDebugUtilsMessengerEXT(DebugUtilsMessengerCreateInfoEXT);
}

std::vector<const char* > TriangleApp::getRequiredInstanceExtensions()
{
	uint32_t glfwExtensionCount = 0;
	auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
	if (enableValidationLayers)
	{
		extensions.push_back(vk::EXTDebugUtilsExtensionName);
	}
	return extensions;
}