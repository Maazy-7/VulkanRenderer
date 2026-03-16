#include "SimpleRenderSystem.h"

SimpleRenderSystem::SimpleRenderSystem(VulkanDevice& device, VulkanSwapChain& swapChain) 
	:m_vkDevice(device)
{
	createPipeline(swapChain);
}

void SimpleRenderSystem::createPipeline(VulkanSwapChain& swapChain) 
{
	m_vkPipeline = std::make_unique<VulkanPipeline>(m_vkDevice, "Shaders/shader.vert.spv", "Shaders/shader.frag.spv", swapChain);
}

void SimpleRenderSystem::renderGameObjects(VkCommandBuffer commandBuffer)
{
	//currently just renders a triangle
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vkPipeline->getPipeline());
	vkCmdDraw(commandBuffer, 3, 1, 0, 0);
}