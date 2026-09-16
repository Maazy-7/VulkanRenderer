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
	vkCmdBufAllocateInfo.commandBufferCount = static_cast<uint32_t>(m_vkCommandBuffers.size());

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

	if (m_swapchainRecreateCallback) 
	{
		m_swapchainRecreateCallback();
	}
	else { throw std::runtime_error("no swapchain recreation callback function was set"); }
}

VkCommandBuffer VulkanRenderer::getCurrentCommandBuffer() const
{
	return m_vkCommandBuffers[m_currentFrameIndex];
}

const VulkanSwapChain& VulkanRenderer::getSwapChain() const
{
	return *m_vkSwapChain.get();
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

void VulkanRenderer::beginRendering(VkCommandBuffer commandBuffer)
{
	/*VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = m_vkSwapChain->getRenderPass();
	renderPassBeginInfo.framebuffer = m_vkSwapChain->getFramebuffer(m_currentImageIndex);
	renderPassBeginInfo.renderArea.offset = { 0,0 };
	renderPassBeginInfo.renderArea.extent = m_vkSwapChain->getSwapChainExtent();

	VkClearValue clearColor = {{{0.f, 0.f, 0.f, 1.f}}};
	renderPassBeginInfo.clearValueCount = 1;
	renderPassBeginInfo.pClearValues = &clearColor;

	vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);*/

	//transition image layout for rendering
	VkImageMemoryBarrier2 imgBarrier = {};
	imgBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	imgBarrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
	imgBarrier.srcAccessMask = VK_ACCESS_2_NONE;
	imgBarrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
	imgBarrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
	imgBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imgBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	imgBarrier.image = m_vkSwapChain->getSwapChainImage(m_currentImageIndex);
	imgBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imgBarrier.subresourceRange.baseMipLevel = 0;
	imgBarrier.subresourceRange.levelCount = 1;
	imgBarrier.subresourceRange.baseArrayLayer = 0;
	imgBarrier.subresourceRange.layerCount = 1;
	
	VkDependencyInfo dependencyInfo = {};
	dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	dependencyInfo.imageMemoryBarrierCount = 1;
	dependencyInfo.pImageMemoryBarriers = &imgBarrier;

	vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);

	//dynamic rendering attachment info
	VkRenderingAttachmentInfo colorAttachmentInfo = {};
	colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	colorAttachmentInfo.imageView = m_vkSwapChain->getSwapChainImageView(m_currentImageIndex);
	colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentInfo.clearValue = { {0.f, 0.f, 0.f, 1.f} };
	
	VkRenderingAttachmentInfo depthAttachmentInfo{};
	depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	depthAttachmentInfo.imageView = m_vkSwapChain->getSwapChainDepthBufferView();
	depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
	depthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthAttachmentInfo.clearValue.depthStencil = { 1.0f, 0 };

	VkRenderingInfo renderingInfo = {};
	renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	renderingInfo.renderArea.offset = { 0, 0 };
	renderingInfo.renderArea.extent = m_vkSwapChain->getSwapChainExtent();
	renderingInfo.layerCount = 1;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachments = &colorAttachmentInfo;
	renderingInfo.pDepthAttachment = &depthAttachmentInfo;
	renderingInfo.pStencilAttachment = VK_NULL_HANDLE;

	vkCmdBeginRendering(commandBuffer, &renderingInfo);
	
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

void VulkanRenderer::endRendering(VkCommandBuffer commandBuffer) 
{
	vkCmdEndRendering(commandBuffer);

	//transition image layout for presenation
	VkImageMemoryBarrier2 presentBarrier = {};
	presentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	presentBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
	presentBarrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
	presentBarrier.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
	presentBarrier.dstAccessMask = 0;
	presentBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	presentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	presentBarrier.image = m_vkSwapChain->getSwapChainImage(m_currentImageIndex);
	presentBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	presentBarrier.subresourceRange.baseMipLevel = 0;
	presentBarrier.subresourceRange.levelCount = 1;
	presentBarrier.subresourceRange.baseArrayLayer = 0;
	presentBarrier.subresourceRange.layerCount = 1;

	VkDependencyInfo presentDependencyInfo{};
	presentDependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	presentDependencyInfo.imageMemoryBarrierCount = 1;
	presentDependencyInfo.pImageMemoryBarriers = &presentBarrier;

	vkCmdPipelineBarrier2(commandBuffer, &presentDependencyInfo);
}

void VulkanRenderer::blitVkImageToSwapChain(VkCommandBuffer commandBuffer, const VulkanImage& srcImage)
{
	//transitioning swapchain image for being blitted to
	VkImageMemoryBarrier2 imgBarrier = {};
	imgBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	imgBarrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
	imgBarrier.srcAccessMask = VK_ACCESS_2_NONE;
	imgBarrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
	imgBarrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
	imgBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imgBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imgBarrier.image = m_vkSwapChain->getSwapChainImage(m_currentImageIndex);
	imgBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imgBarrier.subresourceRange.baseMipLevel = 0;
	imgBarrier.subresourceRange.levelCount = 1;
	imgBarrier.subresourceRange.baseArrayLayer = 0;
	imgBarrier.subresourceRange.layerCount = 1;

	VkDependencyInfo dependencyInfo = {};
	dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	dependencyInfo.imageMemoryBarrierCount = 1;
	dependencyInfo.pImageMemoryBarriers = &imgBarrier;

	vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
	int width = m_vkSwapChain->getSwapChainWidth();
	int height = m_vkSwapChain->getSwapChainHeight();

	VkImageBlit2 region = {};
	region.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;
	region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.srcSubresource.layerCount = 1;
	region.srcOffsets[1] = { width, height, 1 };

	region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.dstSubresource.layerCount = 1;
	region.dstOffsets[1] = { width, height, 1 };

	VkBlitImageInfo2 blitImgInfo{};
	blitImgInfo.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2;
	blitImgInfo.srcImage = srcImage.getVkImage();
	blitImgInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	blitImgInfo.dstImage = m_vkSwapChain->getSwapChainImage(m_currentImageIndex);
	blitImgInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	blitImgInfo.regionCount = 1;
	blitImgInfo.pRegions = &region;

	vkCmdBlitImage2(commandBuffer, &blitImgInfo);

	//transition image layout for presenation
	VkImageMemoryBarrier2 presentBarrier = {};
	presentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	imgBarrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
	imgBarrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
	presentBarrier.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
	presentBarrier.dstAccessMask = 0;
	presentBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	presentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	presentBarrier.image = m_vkSwapChain->getSwapChainImage(m_currentImageIndex);
	presentBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	presentBarrier.subresourceRange.baseMipLevel = 0;
	presentBarrier.subresourceRange.levelCount = 1;
	presentBarrier.subresourceRange.baseArrayLayer = 0;
	presentBarrier.subresourceRange.layerCount = 1;

	VkDependencyInfo presentDependencyInfo{};
	presentDependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	presentDependencyInfo.imageMemoryBarrierCount = 1;
	presentDependencyInfo.pImageMemoryBarriers = &presentBarrier;

	vkCmdPipelineBarrier2(commandBuffer, &presentDependencyInfo);
}

void VulkanRenderer::setSwapchainRecreateCallback(std::function<void()> swapchainRecreateFunction)
{
	m_swapchainRecreateCallback = std::move(swapchainRecreateFunction);
}
