#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <chrono>

#include "Window.h"
#include "VulkanDevice.h"
#include "VulkanSwapChain.h"
#include "VulkanPipeline.h"
#include "VulkanBuffer.h"

const std::vector<Vertex> vertices = {
	{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
	{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
	{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
	{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
};

const std::vector<uint16_t> indices = { 0, 1, 2, 2, 3, 0 };

struct UniformBufferObject {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

class SimpleRenderSystem
{

public:

	SimpleRenderSystem(const SimpleRenderSystem&) = delete;
	SimpleRenderSystem& operator=(const SimpleRenderSystem&) = delete;

	SimpleRenderSystem(VulkanDevice& device, const VulkanSwapChain& swapChain);

	void renderGameObjects(VkCommandBuffer commandBuffer, float dt);

private:

	void createPipeline(const VulkanSwapChain& swapChain);

	VkDeviceSize vCount = vertices.size();
	VkDeviceSize iCount = indices.size();
	VkDeviceSize vBuffSize = sizeof(vertices[0]) * vCount;
	VkDeviceSize iBuffSize = sizeof(indices[0]) * iCount;

	VulkanDevice& m_vkDevice;
	const VulkanSwapChain& m_vkSwapChain;
	std::unique_ptr<VulkanPipeline> m_vkPipeline;
	//temp
	VulkanBuffer vertexBuffer;
	VulkanBuffer indexBuffer;
	VulkanBuffer stagingBuffer;
	std::vector<std::unique_ptr<VulkanBuffer>> m_uniformBuffers;
	std::vector<VkDescriptorSet> m_vkDescriptorSets;
	UniformBufferObject ubo;
	float m_deltaTime;
	float m_previousTime;
};

