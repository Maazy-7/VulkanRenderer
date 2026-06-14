#include "VulkanRenderer.h"

VulkanRenderer::VulkanRenderer(Window& window, VulkanDevice& device) 
	:m_window(window), m_vkDevice(device), m_vkSwapChain(std::make_unique<VulkanSwapChain>(device))
{
	createCommandBuffer();
}

VulkanRenderer::~VulkanRenderer() 
{

}

void VulkanRenderer::createCommandBuffer() 
{
	m_vkCommandBuffers.resize(VulkanSwapChain::MAX_FRAMES_IN_FLIGHT);
	VkCommandBufferAllocateInfo vkCmdBufAllocateInfo = {};
	vkCmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	vkCmdBufAllocateInfo.commandPool = m_vkDevice.getCommandPool();
	vkCmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	vkCmdBufAllocateInfo.commandBufferCount = (uint32_t) m_vkCommandBuffers.size();

	if (vkAllocateCommandBuffers(m_vkDevice.getDevice(), &vkCmdBufAllocateInfo, m_vkCommandBuffers.data()) != VK_SUCCESS) 
	{
		throw std::runtime_error("Failed to create Vulkan command buffers");
	}
}

void VulkanRenderer::recreateSwapChain() 
{
	int width, height = 0;
	glfwGetFramebufferSize(m_window.getWindow(), &width, &height);
	while (width == 0 || height == 0) 
	{
		glfwGetFramebufferSize(m_window.getWindow(), &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(m_vkDevice.getDevice());
	if (m_vkSwapChain == nullptr) 
	{
		m_vkSwapChain = std::make_unique<VulkanSwapChain>(m_vkDevice);
	}
	else 
	{
		std::shared_ptr<VulkanSwapChain> old_swapChain = std::move(m_vkSwapChain);
		m_vkSwapChain = std::make_unique<VulkanSwapChain>(m_vkDevice, old_swapChain);
	}
}

VkCommandBuffer VulkanRenderer::getCurrentCommandBuffer() 
{
	return m_vkCommandBuffers[m_currentFrameIndex];
}

VulkanSwapChain* VulkanRenderer::getSwapChain() 
{
	return m_vkSwapChain.get();
}

VkCommandBuffer VulkanRenderer::beginFrame() 
{
	VkResult result = m_vkSwapChain->acquireNextImage(&m_currentImageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) 
	{
		recreateSwapChain();
		return VK_NULL_HANDLE;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) 
	{
		throw std::runtime_error("Failed to acquire swap chain image");
	}

	m_isFrameStarted = true;

	VkCommandBufferBeginInfo cmdBufBeginInfo = {};
	cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufBeginInfo.flags = 0;
	cmdBufBeginInfo.pInheritanceInfo = nullptr;

	VkCommandBuffer commandBuffer = getCurrentCommandBuffer();
	if (vkBeginCommandBuffer(commandBuffer, &cmdBufBeginInfo) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to begin Vulkan command buffer");
	}

	return commandBuffer;
}

void VulkanRenderer::endFrame() 
{
	VkCommandBuffer commandBuffer = getCurrentCommandBuffer();
	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) 
	{
		throw std::runtime_error("Failed to end Vulkan command buffer");
	}

	VkResult result = m_vkSwapChain->submitCommandBuffers(&commandBuffer, &m_currentImageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_window.isFrameBufferResized())
	{
		recreateSwapChain();

	}
	else if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to present swap chain image");
	}

	m_isFrameStarted = false;
	m_currentFrameIndex = (m_currentFrameIndex + 1) % VulkanSwapChain::MAX_FRAMES_IN_FLIGHT;
}

void VulkanRenderer::beginRenderPass(VkCommandBuffer commandBuffer)
{
	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = m_vkSwapChain->getRenderPass();
	renderPassBeginInfo.framebuffer = m_vkSwapChain->getFramebuffer(m_currentImageIndex);
	renderPassBeginInfo.renderArea.offset = { 0,0 };
	renderPassBeginInfo.renderArea.extent = m_vkSwapChain->getSwapChainExtent();

	VkClearValue clearColor = {{{0.f, 0.f, 0.f, 1.f}}};
	renderPassBeginInfo.clearValueCount = 1;
	renderPassBeginInfo.pClearValues = &clearColor;

	vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(m_vkSwapChain->getSwapChainWidth());
	viewport.height = static_cast<float>(m_vkSwapChain->getSwapChainHeight());
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = m_vkSwapChain->getSwapChainExtent();
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void VulkanRenderer::endRenderPass(VkCommandBuffer commandBuffer) 
{
	vkCmdEndRenderPass(commandBuffer);
}