#pragma once

#include <memory>
#include <glm/gtx/hash.hpp>

#include "VulkanBuffer.h"

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;

	static VkVertexInputBindingDescription getBindingDescription();
	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();
	bool operator==(const Vertex& other) const;
};

struct vertexHasher { size_t operator()(Vertex const& vertex) const; };

class Mesh
{
public:
	Mesh(const Mesh&) = delete;
	const Mesh operator=(const Mesh&) = delete;

	Mesh(VulkanDevice& vulkanDevice, const std::vector<Vertex>& vertices, const std::vector<uint32_t> indices);
	~Mesh();

	void draw(VkCommandBuffer commandBuffer);

private:

	std::vector<Vertex> m_vertices;
	std::vector<uint32_t> m_indices;
	std::unique_ptr<VulkanBuffer> m_vertexBuffer;
	std::unique_ptr<VulkanBuffer> m_indexBuffer;
};

