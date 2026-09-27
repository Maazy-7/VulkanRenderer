#include "VulkanDevice.h"

VulkanDevice::VulkanDevice(Window& window) 
	:m_window(window)
{
	createVkInstance();
	setupDebugMessenger();
	createSurface();
	pickPhysicalDevice();
	createDevice();
	createCommandPool();
	createSamplers();
}

VulkanDevice::VulkanDevice(Window& window, bool useIgpu)
	:m_window(window), m_useIgpu(useIgpu)
{
	createVkInstance();
	setupDebugMessenger();
	createSurface();
	pickPhysicalDevice();
	createDevice();
	createCommandPool();
	createSamplers();
}

VulkanDevice::~VulkanDevice() 
{
	if (enableValidationLayers) 
	{
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_vkInstance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr) 
		{
			func(m_vkInstance, m_vkDebugMessenger, nullptr);
		}
	}
	for (auto& pair : m_shaderCache) 
	{
		vkDestroyShaderModule(m_vkDevice, pair.second, nullptr);
	}
	for (VkSampler& sampler : m_vkSamplers) 
	{
		vkDestroySampler(m_vkDevice, sampler, nullptr);
	}
	vkDestroyCommandPool(m_vkDevice, m_vkCommandPool, nullptr);
	vkDestroyDevice(m_vkDevice, nullptr);
}

void VulkanDevice::createVkInstance() 
{
	if (enableValidationLayers && !checkValidationLayerSupport()) 
	{
		throw std::runtime_error("validation layers requested but not available");
	}
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "vulkan renderer";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "vulkan engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_3;

	auto exts = getRequiredExtensions();
	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = (uint32_t)exts.size();
	createInfo.ppEnabledExtensionNames = exts.data();
	
	if (enableValidationLayers) 
	{
		createInfo.ppEnabledLayerNames = validationLayers.data();
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
	}
	else { createInfo.enabledLayerCount = 0; }
	
	if (vkCreateInstance(&createInfo, nullptr, &m_vkInstance) != VK_SUCCESS) 
	{
		throw std::runtime_error("Failed to create Vulkan instance");
	}
}

void VulkanDevice::setupDebugMessenger() 
{
	if (!enableValidationLayers) { return; }
	VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo = {};
	debugMessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debugMessengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | 
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debugMessengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debugMessengerCreateInfo.pfnUserCallback = debugCallback;
	debugMessengerCreateInfo.pUserData = nullptr;
	
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_vkInstance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		if (func(m_vkInstance, &debugMessengerCreateInfo, nullptr, &m_vkDebugMessenger) != VK_SUCCESS) 
		{
			throw std::runtime_error("Failed to create debug");
		}
	}
	else 
	{
		std::cout << "extension not present" << "\n";
	}
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDevice::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	std::cerr << "\nvalidation layer: " << pCallbackData->pMessage << "\n";
	return VK_FALSE;
}

bool VulkanDevice::checkValidationLayerSupport() const
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : validationLayers) 
	{
		bool layerFound = false;
		for (auto& layerProperties : availableLayers) 
		{
			if (strcmp(layerName, layerProperties.layerName) == 0) { layerFound = true; break; }
		}
		if (!layerFound) 
		{
			return false;
		}
	}
	return true;
}

std::vector<const char*> VulkanDevice::getRequiredExtensions() const 
{
	uint32_t extCount;
	const char** exts = glfwGetRequiredInstanceExtensions(&extCount);
	std::vector<const char*> extensions(exts, exts + extCount);

	if (enableValidationLayers) { extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME); }
	return extensions;
}

void VulkanDevice::createSurface()
{
	if (glfwCreateWindowSurface(m_vkInstance, m_window.getWindow(), nullptr, &m_vkSurfaceKHR) != VK_SUCCESS) 
	{
		throw std::runtime_error("Failed to create Vulkan surface");
	}
}

QueueFamilyIndices VulkanDevice::findQueueIndices() 
{
	return findQueueIndices(m_vkPhysicalDevice);
}

VkFormat VulkanDevice::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling imgTiling, VkFormatFeatureFlags2 formatFeatureFlags,
	std::string errorMsg) const
{
	for (VkFormat format : candidates) 
	{
		VkFormatProperties2 formatProperties;
		formatProperties.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2;
		formatProperties.pNext = nullptr;
		vkGetPhysicalDeviceFormatProperties2(m_vkPhysicalDevice, format, &formatProperties);

		VkFormatFeatureFlags2 flags = formatProperties.formatProperties.optimalTilingFeatures & formatFeatureFlags;
		if (imgTiling == VK_IMAGE_TILING_LINEAR && (formatProperties.formatProperties.linearTilingFeatures & formatFeatureFlags) == formatFeatureFlags) 
		{
			return format;
		}
		else if (imgTiling == VK_IMAGE_TILING_OPTIMAL && flags == formatFeatureFlags) 
		{
			return format;
		}
	}

	throw std::runtime_error(errorMsg);
}

VkFormat VulkanDevice::findDepthFormat() const
{
	return findSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

QueueFamilyIndices VulkanDevice::findQueueIndices(VkPhysicalDevice device) 
{
	uint32_t qFamPropertyCount;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &qFamPropertyCount, nullptr);
	std::vector<VkQueueFamilyProperties> qFamProperties(qFamPropertyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &qFamPropertyCount, qFamProperties.data());
	
	QueueFamilyIndices res;
	for (uint32_t i = 0; i < qFamPropertyCount; i++) 
	{
		if (qFamProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) { res.m_graphicsQueue = i; }
		VkBool32 presRes;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_vkSurfaceKHR, &presRes);
		if (presRes) { res.m_presentQueue = i; }
		if (res.isComplete()) { break; }
	}
	return res;
}

void VulkanDevice::pickPhysicalDevice() 
{
	uint32_t devCount;
	vkEnumeratePhysicalDevices(m_vkInstance, &devCount, nullptr);
	std::vector<VkPhysicalDevice> devices(devCount);
	vkEnumeratePhysicalDevices(m_vkInstance, &devCount, devices.data());

	VkPhysicalDeviceProperties devProperties;//used to pick the discrete gpu if found
	for (VkPhysicalDevice dev : devices) 
	{
		vkGetPhysicalDeviceProperties(dev, &devProperties);
		//best case: to select suitable device that is also the discrete gpu
		bool suitableGPU = findQueueIndices(dev).isComplete();
		
		if (m_useIgpu && suitableGPU) 
		{
			if (devProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) 
			{
				m_vkPhysicalDevice = dev;
				m_vkPhysicalDeviceProperties = devProperties;
				break;
			}
			continue;
		}
		if (suitableGPU && devProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) 
		{
			m_vkPhysicalDevice = dev;
			m_vkPhysicalDeviceProperties = devProperties;
			break;
		}
		//otherwise select any suitable gpu
		else if (suitableGPU)
		{
			//no break, will check next gpu incase it is a discrete gpu
			m_vkPhysicalDevice = dev;
			m_vkPhysicalDeviceProperties = devProperties;
		}
	}
	LOG("GPU: " << devProperties.deviceName);
	if (!m_vkPhysicalDevice) { throw std::runtime_error("Failed to find suitable GPU"); }
}

void VulkanDevice::createDevice() 
{
	QueueFamilyIndices qfamInds = findQueueIndices();
	float qPriority = 1.f;
	//set ensures same queue index isn't iterated over twice
	std::set<uint32_t> queueFamilies = {qfamInds.m_graphicsQueue.value(), qfamInds.m_presentQueue.value()};
	std::vector<VkDeviceQueueCreateInfo> qCreateInfo;
	for (uint32_t f : queueFamilies) 
	{
		VkDeviceQueueCreateInfo qCrInfo = {};
		qCrInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		qCrInfo.queueCount = 1;
		qCrInfo.queueFamilyIndex = f;
		qCrInfo.pQueuePriorities = &qPriority;
		qCreateInfo.push_back(qCrInfo);
	}

	//enabling buffer device address and descriptor indexing
	VkPhysicalDeviceVulkan12Features vkDevice12Features{};
	vkDevice12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	vkDevice12Features.bufferDeviceAddress = VK_TRUE;
	vkDevice12Features.runtimeDescriptorArray = VK_TRUE;
	vkDevice12Features.descriptorBindingVariableDescriptorCount = VK_TRUE;
	vkDevice12Features.descriptorBindingPartiallyBound = VK_TRUE;
	vkDevice12Features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
	vkDevice12Features.shaderUniformBufferArrayNonUniformIndexing = VK_TRUE;
	vkDevice12Features.shaderStorageBufferArrayNonUniformIndexing = VK_TRUE;
	vkDevice12Features.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
	vkDevice12Features.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
	vkDevice12Features.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;

	//enabling dynamic rendering and synchronization2
	VkPhysicalDeviceVulkan13Features vkDevice13Features = {};
	vkDevice13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	vkDevice13Features.dynamicRendering = VK_TRUE;
	vkDevice13Features.synchronization2 = VK_TRUE;
	vkDevice13Features.pNext = &vkDevice12Features;

	//enabling anisotropic filtering
	VkPhysicalDeviceFeatures physicalDeviceFeatures{};
	physicalDeviceFeatures.samplerAnisotropy = VK_TRUE;

	VkPhysicalDeviceFeatures2 physicalDeviceFeatures2{};
	physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	physicalDeviceFeatures2.features = physicalDeviceFeatures;
	physicalDeviceFeatures2.pNext = &vkDevice13Features;

	VkPhysicalDeviceMultiviewFeatures physicalDeviceMultiviewFeatures{};
	physicalDeviceMultiviewFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES;
	physicalDeviceMultiviewFeatures.multiview = VK_TRUE;
	physicalDeviceMultiviewFeatures.pNext = &physicalDeviceFeatures2;

	//checking and enabling ray tracing features
	VkPhysicalDeviceAccelerationStructurePropertiesKHR accelerationStructureProperties{};
	accelerationStructureProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;

	VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtPipelineProperties{};
	rtPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
	rtPipelineProperties.pNext = &accelerationStructureProperties;

	VkPhysicalDeviceProperties2 physcialDeviceProperties2{};
	physcialDeviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	physcialDeviceProperties2.pNext = &rtPipelineProperties;
	m_vkPhysicalDeviceRTPiplineProperties = rtPipelineProperties;
	m_vkPhysicalDeviceAccelerationProperties = accelerationStructureProperties;

	vkGetPhysicalDeviceProperties2(m_vkPhysicalDevice, &physcialDeviceProperties2);

	VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtPipelineFeatures{};
	rtPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;

	VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};
	accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
	accelerationStructureFeatures.pNext = &rtPipelineFeatures;

	VkPhysicalDeviceFeatures2 physicalDeviceFeatures2_forRayTracing{};//seperate struct made for physical device features 2 for checking ray tracing capability
	physicalDeviceFeatures2_forRayTracing.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	physicalDeviceFeatures2_forRayTracing.pNext = &accelerationStructureFeatures;

	vkGetPhysicalDeviceFeatures2(m_vkPhysicalDevice, &physicalDeviceFeatures2_forRayTracing);

	//enable ray tracing if supported and link chains to ones use in device create info
	if (rtPipelineFeatures.rayTracingPipeline == VK_TRUE || accelerationStructureFeatures.accelerationStructure == VK_TRUE)
	{
		m_deviceExtensions.emplace_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
		m_deviceExtensions.emplace_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
		m_deviceExtensions.emplace_back(VK_KHR_RAY_QUERY_EXTENSION_NAME);
		m_deviceExtensions.emplace_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);

		VkPhysicalDeviceRayTracingPipelineFeaturesKHR enabledRayTracingPipelineFeatures{};
		enabledRayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
		enabledRayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;
		
		VkPhysicalDeviceAccelerationStructureFeaturesKHR enabledAccelerationStructureFeatures{};
		enabledAccelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
		enabledAccelerationStructureFeatures.accelerationStructure = VK_TRUE;
		enabledAccelerationStructureFeatures.pNext = &enabledRayTracingPipelineFeatures;

		//changing pNext chain to include ray tracing, multiview -> accel structs -> rt pipeline -> vulkan_1_3 features -> ...
		physicalDeviceMultiviewFeatures.pNext = &enabledAccelerationStructureFeatures;
		enabledRayTracingPipelineFeatures.pNext = &physicalDeviceFeatures2;
	}
	//current chain
	//createinfo -> multiview -> (if ray tracing then rt structs) -> physical device features 2 -> vkdevice 13 features -> vkdevice 12 features

	VkDeviceCreateInfo devCreateInfo = {};
	devCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	devCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(qCreateInfo.size());
	devCreateInfo.pQueueCreateInfos = qCreateInfo.data();
	devCreateInfo.enabledExtensionCount = static_cast<uint32_t>(m_deviceExtensions.size());
	devCreateInfo.ppEnabledExtensionNames = m_deviceExtensions.data();
	devCreateInfo.pNext = &physicalDeviceMultiviewFeatures;
	if (vkCreateDevice(m_vkPhysicalDevice, &devCreateInfo, nullptr, &m_vkDevice) != VK_SUCCESS) 
	{
		throw std::runtime_error("Failed to create Vulkan device");
	}
	vkGetDeviceQueue(m_vkDevice, qfamInds.m_graphicsQueue.value(), 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_vkDevice, qfamInds.m_presentQueue.value(), 0, &m_presentQueue);
}

void VulkanDevice::createCommandPool()
{
	QueueFamilyIndices qFamilyIndices = findQueueIndices();

	VkCommandPoolCreateInfo commandPoolCreateInfo = {};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	commandPoolCreateInfo.queueFamilyIndex = qFamilyIndices.m_graphicsQueue.value();

	if (vkCreateCommandPool(m_vkDevice, &commandPoolCreateInfo, nullptr, &m_vkCommandPool) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Vulkan command pool");
	}
	
}

void VulkanDevice::createSamplers()
{
	//default texture sampler = index 0, shadow sampler = index 1
	//default sampler for normal srgb textures
	VkSamplerCreateInfo samplerCreateInfo{};
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.anisotropyEnable = VK_TRUE;
	samplerCreateInfo.maxAnisotropy = getPhysicalDeviceProperties().limits.maxSamplerAnisotropy;
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	VkSampler defaultTextureSampler;
	if (vkCreateSampler(m_vkDevice, &samplerCreateInfo, nullptr, &defaultTextureSampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create vulkan Sampler");
	}
	m_vkSamplers.push_back(defaultTextureSampler);

	//default sampler for shadow
	VkSamplerCreateInfo shadowSamplerCreateInfo{};
	shadowSamplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	shadowSamplerCreateInfo.minFilter = VK_FILTER_LINEAR;
	shadowSamplerCreateInfo.magFilter = VK_FILTER_LINEAR;
	shadowSamplerCreateInfo.anisotropyEnable = VK_FALSE;
	shadowSamplerCreateInfo.maxAnisotropy = 1.f;
	shadowSamplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	shadowSamplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	shadowSamplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	shadowSamplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	shadowSamplerCreateInfo.compareEnable = VK_TRUE;
	shadowSamplerCreateInfo.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

	VkSampler shadowMapSampler;
	if (vkCreateSampler(m_vkDevice, &shadowSamplerCreateInfo, nullptr, &shadowMapSampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create vulkan Sampler");
	}
	m_vkSamplers.push_back(shadowMapSampler);
}

SwapChainSupportDetails VulkanDevice::querySwapChainSupportDetails() const
{
	SwapChainSupportDetails details;

	//surface capabilites
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_vkPhysicalDevice, m_vkSurfaceKHR, &details.m_surfaceCapabilites);

	//surface formats
	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_vkPhysicalDevice, m_vkSurfaceKHR, &formatCount, nullptr);
	if (formatCount != 0) 
	{
		details.m_formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(m_vkPhysicalDevice, m_vkSurfaceKHR, &formatCount, details.m_formats.data());
	}

	//surface present modes
	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_vkPhysicalDevice, m_vkSurfaceKHR, &presentModeCount, nullptr);
	if (presentModeCount != 0)
	{
		details.m_presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(m_vkPhysicalDevice, m_vkSurfaceKHR, &presentModeCount, details.m_presentModes.data());
	}

	return details;
}

VkInstance VulkanDevice::getVkInstance() const { return m_vkInstance; }
VkDevice VulkanDevice::getDevice() const { return m_vkDevice; }
VkSurfaceKHR VulkanDevice::getSurface() const { return m_vkSurfaceKHR; }
float VulkanDevice::getAspect() const { return m_window.getAspect(); }
VkQueue VulkanDevice::getGraphicsQueue() const { return m_graphicsQueue; }
VkQueue VulkanDevice::getPresentQueue() const { return m_presentQueue; }
VkCommandPool VulkanDevice::getCommandPool() const { return m_vkCommandPool; }
VkPhysicalDeviceProperties VulkanDevice::getPhysicalDeviceProperties() const { return m_vkPhysicalDeviceProperties; }
VkPhysicalDeviceRayTracingPipelinePropertiesKHR VulkanDevice::getPhysicalDeviceRTPipelineProperties() const { return m_vkPhysicalDeviceRTPiplineProperties; }
VkPhysicalDeviceAccelerationStructurePropertiesKHR VulkanDevice::getPhysicalDeviceAccelerationStructureProperties() const { return m_vkPhysicalDeviceAccelerationProperties; }
void VulkanDevice::getWindowFrameBufferSize(int* width, int* height) const { glfwGetFramebufferSize(m_window.getWindow(), width, height); }

uint32_t VulkanDevice::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) 
{
	VkPhysicalDeviceMemoryProperties physicalDevMemProperties{};
	vkGetPhysicalDeviceMemoryProperties(m_vkPhysicalDevice, &physicalDevMemProperties);

	for (uint32_t i = 0; i < physicalDevMemProperties.memoryTypeCount; i++) 
	{
		if (typeFilter & (1 << i) && (physicalDevMemProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	throw std::runtime_error("failed to find a suitable memory type");
}

VkCommandBuffer VulkanDevice::beginSingleTimeCommands() 
{
	VkCommandBufferAllocateInfo cmdBufferAllocateInfo{};
	cmdBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdBufferAllocateInfo.commandPool = m_vkCommandPool;
	cmdBufferAllocateInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	if (vkAllocateCommandBuffers(m_vkDevice, &cmdBufferAllocateInfo, &commandBuffer) != VK_SUCCESS) 
	{
		throw std::runtime_error("failed to allocate single time Vulkan command buffer");
	}
	VkCommandBufferBeginInfo cmdBufferBeginInfo{};
	cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &cmdBufferBeginInfo);

	return commandBuffer;
}

void VulkanDevice::endSingleTimeCommands(VkCommandBuffer commandBuffer) 
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(m_graphicsQueue);

	vkFreeCommandBuffers(m_vkDevice, m_vkCommandPool, 1, &commandBuffer);
}

bool VulkanDevice::getShader(const std::string& shaderPath, VkShaderModule& shaderOut)
{
	auto it = m_shaderCache.find(shaderPath);
	if (it != m_shaderCache.end()) 
	{
		shaderOut = it->second;
		return true;
	}
	else 
	{
		return false;
	}
}

void VulkanDevice::addShader(const std::string& shaderPath, VkShaderModule shader)
{
	m_shaderCache[shaderPath] = shader;
}

VkSampler VulkanDevice::getDefaultTextureSampler() const
{
	return m_vkSamplers[0];
}

VkSampler VulkanDevice::getShadowMapSampler() const
{
	return m_vkSamplers[1];
}
