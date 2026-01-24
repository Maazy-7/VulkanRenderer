#include "VulkanDevice.h"

VulkanDevice::VulkanDevice(Window& window) 
	:m_window(window)
{
	createVkInstance();
	createSurface();
	pickPhysicalDevice();
	createDevice();
	createCommandBuffer();
}

VulkanDevice::~VulkanDevice() 
{
	vkDestroyDevice(m_vkDevice, nullptr);
}

void VulkanDevice::createVkInstance() 
{
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "vulkan renderer";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "vulkan engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_3;

	uint32_t extCount;
	const char** exts = glfwGetRequiredInstanceExtensions(&extCount);
	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = extCount;
	createInfo.ppEnabledExtensionNames = exts;
	if (vkCreateInstance(&createInfo, nullptr, &m_vkInstance) != VK_SUCCESS) 
	{
		throw std::runtime_error("Failed to create Vulkan instance");
	}
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
		if (suitableGPU && devProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) 
		{
			m_vkPhysicalDevice = dev;
			break;
		}
		//otherwise select andy suitable gpu
		else if (suitableGPU)
		{
			//no break, will check next gpu incase it is a discrete gpu
			m_vkPhysicalDevice = dev;
		}
	}
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
		qCrInfo.pQueuePriorities = &qPriority;
		qCreateInfo.push_back(qCrInfo);
	}

	VkDeviceCreateInfo devCreateInfo = {};
	devCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	devCreateInfo.queueCreateInfoCount = (uint32_t)qCreateInfo.size();
	devCreateInfo.pQueueCreateInfos = qCreateInfo.data();
	devCreateInfo.enabledExtensionCount = static_cast<uint32_t>(m_deviceExtensions.size());
	devCreateInfo.ppEnabledExtensionNames = m_deviceExtensions.data();
	if (vkCreateDevice(m_vkPhysicalDevice, &devCreateInfo, nullptr, &m_vkDevice) != VK_SUCCESS) 
	{
		throw std::runtime_error("Failed to create Vulkan device");
	}
	vkGetDeviceQueue(m_vkDevice, qfamInds.m_graphicsQueue.value(), 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_vkDevice, qfamInds.m_presentQueue.value(), 0, &m_presentQueue);
}

void VulkanDevice::createCommandBuffer() 
{
	
}

SwapChainSupportDetails VulkanDevice::querySwapChainSupportDetails()
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
VkQueue VulkanDevice::getGraphicsQueue() const { return m_graphicsQueue; }
VkQueue VulkanDevice::getPresentQueue() const { return m_presentQueue; }
VkCommandPool VulkanDevice::getCommandPool() const { return m_vkCommandPool; }
void VulkanDevice::getWindowFrameBufferSize(int* width, int* height) const { glfwGetFramebufferSize(m_window.getWindow(), width, height); }