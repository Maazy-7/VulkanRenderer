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

	for (size_t i = 0; i < swapChain.MAX_FRAMES_IN_FLIGHT; i++) 
	{
		m_uniformBuffers.push_back(std::make_unique<VulkanBuffer>(m_vkDevice, 3/*temp*/, sizeof(UniformBufferObject),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
		m_uniformBuffers[i]->map(0, sizeof(UniformBufferObject));
	}

	ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), (float)swapChain.getSwapChainExtent().width / (float)swapChain.getSwapChainExtent().height, 0.1f, 10.0f);
	ubo.proj[1][1] *= -1;
}

void SimpleRenderSystem::createPipeline(VulkanSwapChain& swapChain) 
{
	m_vkPipeline = std::make_unique<VulkanPipeline>(m_vkDevice, "Shaders/shader.vert.spv", "Shaders/shader.frag.spv", swapChain);
}

void SimpleRenderSystem::renderGameObjects(VkCommandBuffer commandBuffer, float dt)
{
	ubo.model = glm::rotate(glm::mat4(1.0f), dt * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	m_uniformBuffers[m_vkSwapChain.getCurrentFrameIndex()]->writeToBuffer(0, sizeof(UniformBufferObject), &ubo);
	
	//currently just renders a triangle
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vkPipeline->getPipeline());
	vertexBuffer.bindVertexBuffer(commandBuffer);
	indexBuffer.bindIndexBuffer(commandBuffer);
	vkCmdDrawIndexed(commandBuffer, indexBuffer.getBufferElementCount(), 1, 0, 0, 0);
}