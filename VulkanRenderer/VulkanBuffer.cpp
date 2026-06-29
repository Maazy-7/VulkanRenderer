#include "VulkanBuffer.h"


VkVertexInputBindingDescription Vertex::getBindingDescription()
{
	VkVertexInputBindingDescription bindingDescription{};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(Vertex);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 2> Vertex::getAttributeDescriptions()
{
	std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(Vertex, pos);

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Vertex, color);
	return attributeDescriptions;
}

VulkanBuffer::VulkanBuffer(VulkanDevice& device, VkDeviceSize elementCount, VkDeviceSize size, 
	VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
	:m_vkDevice(device), m_count((uint32_t)elementCount), m_buffSize(size)
{
	VkBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = size;
	bufferCreateInfo.usage = usage;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(m_vkDevice.getDevice(), &bufferCreateInfo, nullptr, &m_vkBuffer) != VK_SUCCESS) 
	{
		throw std::runtime_error("failed to create vulkan vertex buffer");
	}

	VkMemoryRequirements memoryRequirements{};
	vkGetBufferMemoryRequirements(m_vkDevice.getDevice(), m_vkBuffer, &memoryRequirements);

	VkMemoryAllocateInfo memoryAllocateInfo{};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = m_vkDevice.findMemoryType(memoryRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(m_vkDevice.getDevice(), &memoryAllocateInfo, nullptr, &m_vkDeviceMemory) != VK_SUCCESS) 
	{
		throw std::runtime_error("failed to allocate memory to vulkan buffer");
	}

	vkBindBufferMemory(m_vkDevice.getDevice(), m_vkBuffer, m_vkDeviceMemory, 0);
}

VulkanBuffer::~VulkanBuffer() 
{
	vkDestroyBuffer(m_vkDevice.getDevice(), m_vkBuffer, nullptr);
	vkFreeMemory(m_vkDevice.getDevice(), m_vkDeviceMemory, nullptr);
}

void VulkanBuffer::bindVertexBuffer(VkCommandBuffer commandBuffer) const
{
	VkBuffer vBuffers[] = { m_vkBuffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vBuffers, offsets);
}

void VulkanBuffer::bindIndexBuffer(VkCommandBuffer commandBuffer) const
{
	VkBuffer vBuffers[] = { m_vkBuffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindIndexBuffer(commandBuffer, m_vkBuffer, 0, VK_INDEX_TYPE_UINT16);
}

uint32_t VulkanBuffer::getBufferElementCount() const 
{
	return m_count;
}

VkBuffer VulkanBuffer::getBuffer() const 
{
	return m_vkBuffer;
}

VulkanDevice& VulkanBuffer::getVulkanDevice() const 
{
	return m_vkDevice;
}

void VulkanBuffer::map(VkDeviceSize offset, VkDeviceSize size) 
{
	vkMapMemory(m_vkDevice.getDevice(), m_vkDeviceMemory, offset, size, 0, &m_mapped);
}

void VulkanBuffer::writeToBuffer(VkDeviceSize offset, VkDeviceSize size, const void* src) 
{
	memcpy(m_mapped, src, size);//need to implement buffer write with offset
}

void VulkanBuffer::unMap() 
{
	if (m_mapped) 
	{
		vkUnmapMemory(m_vkDevice.getDevice(), m_vkDeviceMemory);
		m_mapped = nullptr;
	}
}

void VulkanBuffer::copyBuffer(VulkanBuffer& srcBuffer, VulkanBuffer& dstBuffer, VkDeviceSize size)
{
	VkCommandBuffer commandBuffer = srcBuffer.m_vkDevice.beginSingleTimeCommands();

	VkBufferCopy copyRegion{};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer.getBuffer(), dstBuffer.getBuffer(), 1, &copyRegion);

	srcBuffer.m_vkDevice.endSingleTimeCommands(commandBuffer);
}