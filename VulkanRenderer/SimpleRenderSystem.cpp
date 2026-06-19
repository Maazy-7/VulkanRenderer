#include "SimpleRenderSystem.h"

SimpleRenderSystem::SimpleRenderSystem(VulkanDevice& device, VulkanSwapChain& swapChain) 
	:m_vkDevice(device),
	m_vkSwapChain(swapChain),
	stagingBuffer(VulkanBuffer(m_vkDevice, vCount, vBuffSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)),
	vertexBuffer(VulkanBuffer(m_vkDevice, vCount, vBuffSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)),
	indexBuffer(VulkanBuffer(m_vkDevice, iCount, iBuffSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
{
	createPipeline(swapChain);
	stagingBuffer.map(0, vBuffSize);
	stagingBuffer.writeToBuffer(0, vBuffSize, vertices.data());
	stagingBuffer.unMap();
	VulkanBuffer::copyBuffer(stagingBuffer, vertexBuffer, vBuffSize);
	
	stagingBuffer.map(0, iBuffSize);// writing and copy below, currently unsafe but fine right now since vBuffSize > iBuffSize
	stagingBuffer.writeToBuffer(0, iBuffSize, indices.data());
	stagingBuffer.unMap();
	VulkanBuffer::copyBuffer(stagingBuffer, indexBuffer, iBuffSize);

	m_uniformBuffers.reserve(swapChain.MAX_FRAMES_IN_FLIGHT);
	for (size_t i = 0; i < swapChain.MAX_FRAMES_IN_FLIGHT; i++) 
	{
		m_uniformBuffers.emplace_back(m_vkDevice, 3/*temp*/, sizeof(UniformBufferObject),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		m_uniformBuffers[i].map(0, sizeof(UniformBufferObject));
	}
}

void SimpleRenderSystem::createPipeline(VulkanSwapChain& swapChain) 
{
	m_vkPipeline = std::make_unique<VulkanPipeline>(m_vkDevice, "Shaders/shader.vert.spv", "Shaders/shader.frag.spv", swapChain);
}

void SimpleRenderSystem::renderGameObjects(VkCommandBuffer commandBuffer)
{
	//currently just renders a triangle
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vkPipeline->getPipeline());
	vertexBuffer.bindVertexBuffer(commandBuffer);
	indexBuffer.bindIndexBuffer(commandBuffer);
	vkCmdDrawIndexed(commandBuffer, indexBuffer.getBufferElementCount(), 1, 0, 0, 0);
}