#include "VulkanBuffer.h"

VulkanBuffer::VulkanBuffer(VulkanBuffer&& other) noexcept
	: m_vkDevice(other.m_vkDevice),
	m_mapped(other.m_mapped),
	m_vkBuffer(other.m_vkBuffer),
	m_vkDeviceMemory(other.m_vkDeviceMemory),
	m_count(other.m_count),
	m_buffSize(other.m_buffSize)
{
	other.m_mapped = nullptr;
	other.m_vkBuffer = VK_NULL_HANDLE;
	other.m_vkDeviceMemory = VK_NULL_HANDLE;
	other.m_count = 0;
	other.m_buffSize = 0;
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

	if ((usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) == VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) 
	{
		VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo{};
		memoryAllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
		memoryAllocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
		memoryAllocateInfo.pNext = &memoryAllocateFlagsInfo;
	}

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
	vkCmdBindIndexBuffer(commandBuffer, m_vkBuffer, 0, VK_INDEX_TYPE_UINT32);
}

uint32_t VulkanBuffer::getBufferElementCount() const 
{
	return m_count;
}

VkBuffer VulkanBuffer::getBuffer() const 
{
	return m_vkBuffer;
}

VkDeviceAddress VulkanBuffer::getBufferDeviceAddress() const
{
	VkBufferDeviceAddressInfo bufferAddressDeviceInfo{};
	bufferAddressDeviceInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	bufferAddressDeviceInfo.buffer = m_vkBuffer;
	return vkGetBufferDeviceAddress(m_vkDevice.getDevice(), &bufferAddressDeviceInfo);
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
	uint8_t* dst = static_cast<uint8_t*>(m_mapped);
	memcpy(dst + offset, src, size);
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

	VkBufferCopy2 copyRegion{};
	copyRegion.sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2;
	copyRegion.size = size;

	VkCopyBufferInfo2 copyBufferInfo2{};
	copyBufferInfo2.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2;
	copyBufferInfo2.srcBuffer = srcBuffer.getBuffer();
	copyBufferInfo2.dstBuffer = dstBuffer.getBuffer();
	copyBufferInfo2.regionCount = 1;
	copyBufferInfo2.pRegions = &copyRegion;

	vkCmdCopyBuffer2(commandBuffer, &copyBufferInfo2);

	srcBuffer.m_vkDevice.endSingleTimeCommands(commandBuffer);
}