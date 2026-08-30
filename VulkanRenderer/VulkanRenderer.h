#pragma once

#include <vector>
#include <memory>

#include "Window.h"
#include "VulkanDevice.h"
#include "VulkanSwapChain.h"
#include "VulkanPipeline.h"


class VulkanRenderer
{
public:

	VulkanRenderer(const VulkanRenderer&) = delete;
	VulkanRenderer& operator=(const VulkanRenderer&) = delete;

	VulkanRenderer(Window& window, VulkanDevice& device);
	~VulkanRenderer();

	VkCommandBuffer getCurrentCommandBuffer() const;
	const VulkanSwapChain& getSwapChain() const;

	VkCommandBuffer beginFrame();
	void endFrame();
	void beginRendering(VkCommandBuffer commandBuffer);
	void endRendering(VkCommandBuffer commandBuffer);
	void blitVkImageToSwapChain(VkCommandBuffer commandBuffer, const VulkanImage& srcImage);

private:

	void createCommandBuffer();
	void recreateSwapChain();

	Window& m_window;
	VulkanDevice& m_vkDevice;
	std::unique_ptr<VulkanSwapChain> m_vkSwapChain;
	std::vector<VkCommandBuffer> m_vkCommandBuffers;
	uint32_t m_currentImageIndex = 0;
	
	int m_currentFrameIndex = 0;
	bool m_isFrameStarted = false;
};

