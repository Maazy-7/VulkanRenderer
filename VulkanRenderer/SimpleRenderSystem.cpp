#include "SimpleRenderSystem.h"

SimpleRenderSystem::SimpleRenderSystem(VulkanDevice& device, const VulkanSwapChain& swapChain)
	:m_vkDevice(device),
	m_vkSwapChain(swapChain),
	//m_vulkanDescriptorSetLayout(vulkanDescriptorSetLayout),
	stagingBuffer(VulkanBuffer(m_vkDevice, vCount, vBuffSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)),
	vertexBuffer(VulkanBuffer(m_vkDevice, vCount, vBuffSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)),
	indexBuffer(VulkanBuffer(m_vkDevice, iCount, iBuffSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
{
	
	//allocating vertex and index buffers
	stagingBuffer.map(0, vBuffSize);
	stagingBuffer.writeToBuffer(0, vBuffSize, vertices.data());
	stagingBuffer.unMap();
	VulkanBuffer::copyBuffer(stagingBuffer, vertexBuffer, vBuffSize);
	
	stagingBuffer.map(0, iBuffSize);// writing and copy below, currently unsafe but fine right now since vBuffSize > iBuffSize
	stagingBuffer.writeToBuffer(0, iBuffSize, indices.data());
	stagingBuffer.unMap();
	VulkanBuffer::copyBuffer(stagingBuffer, indexBuffer, iBuffSize);

	//allocating uniform buffers
	for (size_t i = 0; i < swapChain.MAX_FRAMES_IN_FLIGHT; i++) 
	{
		m_uniformBuffers.push_back(std::make_unique<VulkanBuffer>(m_vkDevice, 3/*temp*/, sizeof(UniformBufferObject),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
		m_uniformBuffers[i]->map(0, sizeof(UniformBufferObject));
	}

	ubo.model = glm::mat4(1.f);//glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	ubo.view = glm::lookAt(glm::vec3(0.0f, 0.f, 2.f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), static_cast<float>(swapChain.getSwapChainExtent().width) 
		/ static_cast<float>(swapChain.getSwapChainExtent().height), 0.1f, 10.0f);
	ubo.proj[1][1] *= -1;

	m_vkDescriptorSets.resize(m_vkSwapChain.MAX_FRAMES_IN_FLIGHT);
}

void SimpleRenderSystem::createPipeline(VulkanDescriptorSetLayout& vulkanDescriptorSetLayout) 
{
	m_vkPipeline = std::make_unique<VulkanPipeline>(m_vkDevice, "Shaders/shader.vert.spv", "Shaders/shader.frag.spv", m_vkSwapChain, vulkanDescriptorSetLayout);
}

void SimpleRenderSystem::renderGameObjects(VkCommandBuffer commandBuffer, float dt)
{
	ubo.model = glm::rotate(ubo.model, dt * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	//ubo.model = glm::translate(ubo.model, glm::vec3(0.f, 0.01f, 0.f));
	m_uniformBuffers[m_vkSwapChain.getCurrentFrameIndex()]->writeToBuffer(0, sizeof(UniformBufferObject), &ubo);
	//m_uniformBuffers[m_vkSwapChain.getCurrentFrameIndex()]->unMap();
	//m_uniformBuffers[m_vkSwapChain.getCurrentFrameIndex()]->map(0, sizeof(UniformBufferObject));

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vkPipeline->getPipeline());
	vertexBuffer.bindVertexBuffer(commandBuffer);
	indexBuffer.bindIndexBuffer(commandBuffer);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vkPipeline->getPipelineLayout(), 0, 1, 
		&m_vkDescriptorSets[m_vkSwapChain.getCurrentFrameIndex()], 0, nullptr);
	vkCmdDrawIndexed(commandBuffer, indexBuffer.getBufferElementCount(), 1, 0, 0, 0);
}

std::vector<VkDescriptorSet>& SimpleRenderSystem::getDescriptorSets() 
{
	return m_vkDescriptorSets;
}