#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <memory>

#include "Window.h"
#include "VulkanDevice.h"
#include "VulkanSwapChain.h"
#include "VulkanPipeline.h"
#include "VulkanBuffer.h"

const std::vector<Vertex> vertices =
{
{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
};

class SimpleRenderSystem
{

public:

	SimpleRenderSystem(const SimpleRenderSystem&) = delete;
	SimpleRenderSystem& operator=(const SimpleRenderSystem&) = delete;

	SimpleRenderSystem(VulkanDevice& device, VulkanSwapChain& swapChain);

	void renderGameObjects(VkCommandBuffer commandBuffer);

private:

	void createPipeline(VulkanSwapChain& swapChain);

	VulkanDevice& m_vkDevice;
	std::unique_ptr<VulkanPipeline> m_vkPipeline;
	//temp
	VulkanBuffer vertexBuffer;
	VulkanBuffer stagingBuffer;
};

