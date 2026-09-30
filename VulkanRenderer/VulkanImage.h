#pragma once

#include "VulkanDevice.h"
#include "VulkanBuffer.h"

class VulkanImage
{
public:
	VulkanImage(const VulkanImage&) = delete;
	VulkanImage& operator=(const VulkanImage&) = delete;

	//move constructor
	VulkanImage(VulkanImage&& other) noexcept;

	VulkanImage(VulkanDevice& vulkanDevice, uint32_t width, uint32_t height, VkFormat format, 
		VkImageTiling imgTilling, VkImageUsageFlags imgUsage, VkMemoryPropertyFlags memoryPropertyFlags, 
		uint32_t mipLevels = 1);
	VulkanImage(VulkanDevice& vulkanDevice, uint32_t width, uint32_t height, VkFormat format,
		VkImageTiling imgTilling, VkImageUsageFlags imgUsage, VkMemoryPropertyFlags memoryPropertyFlags, 
		VkImageAspectFlagBits imgAspectFlags, bool useSampler = false, VkSampler sampler = VK_NULL_HANDLE);
	VulkanImage(VulkanDevice& vulkanDevice, VkImage vkImage, VkImageViewCreateInfo* vkImageViewCreateInfo, VkFormat format, VkSampler vkSampler = VK_NULL_HANDLE);
	~VulkanImage();

	void transitionImageLayout(VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels = 1);
	void transitionImageLayout(VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, VkAccessFlags2 srcAccess, VkAccessFlags2 dstAccess,
		VkPipelineStageFlags2 srcStage, VkPipelineStageFlags2 dstStage, uint32_t layerCount, VkCommandBuffer mainCommandBuffer = VK_NULL_HANDLE);
	void transitionImageLayout(VkFormat format, VkImageLayout newLayout, VkAccessFlags2 dstAccess, VkPipelineStageFlags2 dstStage, uint32_t layerCount,
		VkCommandBuffer mainCommandBuffer = VK_NULL_HANDLE);
	void copyBufferToImage(VulkanBuffer& srcBuffer, uint32_t width, uint32_t height);
	void generateMipMaps(int width, int height, uint32_t mipLevels);

	VkImage getVkImage() const;
	VkImageView getVkImageView() const;
	VkSampler getVkSampler() const; 

private:
	VulkanDevice& m_vulkanDevice;
	VkImage m_vkImage;
	VkDeviceMemory m_vkDeviceMemory;
	VkImageView m_vkImageView;
	VkSampler m_vkSampler;
	VkFormat m_vkFormat;
	bool m_ownsSampler = true;//used to determine if vulkan image object owns the sample it was given
	//image transition info stored to ensure the correct old layout/access/pipeline stage flags are used
	VkImageLayout m_currentImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkAccessFlags2 m_currentAccessFlag = VK_ACCESS_2_NONE;
	VkPipelineStageFlags2 m_currentPipelineStageFlags = VK_PIPELINE_STAGE_2_NONE;
};

