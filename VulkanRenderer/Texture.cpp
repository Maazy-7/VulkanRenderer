#include "Texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

Texture::Texture(VulkanDevice& vulkanDevice, const std::string& texturePath, VkFormat imgFormat, VkImageTiling imgTiling,
	VkImageUsageFlags imgUsageFlags, VkMemoryPropertyFlags memoryPropertyFlags, bool generateMipMaps)
	:m_vulkanDevice(vulkanDevice)
{

	int width, height, channels;
	stbi_uc* pixels;
	std::vector<stbi_uc> defaultColor = { 255, 0, 255 };
	VkDeviceSize imageSize;
	if (imgFormat == VK_FORMAT_R8G8B8A8_SRGB) 
	{
		pixels = stbi_load(texturePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
		if (!pixels)
		{
			std::cout << "failed to load image file" << texturePath << "\n";
			width = 1; height = 1; channels = 4;
			pixels = defaultColor.data();
			generateMipMaps = false;
		}
		imageSize = static_cast<uint64_t>(width * height * 4);
	}
	else
	{
		pixels = stbi_load(texturePath.c_str(), &width, &height, &channels, STBI_rgb);
		if (!pixels)
		{
			std::cout << "failed to load image file" << texturePath << "\n";
			width = 1; height = 1; channels = 3;
			pixels = defaultColor.data();
			generateMipMaps = false;
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
	if (!pixels) { stbi_image_free(pixels); }//only freeing if image data was actually loaded
	imgUsageFlags = generateMipMaps ? imgUsageFlags | VK_IMAGE_USAGE_TRANSFER_SRC_BIT : imgUsageFlags;//giving extra flag as src bit in case of mipmap generation
	m_vulkanImage.emplace(m_vulkanDevice, static_cast<uint32_t>(width), static_cast<uint32_t>(height), imgFormat, imgTiling,
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

Texture::Texture(VulkanDevice& vulkanDevice, const std::string& texturePath, VkFormat imgFormat, bool generateMipMaps)
	:Texture(vulkanDevice, texturePath, imgFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, 
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, generateMipMaps)
{
	//default texture constructor, makes optimal tiling, 8 bit sRGBA format, image usage for sampling and transfer destination, also src transfer if
	// generateMipMaps is true
}

Texture::Texture(VulkanDevice& vulkanDevice, uint8_t r, uint8_t g, uint8_t b, VkFormat imgFormat)//a 1x1 singular color texture, can be used for default material/normal maps
	:m_vulkanDevice(vulkanDevice)
{
	uint8_t width = 1;//1x1 dimensions - 1 singular pixel color
	uint8_t height = 1;
	int imageSize = 4;//1 pixel 4 channels
	uint8_t pixels[4] = {r, g, b, 1};
	VulkanBuffer stagingBuffer(m_vulkanDevice, 1, imageSize, VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	stagingBuffer.map(0, imageSize);
	stagingBuffer.writeToBuffer(0, imageSize, pixels);
	m_vulkanImage.emplace(m_vulkanDevice, width, height, imgFormat, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	m_vulkanImage->transitionImageLayout(imgFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	m_vulkanImage->copyBufferToImage(stagingBuffer, width, height);
}

Texture::~Texture()
{

}

VkImage Texture::getVkImage() const { return m_vulkanImage->getVkImage(); }
VkImageView Texture::getVkImageView() const { return m_vulkanImage->getVkImageView(); }
VkSampler Texture::getVkSampler() const { return m_vulkanImage->getVkSampler(); }