#include "SimpleRenderSystem.h"

SimpleRenderSystem::SimpleRenderSystem(VulkanDevice& device, VulkanSwapChain& swapChain) 
	:m_vkDevice(device), 
	stagingBuffer(VulkanBuffer(m_vkDevice, vertices, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)),
	vertexBuffer(VulkanBuffer(m_vkDevice, vertices, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
{
	createPipeline(swapChain);
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
	stagingBuffer.map(0, bufferSize, vertices.data());
	VulkanBuffer::copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
}

void SimpleRenderSystem::createPipeline(VulkanSwapChain& swapChain) 
{
	m_vkPipeline = std::make_unique<VulkanPipeline>(m_vkDevice, "Shaders/shader.vert.spv", "Shaders/shader.frag.spv", swapChain);
}

void SimpleRenderSystem::renderGameObjects(VkCommandBuffer commandBuffer)
{
	//currently just renders a triangle
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vkPipeline->getPipeline());
	vertexBuffer.bindBuffer(commandBuffer);
	vkCmdDraw(commandBuffer, vertexBuffer.getBufferElementCount(), 1, 0, 0);
}