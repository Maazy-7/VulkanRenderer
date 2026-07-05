#pragma once

#include <vector>
#include <limits>
#include <fstream>

#include "VulkanDevice.h"
#include "VulkanImage.h"


class VulkanSwapChain
{
public:
	static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

	VulkanSwapChain(const VulkanSwapChain&) = delete;
	VulkanSwapChain& operator=(const VulkanSwapChain&) = delete;

	VulkanSwapChain(VulkanDevice& device);
	VulkanSwapChain(VulkanDevice& device, std::shared_ptr<VulkanSwapChain> previousSwapChain);

	~VulkanSwapChain();

	unsigned int getSwapChainWidth() const;
	unsigned int getSwapChainHeight() const;
	size_t getCurrentFrameIndex() const;
	VkExtent2D getSwapChainExtent() const;
	VkImageView getSwapChainImageView(int index) const;
	VkImageView getSwapChainDepthBufferView() const;
	VkImage getSwapChainImage(int index) const;
	VkImage getSwapChainDepthBuffer() const;
	VkFormat getSwapChainFormat() const;

	VkResult acquireNextImage(uint32_t* imageIndex);
	VkResult submitCommandBuffers(const VkCommandBuffer* commandBuffers, uint32_t* imageIndex);

private:

	void createSwapChain();
	void createImageViews();
	void createDepthBuffer();
	void createSyncObjects();
	void cleanupSwapChain();

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& surfaceFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& presentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilites);
	
	VkSwapchainKHR m_vkSwapChain;
	std::shared_ptr<VulkanSwapChain> m_oldSwapChain;
	VulkanDevice& m_vkDevice;
	VkFormat m_swapChainFormat;
	VkExtent2D m_extent;

	std::vector<VkImage> m_swapChainImages;//could also make these a VulkanImage
	std::vector<VkImageView> m_swapChainImageViews;
	std::unique_ptr<VulkanImage> m_depthBuffer;

	std::vector<VkSemaphore> m_imageAvailableSemaphores;
	std::vector<VkSemaphore> m_renderFinishedSemaphores;
	std::vector<VkFence> m_imagesInFlight;
	std::vector<VkFence> m_inFlightFences;

	size_t m_currentFrame = 0;
};