#pragma once

#include "VulkanDevice.h"
#include "VulkanBuffer.h"

class VulkanImage
{
public:
	VulkanImage(const VulkanImage&) = delete;
	VulkanImage& operator=(const VulkanImage&) = delete;

	VulkanImage(VulkanDevice& vulkanDevice, uint32_t width, uint32_t height, VkFormat format, 
		VkImageTiling imgTilling, VkImageUsageFlags imgUsage, VkMemoryPropertyFlags memoryPropertyFlags);
	VulkanImage(VulkanDevice& vulkanDevice, uint32_t width, uint32_t height, VkFormat format,
		VkImageTiling imgTilling, VkImageUsageFlags imgUsage, VkMemoryPropertyFlags memoryPropertyFlags, VkImageAspectFlags imgAspectFlags, bool useSampler = false);
	~VulkanImage();

	void transitionImageLayout(VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	void copyBufferToImage(VulkanBuffer& srcBuffer, uint32_t width, uint32_t height);

	VkImage getVkImage() const;
	VkImageView getVkImageView() const;
	VkSampler getVkSampler() const; 

private:
	VulkanDevice& m_vulkanDevice;
	VkImage m_vkImage;
	VkDeviceMemory m_vkDeviceMemory;
	VkImageView m_vkImageView;
	VkSampler m_vkSampler;
};

