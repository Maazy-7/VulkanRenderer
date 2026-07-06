#include "VulkanImage.h"

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
	VkImageUsageFlags imgUsage, VkMemoryPropertyFlags memoryPropertyFlags, VkImageAspectFlagBits imgAspectFlags, bool useSampler)
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
	if (useSampler) 
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

	VkImageMemoryBarrier imgMemoryBarrier{};
	imgMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imgMemoryBarrier.oldLayout = oldLayout;
	imgMemoryBarrier.newLayout = newLayout;
	imgMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imgMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imgMemoryBarrier.image = m_vkImage;
	imgMemoryBarrier.subresourceRange.baseMipLevel = 0;
	imgMemoryBarrier.subresourceRange.levelCount = mipLevels;
	imgMemoryBarrier.subresourceRange.baseArrayLayer = 0;
	imgMemoryBarrier.subresourceRange.layerCount = 1;

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) 
	{
		imgMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT)
		{
			imgMemoryBarrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else { imgMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; }

	VkPipelineStageFlags srcStageFlags;
	VkPipelineStageFlags dstStageFlags;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) 
	{
		imgMemoryBarrier.srcAccessMask = 0;
		imgMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		srcStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) 
	{
		imgMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imgMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		srcStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStageFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) 
	{
		imgMemoryBarrier.srcAccessMask = 0;
		imgMemoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		srcStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStageFlags = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else 
	{
		throw std::invalid_argument("unsupported layout transition");
	}

	vkCmdPipelineBarrier(commandBuffer, srcStageFlags, dstStageFlags, 0, 0, nullptr, 0, nullptr, 1, &imgMemoryBarrier);

	m_vulkanDevice.endSingleTimeCommands(commandBuffer);
}

void VulkanImage::copyBufferToImage(VulkanBuffer& srcBuffer, uint32_t width, uint32_t height)
{
	VkCommandBuffer commandBuffer = m_vulkanDevice.beginSingleTimeCommands();

	VkBufferImageCopy imgCopyRegion{};
	imgCopyRegion.bufferOffset = 0;
	imgCopyRegion.bufferRowLength = 0;
	imgCopyRegion.bufferImageHeight = 0;

	imgCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imgCopyRegion.imageSubresource.mipLevel = 0;
	imgCopyRegion.imageSubresource.baseArrayLayer = 0;
	imgCopyRegion.imageSubresource.layerCount = 1;
	imgCopyRegion.imageOffset = { 0, 0, 0 };
	imgCopyRegion.imageExtent = { width, height, 1 };

	vkCmdCopyBufferToImage(commandBuffer, srcBuffer.getBuffer(), m_vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imgCopyRegion);

	m_vulkanDevice.endSingleTimeCommands(commandBuffer);
}

void VulkanImage::generateMipMaps(int width, int height, uint32_t mipLevels)
{
	//checking if image supports linear blitting
	m_vulkanDevice.findSupportedFormat({m_vkFormat}, VK_IMAGE_TILING_LINEAR, VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT, 
		"texture image format does not support linear blitting");

	VkCommandBuffer commandBuffer = m_vulkanDevice.beginSingleTimeCommands();
	VkImageMemoryBarrier imgMemoryBarrier{};
	imgMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imgMemoryBarrier.image = m_vkImage;
	imgMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imgMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imgMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;//fine because depth maps ideally wouldn't need mipmaps
	imgMemoryBarrier.subresourceRange.baseArrayLayer = 0;
	imgMemoryBarrier.subresourceRange.layerCount = 1;
	imgMemoryBarrier.subresourceRange.levelCount = 1;

	int mipWidth = width;
	int mipHeight = height;

	for (uint32_t i = 1; i < mipLevels; i++) 
	{
		imgMemoryBarrier.subresourceRange.baseMipLevel = i;
		imgMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imgMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imgMemoryBarrier.srcAccessMask = 0;
		imgMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &imgMemoryBarrier);

		imgMemoryBarrier.subresourceRange.baseMipLevel = i - 1;
		imgMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imgMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		imgMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imgMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &imgMemoryBarrier);

		VkImageBlit imgBlit{};
		imgBlit.srcOffsets[0] = { 0, 0, 0 };
		imgBlit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		imgBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imgBlit.srcSubresource.mipLevel = i - 1;
		imgBlit.srcSubresource.baseArrayLayer = 0;
		imgBlit.srcSubresource.layerCount = 1;
		imgBlit.dstOffsets[0] = { 0, 0, 0 };
		imgBlit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
		imgBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imgBlit.dstSubresource.mipLevel = i;
		imgBlit.dstSubresource.baseArrayLayer = 0;
		imgBlit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(commandBuffer,
			m_vkImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			m_vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &imgBlit,
			VK_FILTER_LINEAR);

		imgMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		imgMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imgMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		imgMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &imgMemoryBarrier);

		if (mipWidth > 1) { mipWidth /= 2; }
		if (mipHeight > 1) { mipHeight /= 2; }
	}

	imgMemoryBarrier.subresourceRange.baseMipLevel = mipLevels - 1;
	imgMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imgMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imgMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	imgMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(commandBuffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		0, nullptr,
		0, nullptr,
		1, &imgMemoryBarrier);

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
