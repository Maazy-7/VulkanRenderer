#pragma once

#include <glm/glm.hpp>
#include <array>
#include <vector>

#include "VulkanDevice.h"

struct Vertex 
{
	glm::vec2 pos;
	glm::vec3 color;

	static VkVertexInputBindingDescription getBindingDescription();
	static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions();
};

class VulkanBuffer
{
public:
	VulkanBuffer(const VulkanBuffer&) = delete;
	VulkanBuffer& operator=(const VulkanBuffer&) = delete;

	VulkanBuffer(VulkanDevice& device, const std::vector<Vertex>& vertices);
	~VulkanBuffer();

	void bindBuffer(VkCommandBuffer commandBuffer) const;
	uint32_t getBufferElementCount() const;

private:

	VulkanDevice& m_vkDevice;
	void* m_mapped = nullptr;
	VkBuffer m_vkBuffer = VK_NULL_HANDLE;
	VkDeviceMemory m_vkDeviceMemory = VK_NULL_HANDLE;
	uint32_t count;
};

