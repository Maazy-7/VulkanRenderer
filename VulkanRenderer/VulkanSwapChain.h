#pragma once

#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>
#include <vector>
#include <limits>
#include <fstream>

#include "VulkanDevice.h"


class VulkanSwapChain
{
public:

	VulkanSwapChain(const VulkanSwapChain&) = delete;
	VulkanSwapChain& operator=(const VulkanSwapChain&) = delete;

	VulkanSwapChain(VulkanDevice& device);

	~VulkanSwapChain();

	unsigned int getSwapChainWidth() const;
	unsigned int getSwapChainHeight() const;

	//temp
	VkRenderPass& getRenderPass();

private:

	void createSwapChain();
	void createImageViews();
	void createRenderPass();

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& surfaceFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& presentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilites);
	
	VkSwapchainKHR m_vkSwapChain;
	VulkanDevice& m_vkDevice;
	VkFormat m_swapChainFormat;
	VkExtent2D m_extent;

	VkRenderPass m_vkRenderPass;

	std::vector<VkImage> m_swapChainImages;
	std::vector<VkImageView> m_swapChainImageViews;
};