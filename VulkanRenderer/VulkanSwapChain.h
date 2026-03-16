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
	static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

	VulkanSwapChain(const VulkanSwapChain&) = delete;
	VulkanSwapChain& operator=(const VulkanSwapChain&) = delete;

	VulkanSwapChain(VulkanDevice& device);

	~VulkanSwapChain();

	unsigned int getSwapChainWidth() const;
	unsigned int getSwapChainHeight() const;
	VkExtent2D getSwapChainExtent() const;
	VkFramebuffer getFramebuffer(int index) const;

	VkRenderPass getRenderPass();

	VkResult acquireNextImage(uint32_t* imageIndex);
	VkResult submitCommandBuffers(const VkCommandBuffer* commandBuffers, uint32_t* imageIndex);

private:

	void createSwapChain();
	void createImageViews();
	void createRenderPass();
	void createFramebuffers();
	void createSyncObjects();

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
	std::vector<VkFramebuffer> m_framebuffers;

	std::vector<VkSemaphore> m_imageAvailableSemaphores;
	std::vector<VkSemaphore> m_renderFinishedSemaphores;
	std::vector<VkFence> m_imagesInFlight;
	std::vector<VkFence> m_inFlightFences;

	size_t m_currentFrame = 0;
};