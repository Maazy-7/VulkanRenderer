#include "Texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

Texture::Texture(VulkanDevice& vulkanDevice, const std::string& texturePath, VkFormat imgFormat, VkImageTiling imgTiling,
	VkImageUsageFlags imgUsageFlags, VkMemoryPropertyFlags memoryPropertyFlags, bool generateMipMaps)
	:m_vulkanDevice(vulkanDevice)
{

	int width, height, channels;
	stbi_uc* pixels;
	VkDeviceSize imageSize;
	if (imgFormat == VK_FORMAT_R8G8B8A8_SRGB) 
	{
		pixels = stbi_load(texturePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
		if (!pixels)
		{
			//need to set default image/color
			std::cout << "failed to load image file\n";
		}
		imageSize = static_cast<uint64_t>(width * height * 4);
	}
	else
	{
		pixels = stbi_load(texturePath.c_str(), &width, &height, &channels, STBI_rgb);
		if (!pixels)
		{
			//need to set default image/color
			std::cout << "failed to load image file\n";
		}
		imageSize = static_cast<uint64_t>(width * height * channels);
	}
	uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
	mipLevels = generateMipMaps ? mipLevels : 1;
	//should have ability to give reference to an already exsiting staging buffer, ie one belonging to a render pass
	VulkanBuffer stagingBuffer(m_vulkanDevice, 1, imageSize, VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	stagingBuffer.map(0, imageSize);
	stagingBuffer.writeToBuffer(0, imageSize, pixels);
	stbi_image_free(pixels);

	m_vulkanImage = std::make_unique<VulkanImage>(m_vulkanDevice, static_cast<uint32_t>(width), static_cast<uint32_t>(height), imgFormat, imgTiling,
		imgUsageFlags, memoryPropertyFlags, mipLevels);

	m_vulkanImage->transitionImageLayout(imgFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	m_vulkanImage->copyBufferToImage(stagingBuffer, width, height);
	if (generateMipMaps) 
	{ 
		m_vulkanImage->generateMipMaps(static_cast<uint32_t>(width), static_cast<uint32_t>(height), mipLevels); 
	}
	else 
	{
		m_vulkanImage->transitionImageLayout(imgFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
}

Texture::~Texture()
{

}

VkImage Texture::getVkImage() const { return m_vulkanImage->getVkImage(); }
VkImageView Texture::getVkImageView() const { return m_vulkanImage->getVkImageView(); }
VkSampler Texture::getVkSampler() const { return m_vulkanImage->getVkSampler(); }