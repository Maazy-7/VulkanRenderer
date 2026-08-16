#include "VulkanImage.h"

VulkanImage::VulkanImage(VulkanImage&& other) noexcept
	: m_vulkanDevice(other.m_vulkanDevice),
	m_vkImage(other.m_vkImage),
	m_vkDeviceMemory(other.m_vkDeviceMemory),
	m_vkImageView(other.m_vkImageView),
	m_vkSampler(other.m_vkSampler),
	m_vkFormat(other.m_vkFormat)
{
	other.m_vkImage = VK_NULL_HANDLE;
	other.m_vkDeviceMemory = VK_NULL_HANDLE;
	other.m_vkImageView = VK_NULL_HANDLE;
	other.m_vkSampler = VK_NULL_HANDLE;
	other.m_vkFormat = VK_FORMAT_UNDEFINED;
}

VulkanImage::VulkanImage(VulkanDevice& vulkanDevice, uint32_t width, uint32_t height, VkFormat format,
	VkImageTiling imgTilling, VkImageUsageFlags imgUsage, VkMemoryPropertyFlags memoryPropertyFlags, uint32_t mipLevels)
	:m_vulkanDevice(vulkanDevice), m_vkFormat(format)
{
	//creating vk image
	VkImageCreateInfo imgCreateInfo{};
	imgCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imgCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imgCreateInfo.extent.width = width;
	imgCreateInfo.extent.height = height;
	imgCreateInfo.extent.depth = 1;
	imgCreateInfo.mipLevels = mipLevels;
	imgCreateInfo.arrayLayers = 1;
	imgCreateInfo.format = format;
	imgCreateInfo.tiling = imgTilling;
	imgCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imgCreateInfo.usage = imgUsage;
	imgCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imgCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imgCreateInfo.flags = 0;

	if (vkCreateImage(m_vulkanDevice.getDevice(), &imgCreateInfo, nullptr, &m_vkImage) != VK_SUCCESS) 
	{
		throw std::runtime_error("failed to create Vulkan image");
	}

	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(m_vulkanDevice.getDevice(), m_vkImage, &memoryRequirements);

	VkMemoryAllocateInfo memAllocateInfo{};
	memAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllocateInfo.allocationSize = memoryRequirements.size;
	memAllocateInfo.memoryTypeIndex = m_vulkanDevice.findMemoryType(memoryRequirements.memoryTypeBits, memoryPropertyFlags);

	if (vkAllocateMemory(m_vulkanDevice.getDevice(), &memAllocateInfo, nullptr, &m_vkDeviceMemory) != VK_SUCCESS) 
	{
		throw std::runtime_error("failed to allocate Vulkan image memory");
	}

	vkBindImageMemory(m_vulkanDevice.getDevice(), m_vkImage, m_vkDeviceMemory, 0);

	//creating vk image view
	VkImageViewCreateInfo imgViewCreateInfo{};
	imgViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imgViewCreateInfo.image = m_vkImage;
	imgViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imgViewCreateInfo.format = format;
	imgViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imgViewCreateInfo.subresourceRange.baseMipLevel = 0;
	imgViewCreateInfo.subresourceRange.levelCount = mipLevels;
	imgViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imgViewCreateInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(m_vulkanDevice.getDevice(), &imgViewCreateInfo, nullptr, &m_vkImageView) != VK_SUCCESS) 
	{
		throw std::runtime_error("failed to create Vulkan image view");
	}

	//creating vk sampler
	VkSamplerCreateInfo samplerCreateInfo{};
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.anisotropyEnable = VK_TRUE;
	samplerCreateInfo.maxAnisotropy = m_vulkanDevice.getPhysicalDeviceProperties().limits.maxSamplerAnisotropy;
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
	samplerCreateInfo.compareEnable = VK_FALSE;
	samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCreateInfo.mipLodBias = 0.f;
	samplerCreateInfo.minLod = 0.f;
	samplerCreateInfo.maxLod = VK_LOD_CLAMP_NONE;

	if (vkCreateSampler(m_vulkanDevice.getDevice(), &samplerCreateInfo, nullptr, &m_vkSampler) != VK_SUCCESS) 
	{
		throw std::runtime_error("failed to create Vulkan sampler");
	}
}

VulkanImage::VulkanImage(VulkanDevice& vulkanDevice, uint32_t width, uint32_t height, VkFormat format, VkImageTiling imgTilling, 
	VkImageUsageFlags imgUsage, VkMemoryPropertyFlags memoryPropertyFlags, VkImageAspectFlagBits imgAspectFlags, bool useSampler, VkSampler sampler)
	:m_vulkanDevice(vulkanDevice), m_vkFormat(format)
{
	//ideally for making a non sampling image ie swapchain images or depth buffer
	//creating vk image
	VkImageCreateInfo imgCreateInfo{};
	imgCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imgCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imgCreateInfo.extent.width = width;
	imgCreateInfo.extent.height = height;
	imgCreateInfo.extent.depth = 1;
	imgCreateInfo.mipLevels = 1;
	imgCreateInfo.arrayLayers = 1;
	imgCreateInfo.format = format;
	imgCreateInfo.tiling = imgTilling;
	imgCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imgCreateInfo.usage = imgUsage;
	imgCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imgCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imgCreateInfo.flags = 0;

	if (vkCreateImage(m_vulkanDevice.getDevice(), &imgCreateInfo, nullptr, &m_vkImage) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create Vulkan image");
	}

	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(m_vulkanDevice.getDevice(), m_vkImage, &memoryRequirements);

	VkMemoryAllocateInfo memAllocateInfo{};
	memAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllocateInfo.allocationSize = memoryRequirements.size;
	memAllocateInfo.memoryTypeIndex = m_vulkanDevice.findMemoryType(memoryRequirements.memoryTypeBits, memoryPropertyFlags);

	if (vkAllocateMemory(m_vulkanDevice.getDevice(), &memAllocateInfo, nullptr, &m_vkDeviceMemory) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate Vulkan image memory");
	}

	vkBindImageMemory(m_vulkanDevice.getDevice(), m_vkImage, m_vkDeviceMemory, 0);

	//creating vk image view
	VkImageViewCreateInfo imgViewCreateInfo{};
	imgViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imgViewCreateInfo.image = m_vkImage;
	imgViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imgViewCreateInfo.format = format;
	imgViewCreateInfo.subresourceRange.aspectMask = static_cast<VkImageAspectFlags>(imgAspectFlags);
	imgViewCreateInfo.subresourceRange.baseMipLevel = 0;
	imgViewCreateInfo.subresourceRange.levelCount = 1;
	imgViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imgViewCreateInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(m_vulkanDevice.getDevice(), &imgViewCreateInfo, nullptr, &m_vkImageView) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create Vulkan image view");
	}

	//only creating sampler if its needed
	m_vkSampler = VK_NULL_HANDLE;
	
	if (sampler) //existing sampler given
	{
		m_vkSampler = sampler;
	}
	else if (useSampler) //create one if not given
	{
		//creating vk sampler
		VkSamplerCreateInfo samplerCreateInfo{};
		samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.anisotropyEnable = VK_TRUE;
		samplerCreateInfo.maxAnisotropy = m_vulkanDevice.getPhysicalDeviceProperties().limits.maxSamplerAnisotropy;
		samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
		samplerCreateInfo.compareEnable = VK_FALSE;
		samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerCreateInfo.mipLodBias = 0.f;
		samplerCreateInfo.minLod = 0.f;
		samplerCreateInfo.maxLod = 0.f;

		if (vkCreateSampler(m_vulkanDevice.getDevice(), &samplerCreateInfo, nullptr, &m_vkSampler) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create Vulkan sampler");
		}
	}
}

VulkanImage::VulkanImage(VulkanDevice& vulkanDevice, VkImage vkImage, VkImageView vkImageView, VkFormat format, VkSampler vkSampler)
	:m_vulkanDevice(vulkanDevice), m_vkImage(vkImage), m_vkImageView(vkImageView), m_vkSampler(vkSampler)
{
	m_vkFormat = format;

	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(m_vulkanDevice.getDevice(), m_vkImage, &memoryRequirements);

	VkMemoryAllocateInfo memAllocateInfo{};
	memAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllocateInfo.allocationSize = memoryRequirements.size; VkMemoryPropertyFlags memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;//for just hardcoded
	memAllocateInfo.memoryTypeIndex = m_vulkanDevice.findMemoryType(memoryRequirements.memoryTypeBits, memoryPropertyFlags);

	if (vkAllocateMemory(m_vulkanDevice.getDevice(), &memAllocateInfo, nullptr, &m_vkDeviceMemory) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate Vulkan image memory");
	}

	vkBindImageMemory(m_vulkanDevice.getDevice(), m_vkImage, m_vkDeviceMemory, 0);

	if (vkSampler == VK_NULL_HANDLE) { /*for now do nothing*/ }
	else 
	{
		m_vkSampler = vkSampler;
	}
}


VulkanImage::~VulkanImage()
{
	if (m_vkSampler) { vkDestroySampler(m_vulkanDevice.getDevice(), m_vkSampler, nullptr); }
	vkDestroyImageView(m_vulkanDevice.getDevice(), m_vkImageView, nullptr);
	vkDestroyImage(m_vulkanDevice.getDevice(), m_vkImage, nullptr);
	vkFreeMemory(m_vulkanDevice.getDevice(), m_vkDeviceMemory, nullptr);
}

void VulkanImage::transitionImageLayout(VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels)
{
	VkCommandBuffer commandBuffer = m_vulkanDevice.beginSingleTimeCommands();

	VkImageMemoryBarrier2 imgMemoryBarrier2{};
	imgMemoryBarrier2.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	imgMemoryBarrier2.oldLayout = oldLayout;
	imgMemoryBarrier2.newLayout = newLayout;
	imgMemoryBarrier2.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imgMemoryBarrier2.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imgMemoryBarrier2.image = m_vkImage;
	imgMemoryBarrier2.subresourceRange.baseMipLevel = 0;
	imgMemoryBarrier2.subresourceRange.levelCount = mipLevels;
	imgMemoryBarrier2.subresourceRange.baseArrayLayer = 0;
	imgMemoryBarrier2.subresourceRange.layerCount = 1;

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) 
	{
		imgMemoryBarrier2.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT)
		{
			imgMemoryBarrier2.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else { imgMemoryBarrier2.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; }

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) 
	{
		imgMemoryBarrier2.srcAccessMask = 0;
		imgMemoryBarrier2.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		imgMemoryBarrier2.srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		imgMemoryBarrier2.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) 
	{
		imgMemoryBarrier2.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imgMemoryBarrier2.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		imgMemoryBarrier2.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
		imgMemoryBarrier2.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) 
	{
		imgMemoryBarrier2.srcAccessMask = 0;
		imgMemoryBarrier2.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		imgMemoryBarrier2.srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		imgMemoryBarrier2.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) 
	{
		imgMemoryBarrier2.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		imgMemoryBarrier2.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	}
	else 
	{
		throw std::invalid_argument("unsupported layout transition");
	}

	VkDependencyInfo dependencyInfo{};
	dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	dependencyInfo.imageMemoryBarrierCount = 1;
	dependencyInfo.pImageMemoryBarriers = &imgMemoryBarrier2;

	vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);

	m_vulkanDevice.endSingleTimeCommands(commandBuffer);
}

void VulkanImage::transitionImageLayout(VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, VkAccessFlags2 srcAccess, 
	VkAccessFlags2 dstAccess, VkPipelineStageFlags2 srcStage, VkPipelineStageFlags2 dstStage, uint32_t layerCount, VkCommandBuffer mainCommandBuffer)
{
	//can use either single time command buffer or main command buffer
	VkCommandBuffer commandBuffer = mainCommandBuffer == VK_NULL_HANDLE ? m_vulkanDevice.beginSingleTimeCommands() : mainCommandBuffer;

	VkImageMemoryBarrier2 imgMemoryBarrier2{};
	imgMemoryBarrier2.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	imgMemoryBarrier2.oldLayout = oldLayout;
	imgMemoryBarrier2.newLayout = newLayout;
	imgMemoryBarrier2.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imgMemoryBarrier2.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imgMemoryBarrier2.image = m_vkImage;
	imgMemoryBarrier2.subresourceRange.baseMipLevel = 0;
	imgMemoryBarrier2.subresourceRange.levelCount = 1;
	imgMemoryBarrier2.subresourceRange.baseArrayLayer = 0;
	imgMemoryBarrier2.subresourceRange.layerCount = layerCount;

	imgMemoryBarrier2.srcAccessMask = srcAccess;
	imgMemoryBarrier2.dstAccessMask = dstAccess;
	imgMemoryBarrier2.srcStageMask = srcStage;
	imgMemoryBarrier2.dstStageMask = dstStage;

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL || oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		imgMemoryBarrier2.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT)
		{
			imgMemoryBarrier2.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else { imgMemoryBarrier2.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; }

	VkDependencyInfo dependencyInfo{};
	dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	dependencyInfo.imageMemoryBarrierCount = 1;
	dependencyInfo.pImageMemoryBarriers = &imgMemoryBarrier2;

	vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);

	if (mainCommandBuffer == VK_NULL_HANDLE)
	{
		m_vulkanDevice.endSingleTimeCommands(commandBuffer);
	}
}

void VulkanImage::copyBufferToImage(VulkanBuffer& srcBuffer, uint32_t width, uint32_t height)
{
	VkCommandBuffer commandBuffer = m_vulkanDevice.beginSingleTimeCommands();
	
	VkBufferImageCopy2 imgCopyRegion{};
	imgCopyRegion.sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2;
	imgCopyRegion.bufferOffset = 0;
	imgCopyRegion.bufferRowLength = 0;
	imgCopyRegion.bufferImageHeight = 0;

	imgCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imgCopyRegion.imageSubresource.mipLevel = 0;
	imgCopyRegion.imageSubresource.baseArrayLayer = 0;
	imgCopyRegion.imageSubresource.layerCount = 1;
	imgCopyRegion.imageOffset = { 0, 0, 0 };
	imgCopyRegion.imageExtent = { width, height, 1 };

	VkCopyBufferToImageInfo2 copyBufferToImageInfo2{};
	copyBufferToImageInfo2.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2;
	copyBufferToImageInfo2.srcBuffer = srcBuffer.getBuffer();
	copyBufferToImageInfo2.dstImage = m_vkImage;
	copyBufferToImageInfo2.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	copyBufferToImageInfo2.regionCount = 1;
	copyBufferToImageInfo2.pRegions = &imgCopyRegion;

	vkCmdCopyBufferToImage2(commandBuffer, &copyBufferToImageInfo2);

	m_vulkanDevice.endSingleTimeCommands(commandBuffer);
}

void VulkanImage::generateMipMaps(int width, int height, uint32_t mipLevels)
{
	//checking if image supports linear blitting
	m_vulkanDevice.findSupportedFormat({m_vkFormat}, VK_IMAGE_TILING_LINEAR, VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT, 
		"texture image format does not support linear blitting");
	m_vulkanDevice.findSupportedFormat({m_vkFormat}, VK_IMAGE_TILING_LINEAR, VK_FORMAT_FEATURE_2_BLIT_SRC_BIT | VK_FORMAT_FEATURE_2_BLIT_DST_BIT,
		"texture image format does not support image blit 2 src and dst ");

	VkCommandBuffer commandBuffer = m_vulkanDevice.beginSingleTimeCommands();
	VkImageMemoryBarrier2 imgMemoryBarrier2{};
	imgMemoryBarrier2.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	imgMemoryBarrier2.image = m_vkImage;
	imgMemoryBarrier2.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imgMemoryBarrier2.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imgMemoryBarrier2.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;//fine because depth maps ideally wouldn't need mipmaps
	imgMemoryBarrier2.subresourceRange.baseArrayLayer = 0;
	imgMemoryBarrier2.subresourceRange.layerCount = 1;
	imgMemoryBarrier2.subresourceRange.levelCount = 1;

	VkDependencyInfo dependencyInfo{};
	dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	dependencyInfo.imageMemoryBarrierCount = 1;
	dependencyInfo.pImageMemoryBarriers = &imgMemoryBarrier2;

	int mipWidth = width;
	int mipHeight = height;

	for (uint32_t i = 1; i < mipLevels; i++) 
	{
		imgMemoryBarrier2.subresourceRange.baseMipLevel = i;
		imgMemoryBarrier2.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imgMemoryBarrier2.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imgMemoryBarrier2.srcAccessMask = 0;
		imgMemoryBarrier2.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imgMemoryBarrier2.srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		imgMemoryBarrier2.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;

		vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);

		imgMemoryBarrier2.subresourceRange.baseMipLevel = i - 1;
		imgMemoryBarrier2.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imgMemoryBarrier2.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		imgMemoryBarrier2.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imgMemoryBarrier2.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		imgMemoryBarrier2.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
		imgMemoryBarrier2.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;

		vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);

		VkImageBlit2 imgBlit2{};
		imgBlit2.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;
		imgBlit2.srcOffsets[0] = { 0, 0, 0 };
		imgBlit2.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		imgBlit2.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imgBlit2.srcSubresource.mipLevel = i - 1;
		imgBlit2.srcSubresource.baseArrayLayer = 0;
		imgBlit2.srcSubresource.layerCount = 1;
		imgBlit2.dstOffsets[0] = { 0, 0, 0 };
		imgBlit2.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
		imgBlit2.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imgBlit2.dstSubresource.mipLevel = i;
		imgBlit2.dstSubresource.baseArrayLayer = 0;
		imgBlit2.dstSubresource.layerCount = 1;

		VkBlitImageInfo2 blitImgInfo2{};
		blitImgInfo2.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2;
		blitImgInfo2.dstImage = m_vkImage;
		blitImgInfo2.srcImage = m_vkImage;
		blitImgInfo2.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		blitImgInfo2.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		blitImgInfo2.filter = VK_FILTER_LINEAR;
		blitImgInfo2.regionCount = 1;
		blitImgInfo2.pRegions = &imgBlit2;

		vkCmdBlitImage2(commandBuffer, &blitImgInfo2);

		imgMemoryBarrier2.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		imgMemoryBarrier2.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imgMemoryBarrier2.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		imgMemoryBarrier2.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		imgMemoryBarrier2.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
		imgMemoryBarrier2.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

		vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);

		if (mipWidth > 1) { mipWidth /= 2; }
		if (mipHeight > 1) { mipHeight /= 2; }
	}

	imgMemoryBarrier2.subresourceRange.baseMipLevel = mipLevels - 1;
	imgMemoryBarrier2.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imgMemoryBarrier2.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imgMemoryBarrier2.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	imgMemoryBarrier2.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	imgMemoryBarrier2.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
	imgMemoryBarrier2.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

	vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);

	m_vulkanDevice.endSingleTimeCommands(commandBuffer);
}

VkImage VulkanImage::getVkImage() const
{
	return m_vkImage;
}

VkImageView VulkanImage::getVkImageView() const
{
	return m_vkImageView;
}

VkSampler VulkanImage::getVkSampler() const
{
	return m_vkSampler;
}
