#pragma once

#include "VulkanImage.h"

class Texture
{
public:

	Texture(const Texture&) = delete;
	Texture& operator=(const Texture&) = delete;

	//move constructor
	Texture(Texture&&) noexcept = default;

	Texture(VulkanDevice& vulkanDevice, const std::string& texturePath, VkFormat imgFormat, VkImageTiling imgTiling, 
		VkImageUsageFlags imgUsageFlags, VkMemoryPropertyFlags memoryPropertyFlags, bool generateMipMaps = true);
	Texture(VulkanDevice& vulkanDevice, const std::string& texturePath, VkFormat imgFormat = VK_FORMAT_R8G8B8A8_SRGB, bool generateMipMaps = true);
	Texture(VulkanDevice& vulkanDevice, uint8_t r, uint8_t g, uint8_t b, VkFormat imgFormat = VK_FORMAT_R8G8B8A8_SRGB);//a single color texture

	~Texture();

	VkImage getVkImage() const;
	VkImageView getVkImageView() const;
	VkSampler getVkSampler() const;

private:
	VulkanDevice& m_vulkanDevice;
	std::optional<VulkanImage> m_vulkanImage;
};

