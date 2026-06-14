#include "VulkanSwapChain.h"


VulkanSwapChain::VulkanSwapChain(VulkanDevice& device)
	: m_vkDevice(device)
{
	createSwapChain();
	createImageViews();
	createSyncObjects();
}

VulkanSwapChain::VulkanSwapChain(VulkanDevice& device, std::shared_ptr<VulkanSwapChain> previousSwapChain)
	: m_vkDevice(device), m_oldSwapChain(previousSwapChain)
{
	createSwapChain();
	createImageViews();
	createSyncObjects();
	m_oldSwapChain = nullptr;
}

VulkanSwapChain::~VulkanSwapChain()
{
	cleanupSwapChain();
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
	{
		vkDestroySemaphore(m_vkDevice.getDevice(), m_renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(m_vkDevice.getDevice(), m_imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(m_vkDevice.getDevice(), m_inFlightFences[i], nullptr);
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
	swapChainCreateInfo.surface = m_vkDevice.getSurface();
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
	swapChainCreateInfo.oldSwapchain = m_oldSwapChain == nullptr ? VK_NULL_HANDLE : m_oldSwapChain->m_vkSwapChain;

	if (vkCreateSwapchainKHR(m_vkDevice.getDevice(), &swapChainCreateInfo, nullptr, &m_vkSwapChain) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Vulkan swapchain");
	}

	//retrieving handles of Vkimages from the swapchain
	vkGetSwapchainImagesKHR(m_vkDevice.getDevice(), m_vkSwapChain, &imageCount, nullptr);
	m_swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(m_vkDevice.getDevice(), m_vkSwapChain, &imageCount, m_swapChainImages.data());
}

void VulkanSwapChain::cleanupSwapChain() 
{
	for (auto& imageView : m_swapChainImageViews)
	{
		vkDestroyImageView(m_vkDevice.getDevice(), imageView, nullptr);
	}
	vkDestroySwapchainKHR(m_vkDevice.getDevice(), m_vkSwapChain, nullptr);
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
		vkImgVCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		vkImgVCreateInfo.format = m_swapChainFormat;
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

void VulkanSwapChain::createSyncObjects() 
{
	m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	VkSemaphoreCreateInfo semCreateInfo = {};
	semCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	
	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
	{
		if (vkCreateSemaphore(m_vkDevice.getDevice(), &semCreateInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(m_vkDevice.getDevice(), &semCreateInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(m_vkDevice.getDevice(), &fenceCreateInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create Vulkan semaphores or fence");
		}
	}
}

unsigned int VulkanSwapChain::getSwapChainWidth() const 
{
	return m_extent.width;
}

unsigned int VulkanSwapChain::getSwapChainHeight() const
{
	return m_extent.height;
}

VkExtent2D VulkanSwapChain::getSwapChainExtent() const 
{
	return m_extent;
}

VkImageView VulkanSwapChain::getSwapChainImageView(int index) const 
{
	return m_swapChainImageViews[index];
}

VkFormat VulkanSwapChain::getSwapChainFormat() const 
{
	return m_swapChainFormat;
}

VkResult VulkanSwapChain::acquireNextImage(uint32_t* imageIndex) 
{
	vkWaitForFences(m_vkDevice.getDevice(), 1, &m_inFlightFences[m_currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
	VkResult result = vkAcquireNextImageKHR(m_vkDevice.getDevice(), m_vkSwapChain, std::numeric_limits<uint64_t>::max(), 
		m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, imageIndex);
	return result;
}

VkResult VulkanSwapChain::submitCommandBuffers(const VkCommandBuffer* commandBuffers, uint32_t* imageIndex) 
{
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = {m_imageAvailableSemaphores[m_currentFrame]};
	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = commandBuffers;
	VkSemaphore signalSemaphores[] = {m_renderFinishedSemaphores[m_currentFrame]};
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkResetFences(m_vkDevice.getDevice(), 1, &m_inFlightFences[m_currentFrame]);
	if (vkQueueSubmit(m_vkDevice.getGraphicsQueue(), 1, &submitInfo, m_inFlightFences[m_currentFrame]) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to submit Vulkan command buffer");
	}

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChain[] = {m_vkSwapChain};
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChain;

	presentInfo.pImageIndices = imageIndex;

	VkResult result = vkQueuePresentKHR(m_vkDevice.getPresentQueue(), &presentInfo);

	m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

	return result;
}