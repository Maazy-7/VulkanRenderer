#include "SimpleRenderSystem.h"

SimpleRenderSystem::SimpleRenderSystem(VulkanDevice& device, const VulkanRenderer& renderer)
	:m_vkDevice(device),
	m_vulkanRenderer(renderer),
	m_camera(Camera(glm::vec3(0.f, 0.f, 2.f), m_vulkanRenderer.getSwapChain().getSwapChainAspect(), 0.1f, 100.f)),
	m_model(std::make_unique<Model>(m_vkDevice, "Assets/viking_room.obj"))
{
	//allocating uniform buffers
	for (size_t i = 0; i < m_vulkanRenderer.getSwapChain().MAX_FRAMES_IN_FLIGHT; i++) 
	{
		m_uniformBuffers.push_back(std::make_unique<VulkanBuffer>(m_vkDevice, 3/*temp*/, sizeof(UniformBufferObject),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
		m_uniformBuffers[i]->map(0, sizeof(UniformBufferObject));
	}

	ubo.model = glm::mat4(1.f);
	ubo.view = m_camera.getView();
	ubo.proj = m_camera.getProjection();
	ubo.proj[1][1] *= -1;
	
	m_vkDescriptorSets.resize(m_vulkanRenderer.getSwapChain().MAX_FRAMES_IN_FLIGHT);
}

void SimpleRenderSystem::createPipeline(VulkanDescriptorSetLayout& vulkanDescriptorSetLayout) 
{
	m_vkPipeline = std::make_unique<VulkanPipeline>(m_vkDevice, "Shaders/shader.vert.spv", "Shaders/shader.frag.spv", m_vulkanRenderer.getSwapChain(), vulkanDescriptorSetLayout);
}

void SimpleRenderSystem::renderGameObjects(VkCommandBuffer commandBuffer, float dt)
{
	m_camera.update(dt, m_vulkanRenderer.getSwapChain().getSwapChainAspect());
	//ubo.model = glm::rotate(ubo.model, dt * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = m_camera.getProjection();
	ubo.proj[1][1] *= -1;
	ubo.view = m_camera.getView();
	m_uniformBuffers[m_vulkanRenderer.getSwapChain().getCurrentFrameIndex()]->writeToBuffer(0, sizeof(UniformBufferObject), &ubo);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vkPipeline->getPipeline());
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vkPipeline->getPipelineLayout(), 0, 1, 
		&m_vkDescriptorSets[m_vulkanRenderer.getSwapChain().getCurrentFrameIndex()], 0, nullptr);
	m_model->draw(commandBuffer);
}

std::vector<VkDescriptorSet>& SimpleRenderSystem::getDescriptorSets() 
{
	return m_vkDescriptorSets;
}