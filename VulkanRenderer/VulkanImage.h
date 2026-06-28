#pragma once

#include "VulkanDevice.h"

class VulkanImage
{
public:
	VulkanImage(const VulkanImage&) = delete;
	VulkanImage& operator=(const VulkanImage&) = delete;

	VulkanImage(VulkanDevice& vulkanDevice, uint32_t width, uint32_t height, VkFormat format, 
		VkImageTiling imgTilling, VkImageUsageFlags imgUsage, VkMemoryPropertyFlags memoryPropertyFlags);
	~VulkanImage();

private:
	VulkanDevice& m_vulkanDevice;
	VkImage m_vkImage;
	VkDeviceMemory m_vkDeviceMemory;
	VkImageView m_vkImageView;

};

