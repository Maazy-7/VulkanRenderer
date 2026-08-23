#pragma once

#include "VulkanImage.h"

class Texture
{
public:

	Texture(const Texture&) = delete;
	Texture& operator=(const Texture&) = delete;

	Texture(VulkanDevice& vulkanDevice, const std::string& texturePath, VkFormat imgFormat, VkImageTiling imgTiling, 
		VkImageUsageFlags imgUsageFlags, VkMemoryPropertyFlags memoryPropertyFlags, bool generateMipMaps = true);

	~Texture();

	VkImage getVkImage() const;
	VkImageView getVkImageView() const;
	VkSampler getVkSampler() const;

private:
	VulkanDevice& m_vulkanDevice;
	std::unique_ptr<VulkanImage> m_vulkanImage;
};

