#include "VulkanImage.h"

VulkanImage::VulkanImage(VulkanDevice& vulkanDevice, uint32_t width, uint32_t height, VkFormat format, 
	VkImageTiling imgTilling, VkImageUsageFlags imgUsage, VkMemoryPropertyFlags memoryPropertyFlags)
	:m_vulkanDevice(vulkanDevice)
{
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
}

VulkanImage::~VulkanImage() 
{

}