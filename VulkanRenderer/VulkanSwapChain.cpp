#include "VulkanSwapChain.h"


VulkanSwapChain::VulkanSwapChain(VulkanDevice& device)
	: m_vkDevice(device)
{
	createSwapChain();
	createImageViews();
}

VulkanSwapChain::~VulkanSwapChain() 
{
	vkDestroySwapchainKHR(m_vkDevice.getDevice(), m_vkSwapChain, nullptr);
	for (auto& imageView : m_swapChainImageViews) 
	{
		vkDestroyImageView(m_vkDevice.getDevice(), imageView, nullptr);
	}
}

void VulkanSwapChain::createSwapChain() 
{
	//choose best format
	SwapChainSupportDetails	swapChainSupportDetails = m_vkDevice.querySwapChainSupportDetails();

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupportDetails.m_formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupportDetails.m_presentModes);
	VkExtent2D extent = chooseSwapExtent(swapChainSupportDetails.m_surfaceCapabilites);

	m_swapChainFormat = surfaceFormat.format;
	m_extent = extent;

	uint32_t imageCount = swapChainSupportDetails.m_surfaceCapabilites.minImageCount + 1;
	if (swapChainSupportDetails.m_surfaceCapabilites.maxImageCount > 0 && imageCount > swapChainSupportDetails.m_surfaceCapabilites.maxImageCount) 
	{
		imageCount = swapChainSupportDetails.m_surfaceCapabilites.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
	swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainCreateInfo.minImageCount = imageCount;
	swapChainCreateInfo.imageFormat = surfaceFormat.format;
	swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapChainCreateInfo.imageExtent = extent;
	swapChainCreateInfo.imageArrayLayers = 1;
	swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = m_vkDevice.findQueueIndices();
	uint32_t qFamIndices[] = { indices.m_graphicsQueue.value(), indices.m_presentQueue.value() };
	if (indices.m_graphicsQueue != indices.m_presentQueue) 
	{
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapChainCreateInfo.queueFamilyIndexCount = 2;
		swapChainCreateInfo.pQueueFamilyIndices = qFamIndices;
	}
	else 
	{
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapChainCreateInfo.queueFamilyIndexCount = 0;
		swapChainCreateInfo.pQueueFamilyIndices = nullptr;
	}

	swapChainCreateInfo.preTransform = swapChainSupportDetails.m_surfaceCapabilites.currentTransform;
	swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapChainCreateInfo.presentMode = presentMode;
	swapChainCreateInfo.clipped = VK_TRUE;
	swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(m_vkDevice.getDevice(), &swapChainCreateInfo, nullptr, &m_vkSwapChain) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Vulkan swapchain");
	}

	//retrieving handles of Vkimages from the swapchain
	vkGetSwapchainImagesKHR(m_vkDevice.getDevice(), m_vkSwapChain, &imageCount, nullptr);
	m_swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(m_vkDevice.getDevice(), m_vkSwapChain, &imageCount, m_swapChainImages.data());
}

VkSurfaceFormatKHR VulkanSwapChain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& surfaceFormats)
{
	for (const auto& surfaceFormat : surfaceFormats) 
	{
		if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) 
		{
			return surfaceFormat;
		}
	}

	return surfaceFormats[0];
}

VkPresentModeKHR VulkanSwapChain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& presentModes)
{
	for (const auto& presentMode : presentModes)
	{
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return presentMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanSwapChain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilites) 
{
	if (capabilites.currentExtent.width != std::numeric_limits<uint32_t>::max()) 
	{
		return capabilites.currentExtent;
	}
	else 
	{
		int width, height;
		m_vkDevice.getWindowFrameBufferSize(&width, &height);
		VkExtent2D extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
		extent.width = glm::clamp(extent.width, capabilites.minImageExtent.width, capabilites.maxImageExtent.width);
		extent.height = glm::clamp(extent.height, capabilites.minImageExtent.height, capabilites.maxImageExtent.height);
		return extent;
	}
}

void VulkanSwapChain::createImageViews() 
{
	m_swapChainImageViews.resize(m_swapChainImages.size());

	for (int i = 0; i < m_swapChainImages.size(); i++)
	{
		VkImageViewCreateInfo vkImgVCreateInfo = {};
		vkImgVCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		vkImgVCreateInfo.image = m_swapChainImages[i];
		vkImgVCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		vkImgVCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		vkImgVCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		vkImgVCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		vkImgVCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		vkImgVCreateInfo.subresourceRange.baseMipLevel = 0;
		vkImgVCreateInfo.subresourceRange.levelCount = 1;
		vkImgVCreateInfo.subresourceRange.baseArrayLayer = 0;
		vkImgVCreateInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(m_vkDevice.getDevice(), &vkImgVCreateInfo, nullptr, &m_swapChainImageViews[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create Vulkan image view");
		}
	}
}

void VulkanSwapChain::createGraphicsPipeline()
{
	auto vertexCode = readFromFile("Shaders/shader.vert.spv");
	auto fragmentCode = readFromFile("Shaders/shader.frag.spv");

	VkShaderModule vertexShaderModule = createShaderModule(vertexCode);
	VkShaderModule fragmentShaderModule = createShaderModule(fragmentCode);

	VkPipelineShaderStageCreateInfo vertexShaderCreateInfo = {};
	vertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertexShaderCreateInfo.module = vertexShaderModule;
	vertexShaderCreateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragmentShaderCreateInfo = {};
	fragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragmentShaderCreateInfo.module = fragmentShaderModule;
	fragmentShaderCreateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderCreateInfo, fragmentShaderCreateInfo };
	//
	
	vkDestroyShaderModule(m_vkDevice.getDevice(), vertexShaderModule, nullptr);
	vkDestroyShaderModule(m_vkDevice.getDevice(), fragmentShaderModule, nullptr);
}

VkShaderModule VulkanSwapChain::createShaderModule(const std::vector<char>& shaderCode) 
{
	VkShaderModuleCreateInfo shaderCreateMooduleInfo = {};
	shaderCreateMooduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderCreateMooduleInfo.codeSize = shaderCode.size();
	shaderCreateMooduleInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());
	VkShaderModule shaderModule;
	if (vkCreateShaderModule(m_vkDevice.getDevice(), &shaderCreateMooduleInfo, nullptr, &shaderModule) != VK_SUCCESS) 
	{
		throw std::runtime_error("Failed to create Vulkan shader module");
	}
	return shaderModule;
}

std::vector<char> VulkanSwapChain::readFromFile(const std::string& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);
	if (!file.is_open()) 
	{
		throw std::runtime_error("Failed to open shader file");
	}
	size_t fileSize = (uint32_t)file.tellg();
	std::vector<char> buffer(fileSize);
	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();
	return buffer;
}