#pragma once

#include "VulkanDevice.h"
#include "VulkanBuffer.h"

class VulkanImage
{
public:
	VulkanImage(const VulkanImage&) = delete;
	VulkanImage& operator=(const VulkanImage&) = delete;

	VulkanImage(VulkanDevice& vulkanDevice, uint32_t width, uint32_t height, VkFormat format, 
		VkImageTiling imgTilling, VkImageUsageFlags imgUsage, VkMemoryPropertyFlags memoryPropertyFlags, 
		uint32_t mipLevels = 1);
	VulkanImage(VulkanDevice& vulkanDevice, uint32_t width, uint32_t height, VkFormat format,
		VkImageTiling imgTilling, VkImageUsageFlags imgUsage, VkMemoryPropertyFlags memoryPropertyFlags, 
		VkImageAspectFlagBits imgAspectFlags, bool useSampler = false);
	~VulkanImage();

	void transitionImageLayout(VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels = 1);
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
};

