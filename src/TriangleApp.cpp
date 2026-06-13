#define STB_IMAGE_IMPLEMENTATION
#define TINYOBJLOADER_DISABLE_FAST_FLOAT
#define TINYOBJLOADER_IMPLEMENTATION
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

	bool supportsRequiredFeatures = features.template get<vk::PhysicalDeviceFeatures2>().features.samplerAnisotropy &&
									features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
									features.template get<vk::PhysicalDeviceVulkan13Features>().synchronization2 &&
									features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;

	return supportsVulkan1_3 && supportsGraphics && supportsAllRequiredExtensions && supportsRequiredFeatures;
}



void TriangleApp::initWindow()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "VULKAN", nullptr, nullptr);
	glfwSetWindowPos(window, 4200,250);
	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void TriangleApp::initVulkan()
{
	createInstance();
	setupDebugMessenger();
	createSurface();
	pickPhysicalDevice();
	createLogicalDevice();
	createSwapChain();
	createDescriptorSetLayout();
	createDepthResources();
	createGraphicsPipeline();
	createCommandPool();
	createTextureImage();
	createImageViews();
	createTextureImageView();
	createTextureSampler();
	loadModel();
	createVertexBuffer();
	createIndexBuffer();
	createUniformBuffers();
	createDescriptorPool();
	createDescriptorSets();
	createCommandBuffers();
	createSyncObjects();
}


void TriangleApp::mainLoop()
{
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		drawFrame();
	}
	graphicsQueue.waitIdle();

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

	// set up debugMessenger object with enable certain conditions 
	// under which this messenger will trigger the callback.
	// VkDebugUtilsMessageSeverity/MessageFlagsEXT is a bitmask type for setting 
	// a mask of zero or more VkDebugUtilsMessageSeverity/MessageFlagBitsEXT
	vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
	vk::DebugUtilsMessageTypeFlagsEXT	  messageTypeFlags(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
		| vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
	vk::DebugUtilsMessengerCreateInfoEXT DebugUtilsMessengerCreateInfoEXT
	{
		.messageSeverity = severityFlags,
		.messageType = messageTypeFlags,
		.pfnUserCallback = &debugCallback // application callback function to call
	};
	debugMessenger = instance.createDebugUtilsMessengerEXT(DebugUtilsMessengerCreateInfoEXT);
}

void TriangleApp::createSurface()
{
	// At first taking a handler "rawSurface" using glfw for our certain window
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


void TriangleApp::createLogicalDevice()
{
	// looking for graphics queue family in physicalDevice and retreive an index of this queue
	// for creating vk::DeviceQueueCreateInfo
	std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
	uint32_t graphicsQueueFamilyPropertyIndex = ~0;
	for (uint32_t currIndex = 0; currIndex < queueFamilyProperties.size(); currIndex++)
	{
		if ((queueFamilyProperties[currIndex].queueFlags & vk::QueueFlagBits::eGraphics) &&
			physicalDevice.getSurfaceSupportKHR(currIndex, *surface))
		{
			queueIndex = currIndex;
			break;
		}
	}
	if (queueIndex == ~0)
		throw std::runtime_error("Failed to find a Graphics QueueFamily with ability for presentation to surface");

	float queuePriority = 0.5f;

	vk::DeviceQueueCreateInfo deviceQueueCreateInfo
	{
		.queueFamilyIndex = queueIndex,
		.queueCount = 1,
		.pQueuePriorities = &queuePriority
	};

	// Create a chain of additional device features
	vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features,
		vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT, vk::PhysicalDeviceVulkan11Features> featureChain =
	{
		{.features = {.samplerAnisotropy = true}},
		{.synchronization2 = true, .dynamicRendering = true,},
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
	graphicsQueue = vk::raii::Queue(device, queueIndex, 0);
}

void TriangleApp::createSwapChain()
{
	// get 
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

vk::raii::ImageView TriangleApp::createImageView(const vk::Image& image, const vk::Format& format, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels)
{

	vk::ImageViewCreateInfo imageViewCreateInfo
	{
		.image = image,
		.viewType = vk::ImageViewType::e2D,
		.format = format,
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
		.aspectMask = aspectFlags,
		.baseMipLevel = 0,
		.levelCount = mipLevels,
		.baseArrayLayer = 0,
		.layerCount = 1,
	};
	return vk::raii::ImageView(device, imageViewCreateInfo);
}

void TriangleApp::createTextureImageView()
{
	textureImageView = createImageView(*textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor, mipLevels);
}

void TriangleApp::createTextureSampler()
{
	vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();
	vk::SamplerCreateInfo samplerInfo
	{
		.magFilter = vk::Filter::eLinear,
		.minFilter = vk::Filter::eLinear,
		.mipmapMode = vk::SamplerMipmapMode::eLinear,
		.addressModeU = vk::SamplerAddressMode::eRepeat,
		.addressModeV = vk::SamplerAddressMode::eRepeat,
		.addressModeW = vk::SamplerAddressMode::eRepeat,
		.mipLodBias = 0.0f,
		.anisotropyEnable = vk::True,
		.maxAnisotropy = properties.limits.maxSamplerAnisotropy,
		.compareEnable = vk::False,
		.compareOp = vk::CompareOp::eAlways,
		.minLod = 0.0f,
		.maxLod = vk::LodClampNone,
		.borderColor = vk::BorderColor::eIntOpaqueBlack,
		.unnormalizedCoordinates = vk::False,
	};
	textureSampler = vk::raii::Sampler(device, samplerInfo);
}

void TriangleApp::createImageViews()
{
	assert(swapChainImageViews.empty());

	/*vk::ImageViewCreateInfo imageViewCreateInfo
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
	};*/

	for (auto& image : swapChainImages)
	{
		swapChainImageViews.emplace_back(createImageView(image, swapChainSurfaceFormat.format,vk::ImageAspectFlagBits::eColor, 1));
	}
}

void TriangleApp::createDescriptorSetLayout()
{
	std::array<vk::DescriptorSetLayoutBinding, 2> bindgings
	{ {
		{	// descriptor set for uniform buffer
			.binding = 0,
			.descriptorType = vk::DescriptorType::eUniformBuffer,
			.descriptorCount = 1,
			.stageFlags = vk::ShaderStageFlagBits::eVertex
		},
		{	// descriptor set for texture image
			.binding = 1,
			.descriptorType = vk::DescriptorType::eCombinedImageSampler,
			.descriptorCount = 1,
			.stageFlags = vk::ShaderStageFlagBits::eFragment
		}
	}};


	vk::DescriptorSetLayoutCreateInfo layoutInfo
	{
		.bindingCount = static_cast<uint32_t>(bindgings.size()),
		.pBindings = bindgings.data(),
	};
	descriptorSetLayout = vk::raii::DescriptorSetLayout(device, layoutInfo);
}

void TriangleApp::createDescriptorPool()
{
	std::array<vk::DescriptorPoolSize, 2> descriptorPoolSize
	{ { // Descriptor pool size for unifrom buffer and texture image
		{.type = vk::DescriptorType::eUniformBuffer, .descriptorCount = IN_FLIGHT_FRAMES},
		{.type = vk::DescriptorType::eCombinedImageSampler, .descriptorCount = IN_FLIGHT_FRAMES}
	} };



	vk::DescriptorPoolCreateInfo decriptorCreateInfo
	{
		.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
		.maxSets = IN_FLIGHT_FRAMES,
		.poolSizeCount = static_cast<uint32_t>(descriptorPoolSize.size()),
		.pPoolSizes = descriptorPoolSize.data()
	};

	descriptorPool = vk::raii::DescriptorPool(device, decriptorCreateInfo);
}

void TriangleApp::createDescriptorSets()
{

	std::vector<vk::DescriptorSetLayout> layouts(IN_FLIGHT_FRAMES, *descriptorSetLayout);

	vk::DescriptorSetAllocateInfo allocInfo
	{
		.descriptorPool = descriptorPool,
		.descriptorSetCount = static_cast<uint32_t>(layouts.size()),
		.pSetLayouts = layouts.data()
	};

	descriptorSets = device.allocateDescriptorSets(allocInfo);

	for (uint32_t i = 0; i < IN_FLIGHT_FRAMES; i++)
	{
		vk::DescriptorBufferInfo descriptorBufferInfo
		{
			.buffer = uniformBuffers[i],
			.offset = 0,
			.range = sizeof(UniformBufferObject)
		};
		vk::DescriptorImageInfo descriptorImageInfo
		{
			.sampler = textureSampler,
			.imageView = textureImageView,
			.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
		};
		std::array<vk::WriteDescriptorSet, 2> descriptorSetWrites
		{ {
			{.dstSet = descriptorSets[i],
				.dstBinding = 0,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = vk::DescriptorType::eUniformBuffer,
				.pBufferInfo = &descriptorBufferInfo
			},
			{
				.dstSet = descriptorSets[i],
				.dstBinding = 1,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = vk::DescriptorType::eCombinedImageSampler,
				.pImageInfo = &descriptorImageInfo
			}
		} };
		device.updateDescriptorSets(descriptorSetWrites, {});
	}
}

void TriangleApp::loadModel()
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, error;
	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &error, MODEL_PATH "viking_room.obj"))
	{
		throw std::runtime_error(warn + error);
	}

	std::unordered_map<Vertex, uint32_t> uniqueVertices{};
	for (const auto& shape : shapes)
	{
		for (const auto& index : shape.mesh.indices)
		{
			Vertex vertex{};

			vertex.positions =
			{
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			vertex.texCoord =
			{
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};
			vertex.color = { 1.0f,1.0f,1.0f };

			auto [it, inserted] = uniqueVertices.insert({ vertex, static_cast<uint32_t>(vertices.size()) });
			if (inserted)
			{
			vertices.push_back(vertex);	
			}
			indices.push_back(it->second);
		}
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

	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDecription = Vertex::getAttributeDescriptions();

	vk::PipelineVertexInputStateCreateInfo inputVertexInfo
	{
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &bindingDescription,
		.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDecription.size()),
		.pVertexAttributeDescriptions = attributeDecription.data(),
	};

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
		.frontFace = vk::FrontFace::eCounterClockwise,
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

	vk::PipelineDepthStencilStateCreateInfo depthStencil
	{
		.depthTestEnable = vk::True,
		.depthWriteEnable = vk::True,
		.depthCompareOp = vk::CompareOp::eLess,
		.depthBoundsTestEnable = vk::False,
		.stencilTestEnable = vk::False
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
		.setLayoutCount = 1,
		.pSetLayouts = &*descriptorSetLayout,
		.pushConstantRangeCount = 0
	};

	pipelineLayout = vk::raii::PipelineLayout(device, layoutInfo);
	

	// setting up programmable and fixed stages of the pipeline
	vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineCreateInfoChain =
	{
		{
			.stageCount = 2,
			.pStages = shaderStages,
			.pVertexInputState = &inputVertexInfo,
			.pInputAssemblyState = &inputAssemblyInfo,
			.pViewportState = &viewportStateCreateInfo,
			.pRasterizationState = &rasterizerStateInfo,
			.pMultisampleState = &multisamplingStateInfo,
			.pDepthStencilState = &depthStencil,
			.pColorBlendState = &colourBlendStateInfo,
			.pDynamicState = &dynamicStateInfo,
			.layout = pipelineLayout,
			.renderPass = nullptr,
		},
		{
			.colorAttachmentCount = 1,
			.pColorAttachmentFormats = &swapChainSurfaceFormat.format,
			.depthAttachmentFormat = depthFormat
		}
	};

	graphicsPipeline = vk::raii::Pipeline(device, nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>());
}

void TriangleApp::createTextureImage()
{
	int texWidth, texHeight, texChannels;
	
	auto* pixels = stbi_load(TEXTURE_PATH"viking_room.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	vk::DeviceSize imageSize = texWidth * texHeight * 4;

	if (!pixels)
	{
		throw std::runtime_error("failed to load texture");
	}

	mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

	auto [stagingBuffer, stagingBufferMemory] = createBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible |
		vk::MemoryPropertyFlagBits::eHostCoherent);

	void* data = stagingBufferMemory.mapMemory(0, imageSize);
	memcpy(data, pixels, imageSize);
	stagingBufferMemory.unmapMemory();

	stbi_image_free(pixels);

	std::tie(textureImage, textureImageMemory) = createImage(
		texWidth,
		texHeight,
		mipLevels,
		vk::Format::eR8G8B8A8Srgb,
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled,
		vk::MemoryPropertyFlagBits::eDeviceLocal);

	transitionImageLayout(textureImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, mipLevels);
	copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
	// transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps
	generateMipmaps(textureImage, vk::Format::eR8G8B8A8Srgb, texWidth, texHeight, mipLevels);
	
	//transitionImageLayout(textureImage, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, mipLevels);
}

void TriangleApp::createCommandPool()
{
	vk::CommandPoolCreateInfo commandPoolInfo
	{
		.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
		.queueFamilyIndex = queueIndex,
	};
	commandPool = vk::raii::CommandPool(device, commandPoolInfo);
}

void TriangleApp::generateMipmaps(vk::raii::Image& image, vk::Format imageFormat, int texWidth, int texHeight, uint32_t mipLevels)
{
	vk::FormatProperties formatProperties = physicalDevice.getFormatProperties(imageFormat);
	if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear))
	{
		throw std::runtime_error("texture image format does not support linear blitting");
	}

	std::unique_ptr<vk::raii::CommandBuffer> commandBuffer = beginSingleTimeCommands();

	vk::ImageMemoryBarrier barrier =
	{
		.srcAccessMask = vk::AccessFlagBits::eTransferWrite,
		.dstAccessMask = vk::AccessFlagBits::eTransferRead,
		.oldLayout = vk::ImageLayout::eTransferDstOptimal,
		.newLayout = vk::ImageLayout::eTransferSrcOptimal,
		.srcQueueFamilyIndex = vk::QueueFamilyIgnored,
		.dstQueueFamilyIndex = vk::QueueFamilyIgnored,
		.image = image
	};
	barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;
	int mipWidth = texWidth;
	int mipHeight = texHeight;

	for (uint32_t i = 1; i < mipLevels; i++)
	{
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
		barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

		commandBuffer->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, barrier);

		vk::ArrayWrapper1D<vk::Offset3D, 2> offsets, dstOffsets;
		offsets[0] = vk::Offset3D(0, 0, 0);
		offsets[1] = vk::Offset3D(mipWidth, mipHeight, 1);
		dstOffsets[0] = vk::Offset3D(0, 0, 0);
		dstOffsets[1] = vk::Offset3D(mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1);
		vk::ImageBlit blit = { .srcSubresource = {}, .srcOffsets = offsets, .dstSubresource = {}, .dstOffsets = dstOffsets };
		blit.srcSubresource = vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, i - 1, 0, 1);
		blit.dstSubresource = vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, i, 0, 1);

		commandBuffer->blitImage(image, vk::ImageLayout::eTransferSrcOptimal, image, vk::ImageLayout::eTransferDstOptimal, { blit }, vk::Filter::eLinear);
		barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
		barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

		commandBuffer->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, barrier);

		if (mipWidth > 1)
			mipWidth = mipWidth / 2;
		if (mipHeight > 1)
			mipHeight = mipHeight / 2;
	}
	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
	barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
	barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
	barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

	commandBuffer->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, barrier);

	endSingleTimeCommands(*commandBuffer);

}

void TriangleApp::createCommandBuffers()
{
	vk::CommandBufferAllocateInfo bufferAllocInfo
	{
		.commandPool = commandPool,
		.level = vk::CommandBufferLevel::ePrimary,
		.commandBufferCount = IN_FLIGHT_FRAMES
	};
	commandBuffers = vk::raii::CommandBuffers(device, bufferAllocInfo);
}


void TriangleApp::createVertexBuffer()
{
	vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
	auto [stagingBuffer, stagingBufferMemory] = createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

	// copy the vertex and color data into that device memory
	void* data = stagingBufferMemory.mapMemory(0, bufferSize);
	memcpy(data, vertices.data(), bufferSize);
	stagingBufferMemory.unmapMemory();
	// and bind the device memory to the vertex buffer

	std::tie(vertexBuffer, vertexBufferMemory) = createBuffer(bufferSize, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
		vk::MemoryPropertyFlagBits::eDeviceLocal);

	copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
}

void TriangleApp::createIndexBuffer()
{
	vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();
	auto [stagingBuffer, stagingBufferMemory] = createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
	
	void* data = stagingBufferMemory.mapMemory(0, bufferSize);
	memcpy(data, indices.data(), (size_t)bufferSize);
	stagingBufferMemory.unmapMemory();

	std::tie(indexBuffer, indexBufferMemory) = createBuffer(bufferSize, vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
		vk::MemoryPropertyFlagBits::eDeviceLocal);

	copyBuffer(stagingBuffer, indexBuffer, bufferSize);
}

void TriangleApp::createUniformBuffers()
{
	vk::DeviceSize bufferSize = sizeof(UniformBufferObject);
	for (size_t i = 0; i < IN_FLIGHT_FRAMES; i++)
	{
		auto [uniformBuffer, uniformBufferMemory] = createBuffer(bufferSize,
			vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible);
		uniformBuffers.emplace_back(std::move(uniformBuffer));
		uniformBuffersMemory.emplace_back(std::move(uniformBufferMemory));
		uniformBuffersMapped.emplace_back(uniformBuffersMemory.back().mapMemory(0, bufferSize));
	}
}


vk::Format TriangleApp::findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features)
{
	for (const auto format : candidates)
	{
		vk::FormatProperties props = physicalDevice.getFormatProperties(format);

		if (((tiling == vk::ImageTiling::eLinear) && ((props.linearTilingFeatures & features) == features)) ||
			((tiling == vk::ImageTiling::eOptimal) && ((props.optimalTilingFeatures & features) == features)))
		{
			return format;
		}
	}
	throw std::runtime_error("failed to find supported format!");
}

void TriangleApp::createDepthResources()
{
	depthFormat = findSupportedFormat({ vk::Format::eD32Sfloat,vk::Format::eD24UnormS8Uint,vk::Format::eD32SfloatS8Uint },
										     vk::ImageTiling::eOptimal,
											 vk::FormatFeatureFlagBits::eDepthStencilAttachment);

	std::tie(depthImage, depthImageMemory) = createImage(swapChainExtent.width,
														 swapChainExtent.height,
														 1,
														 depthFormat,
														 vk::ImageTiling::eOptimal,
														 vk::ImageUsageFlagBits::eDepthStencilAttachment,
														 vk::MemoryPropertyFlagBits::eDeviceLocal);

	depthImageView = createImageView(depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth, 1);

}

void TriangleApp::updateUniformBuffer(uint32_t frameIndex)
{
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();

	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	UniformBufferObject ubo{};

	ubo.model = glm::rotate(glm::mat4(1.0f),time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.projection = glm::perspective(glm::radians(45.0f), static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height), 0.1f, 10.0f);
	ubo.projection[1][1] *= -1;
	memcpy(uniformBuffersMapped[frameIndex], &ubo, sizeof(ubo));
}

std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> TriangleApp::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties)
{
	// create a buffer for some data
	vk::BufferCreateInfo bufferInfo
	{
		.size = size,
		.usage = usage,
		.sharingMode = vk::SharingMode::eExclusive
	};
	vk::raii::Buffer buffer(device, bufferInfo);

	// allocate device memory for that buffer
	vk::MemoryRequirements memoryRequirements = buffer.getMemoryRequirements();
	uint32_t memoryType = findMemoryType(memoryRequirements.memoryTypeBits, properties);
	vk::MemoryAllocateInfo memoryAllocateInfo
	{
		.allocationSize = memoryRequirements.size,
		.memoryTypeIndex = memoryType
	};
	vk::raii::DeviceMemory bufferMemory(device, memoryAllocateInfo);
	buffer.bindMemory(*bufferMemory, 0);
	return { std::move(buffer), std::move(bufferMemory) };
}

std::pair<vk::raii::Image, vk::raii::DeviceMemory> TriangleApp::createImage(uint32_t width,uint32_t height, uint32_t mipLevels, vk::Format format,
	vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties)
{
	vk::ImageCreateInfo imageInfo
	{
		.imageType = vk::ImageType::e2D,
		.format = format,
		.extent = {width, height, 1},
		.mipLevels = mipLevels,
		.arrayLayers = 1,
		.samples = vk::SampleCountFlagBits::e1,
		.tiling = tiling,
		.usage = usage,
		.sharingMode = vk::SharingMode::eExclusive
	};

	vk::raii::Image image(device, imageInfo);

	vk::MemoryRequirements memoryRequirements = image.getMemoryRequirements();
	uint32_t memoryType = findMemoryType(memoryRequirements.memoryTypeBits, properties);
	vk::MemoryAllocateInfo memoryAllocateInfo
	{
		.allocationSize = memoryRequirements.size,
		.memoryTypeIndex = memoryType
	};
	vk::raii::DeviceMemory imageMemory(device, memoryAllocateInfo);
	image.bindMemory(*imageMemory, 0);
	return { std::move(image), std::move(imageMemory) };
}

void TriangleApp::copyBuffer(vk::raii::Buffer& srcBuffer, vk::raii::Buffer& dstBuffer, vk::DeviceSize size)
{
	/*vk::CommandBufferAllocateInfo allocInfo
	{
		.commandPool = commandPool,
		.level = vk::CommandBufferLevel::ePrimary,
		.commandBufferCount = 1,
	};*/

	auto commandCopyBuffer = beginSingleTimeCommands();
	
	//commandCopyBuffer.begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

	commandCopyBuffer->copyBuffer(*srcBuffer, *dstBuffer, vk::BufferCopy(0, 0, size));

	endSingleTimeCommands(*commandCopyBuffer);

	/*commandCopyBuffer.end();

	vk::SubmitInfo submInfo
	{
		.commandBufferCount = 1,
		.pCommandBuffers = &*commandCopyBuffer
	};
	graphicsQueue.submit(submInfo, nullptr);
	graphicsQueue.waitIdle();*/
}

std::unique_ptr<vk::raii::CommandBuffer> TriangleApp::beginSingleTimeCommands()
{
	vk::CommandBufferAllocateInfo allocInfo
	{
		.commandPool = commandPool,
		.level = vk::CommandBufferLevel::ePrimary,
		.commandBufferCount = 1,
	};
	std::unique_ptr<vk::raii::CommandBuffer> commandBuffer = std::make_unique<vk::raii::CommandBuffer>(std::move(device.allocateCommandBuffers(allocInfo).front()));

	commandBuffer->begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

	return commandBuffer;
}
void TriangleApp::endSingleTimeCommands(const vk::raii::CommandBuffer& commandBuffer)
{
	commandBuffer.end();
	vk::SubmitInfo submInfo
	{
		.commandBufferCount = 1,
		.pCommandBuffers = &*commandBuffer
	};
	graphicsQueue.submit(submInfo, nullptr);
	graphicsQueue.waitIdle();
}

uint32_t TriangleApp::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
{
	vk::PhysicalDeviceMemoryProperties memoryProperties = physicalDevice.getMemoryProperties();

	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
	{
		if ((typeFilter & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}
	throw std::runtime_error("failed to find suitable memory type for vertex buffer");
}


void TriangleApp::createSyncObjects()
{
	assert(renderFinishedSemaphores.empty() && presentCompleteSemaphores.empty() && drawFences.empty());

	for (uint32_t i = 0; i < swapChainImages.size(); i++)
	{
		renderFinishedSemaphores.emplace_back(vk::raii::Semaphore(device, vk::SemaphoreCreateInfo()));
	}
	for (uint32_t i = 0; i < IN_FLIGHT_FRAMES; i++)
	{
		presentCompleteSemaphores.emplace_back(vk::raii::Semaphore(device, vk::SemaphoreCreateInfo()));
		drawFences.emplace_back(vk::raii::Fence(device, { .flags = vk::FenceCreateFlagBits::eSignaled }));
	}

}

void TriangleApp::recreateSwapChain()
{
	int width = 0, height = 0;
	glfwGetFramebufferSize(window, &width, &height);
	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(window, &width, &height);
		glfwPollEvents();
	}
	device.waitIdle();

	cleanupSwapChain();

	createSwapChain();
	createImageViews();
	
	createDepthResources();
}

void TriangleApp::cleanupSwapChain()
{
	swapChainImageViews.clear();
	swapChain = nullptr;

}

void TriangleApp::framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	auto app = reinterpret_cast<TriangleApp*>(glfwGetWindowUserPointer(window));
	app->frameBufferResized = true;
}

void TriangleApp::transition_image_layout(
	vk::Image image,
	vk::ImageLayout old_layout,
	vk::ImageLayout new_layout,
	vk::AccessFlags2 src_access_mask,
	vk::AccessFlags2 dst_access_mask,
	vk::PipelineStageFlags2 src_stage_mask,
	vk::PipelineStageFlags2 dst_stage_mask,
	vk::ImageAspectFlagBits image_aspect_flags)
{
	vk::ImageMemoryBarrier2 barrier =
	{
		.srcStageMask = src_stage_mask,
		.srcAccessMask = src_access_mask,
		.dstStageMask = dst_stage_mask,
		.dstAccessMask = dst_access_mask,
		.oldLayout = old_layout,
		.newLayout = new_layout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange = 
		{
			.aspectMask = image_aspect_flags,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		}
	};
	vk::DependencyInfo dependencyInfo =
	{
		.dependencyFlags = {},
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &barrier
	};

	commandBuffers[frameIndex].pipelineBarrier2(dependencyInfo);
}

void TriangleApp::copyBufferToImage(const vk::raii::Buffer& buffer, vk::raii::Image& image, uint32_t width, uint32_t height)
{
	const auto commandBuffer = beginSingleTimeCommands();
	vk::BufferImageCopy region
	{
		.bufferOffset = 0,
		.bufferRowLength = 0,
		.bufferImageHeight = 0,
		.imageSubresource =
		{
			.aspectMask = vk::ImageAspectFlagBits::eColor,
			.mipLevel = 0,
			.baseArrayLayer = 0,
			.layerCount = 1
		},
		.imageOffset = {0,0,0},
		.imageExtent = {width, height, 1}
	};
	commandBuffer->copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, region);
	endSingleTimeCommands(*commandBuffer);
}

void TriangleApp::transitionImageLayout(const vk::raii::Image& image, const vk::ImageLayout& oldLayout, const vk::ImageLayout& newLayout, uint32_t mipLevels)
{
	const auto commandBuffer = beginSingleTimeCommands();
	vk::ImageMemoryBarrier barrier =
	{
		.oldLayout = oldLayout,
		.newLayout = newLayout,
		.srcQueueFamilyIndex = vk::QueueFamilyIgnored,
		.dstQueueFamilyIndex = vk::QueueFamilyIgnored,
		.image = image,
		.subresourceRange =
		{
			.aspectMask = vk::ImageAspectFlagBits::eColor,
			.levelCount = mipLevels,
			.layerCount = 1
		}

	};
	vk::PipelineStageFlags srcStageMask;
	vk::PipelineStageFlags dstStageMask;
	if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
	{
		barrier.srcAccessMask = {};
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

		srcStageMask = vk::PipelineStageFlagBits::eTopOfPipe;
		dstStageMask = vk::PipelineStageFlagBits::eTransfer;
	}
	else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
	{
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

		srcStageMask = vk::PipelineStageFlagBits::eTransfer;
		dstStageMask = vk::PipelineStageFlagBits::eFragmentShader;
	}
	else
		throw std::invalid_argument("unsupported layout transition!");
	commandBuffer->pipelineBarrier(srcStageMask, dstStageMask, {}, {}, {}, barrier);
	endSingleTimeCommands(*commandBuffer);
}

void TriangleApp::recordCommandBuffer(uint32_t imageIndex)
{
	auto& commandBuffer = commandBuffers[frameIndex];
	commandBuffer.begin({});

	transition_image_layout(
		swapChainImages[imageIndex],
		vk::ImageLayout::eUndefined,
		vk::ImageLayout::eColorAttachmentOptimal,
		{},
		vk::AccessFlagBits2::eColorAttachmentWrite,
		vk::PipelineStageFlagBits2::eColorAttachmentOutput,
		vk::PipelineStageFlagBits2::eColorAttachmentOutput,
		vk::ImageAspectFlagBits::eColor
	);

	transition_image_layout(
		*depthImage,
		vk::ImageLayout::eUndefined,
		vk::ImageLayout::eDepthAttachmentOptimal,
		vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
		vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
		vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
		vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
		vk::ImageAspectFlagBits::eDepth
	);

	vk::ClearValue clearColour = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
	vk::ClearValue clearDepth =  vk::ClearDepthStencilValue(1.0f, 0);
	vk::RenderingAttachmentInfo attachmentInfo =
	{
		.imageView = swapChainImageViews[imageIndex],
		.imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
		.loadOp = vk::AttachmentLoadOp::eClear,
		.storeOp = vk::AttachmentStoreOp::eStore,
		.clearValue = clearColour
	};

	vk::RenderingAttachmentInfo depthAttachmentInfo
	{
		.imageView = depthImageView,
		.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
		.loadOp = vk::AttachmentLoadOp::eClear,
		.storeOp = vk::AttachmentStoreOp::eDontCare,
		.clearValue = clearDepth
	};

	vk::RenderingInfo renderingInfo =
	{
		.renderArea = {.offset = {0,0}, .extent = swapChainExtent},
		.layerCount = 1,
		.colorAttachmentCount = 1,
		.pColorAttachments = &attachmentInfo,
		.pDepthAttachment = &depthAttachmentInfo
	};

	commandBuffer.beginRendering(renderingInfo);

	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);

	commandBuffer.bindVertexBuffers(0, *vertexBuffer, {0});
	commandBuffer.bindIndexBuffer(*indexBuffer, 0, vk::IndexTypeValue<decltype(indices)::value_type>::value);
	
	commandBuffer.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height), 0.0f, 1.0f));
	commandBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChainExtent));
	
	//commandBuffer.draw(static_cast<uint32_t>(vertices.size()), 1, 0, 0);
	commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, *descriptorSets[frameIndex], nullptr);
	commandBuffer.drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
	commandBuffer.endRendering();

	transition_image_layout(
		swapChainImages[imageIndex],
		vk::ImageLayout::eColorAttachmentOptimal,
		vk::ImageLayout::ePresentSrcKHR,
		vk::AccessFlagBits2::eColorAttachmentWrite,
		{},
		vk::PipelineStageFlagBits2::eColorAttachmentOutput,
		vk::PipelineStageFlagBits2::eBottomOfPipe,
		vk::ImageAspectFlagBits::eColor
	);

	commandBuffer.end();
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


void TriangleApp::drawFrame()
{
	// wait Fence -> acquireNextImage -> resetFence -> resetBuffer -> recordBuffer
	// -> set up submitInfo with our buffer -> submit the work to queue -> set up presentInfo
	// -> queue the image with imageIndexInfo for presentation to surface's platform
	auto fenceResult = device.waitForFences(*drawFences[frameIndex], vk::True, UINT64_MAX);
	if (fenceResult != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to wait for fence!");
	}

	auto [result, imageIndex] = swapChain.acquireNextImage(UINT64_MAX, *presentCompleteSemaphores[frameIndex], nullptr);

	if (result == vk::Result::eErrorOutOfDateKHR)
	{
		recreateSwapChain();
		return;
	}
	if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
	{
		assert(result == vk::Result::eTimeout || result == vk::Result::eNotReady);
		throw std::runtime_error("failed to acquire swap chain image");
	}

	device.resetFences(*drawFences[frameIndex]);
	commandBuffers[frameIndex].reset();
	recordCommandBuffer(imageIndex);

	updateUniformBuffer(frameIndex);

	vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
	const vk::SubmitInfo submitInfo
	{
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &*presentCompleteSemaphores[frameIndex],
		.pWaitDstStageMask = &waitDestinationStageMask,
		.commandBufferCount = 1,
		.pCommandBuffers = &*commandBuffers[frameIndex],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &*renderFinishedSemaphores[imageIndex]
	};

	graphicsQueue.submit(submitInfo, *drawFences[frameIndex]);

	const vk::PresentInfoKHR presentInfo
	{
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &*renderFinishedSemaphores[imageIndex],
		.swapchainCount = 1,
		.pSwapchains = &*swapChain,
		.pImageIndices = &imageIndex
	};

	result = graphicsQueue.presentKHR(presentInfo);

	if ((result == vk::Result::eSuboptimalKHR) || (result == vk::Result::eErrorOutOfDateKHR) || frameBufferResized)
	{
		frameBufferResized = false;
		recreateSwapChain();
	}
	else
	{
		assert(result == vk::Result::eSuccess);
	}
	frameIndex = (frameIndex + 1) % IN_FLIGHT_FRAMES;
}




