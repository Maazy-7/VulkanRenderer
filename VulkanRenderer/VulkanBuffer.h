#pragma once

#include <glm/glm.hpp>
#include <array>
#include <vector>

#include "VulkanDevice.h"

class VulkanBuffer
{
public:
	VulkanBuffer(const VulkanBuffer&) = delete;
	VulkanBuffer& operator=(const VulkanBuffer&) = delete;

	//move constructor
	VulkanBuffer(VulkanBuffer&& other) noexcept;

	VulkanBuffer(VulkanDevice& device, VkDeviceSize elementCount, VkDeviceSize size, 
		VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
	~VulkanBuffer();

	void bindVertexBuffer(VkCommandBuffer commandBuffer) const;
	void bindIndexBuffer(VkCommandBuffer commandBuffer) const;
	void map(VkDeviceSize offset, VkDeviceSize size);
	void writeToBuffer(VkDeviceSize offset, VkDeviceSize size, const void* src);
	void unMap();
	uint32_t getBufferElementCount() const;
	VkBuffer getBuffer() const;
	VulkanDevice& getVulkanDevice() const;

	static void copyBuffer(VulkanBuffer& srcBuffer, VulkanBuffer& dstBuffer, VkDeviceSize size);

private:

	VulkanDevice& m_vkDevice;
	void* m_mapped = nullptr;
	VkBuffer m_vkBuffer = VK_NULL_HANDLE;
	VkDeviceMemory m_vkDeviceMemory = VK_NULL_HANDLE;
	uint32_t m_count;
	VkDeviceSize m_buffSize;
};

