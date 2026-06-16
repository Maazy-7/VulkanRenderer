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

VulkanBuffer::VulkanBuffer(VulkanDevice& device, const std::vector<Vertex>& vertices) 
	:m_vkDevice(device), count((uint32_t)vertices.size())
{
	VkBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = sizeof(vertices[0]) * vertices.size();
	bufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
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
	memoryAllocateInfo.memoryTypeIndex = m_vkDevice.findMemoryType(memoryRequirements.memoryTypeBits, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	if (vkAllocateMemory(m_vkDevice.getDevice(), &memoryAllocateInfo, nullptr, &m_vkDeviceMemory) != VK_SUCCESS) 
	{
		throw std::runtime_error("failed to allocate memory to vulkan buffer");
	}

	vkBindBufferMemory(m_vkDevice.getDevice(), m_vkBuffer, m_vkDeviceMemory, 0);

	vkMapMemory(m_vkDevice.getDevice(), m_vkDeviceMemory, 0, bufferCreateInfo.size, 0, &m_mapped);
	memcpy(m_mapped, vertices.data(), (size_t)bufferCreateInfo.size);
	vkUnmapMemory(m_vkDevice.getDevice(), m_vkDeviceMemory);
}

VulkanBuffer::~VulkanBuffer() 
{
	vkDestroyBuffer(m_vkDevice.getDevice(), m_vkBuffer, nullptr);
	vkFreeMemory(m_vkDevice.getDevice(), m_vkDeviceMemory, nullptr);
}

void VulkanBuffer::bindBuffer(VkCommandBuffer commandBuffer) const
{
	VkBuffer vBuffers[] = { m_vkBuffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vBuffers, offsets);
}

uint32_t VulkanBuffer::getBufferElementCount() const 
{
	return count;
}