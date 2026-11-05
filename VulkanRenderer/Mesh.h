#pragma once

#include <memory>
#include <glm/gtx/hash.hpp>

#include "VulkanBuffer.h"

struct Vertex
{
	glm::vec4 pos;//w component is padding
	glm::vec4 normal;//w component is padding
	glm::vec4 texCoord;//z and w components are padding
	glm::vec4 tangent;//w component is padding

	static VkVertexInputBindingDescription getBindingDescription();
	static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions();
	bool operator==(const Vertex& other) const;
};

struct vertexHasher { size_t operator()(Vertex const& vertex) const; };

class Mesh
{
public:
	Mesh(const Mesh&) = delete;
	Mesh& operator=(const Mesh&) = delete;

	//move constructor and assignment
	Mesh(Mesh&&) noexcept = default;
	Mesh& operator=(Mesh&&) noexcept = default;

	Mesh(VulkanDevice& vulkanDevice, const std::vector<Vertex>& vertices, const std::vector<uint32_t> indices);
	~Mesh();

	void draw(VkCommandBuffer commandBuffer) const;
	const VulkanBuffer& getMeshVertexBuffer() const;
	const VulkanBuffer& getMeshIndexBuffer() const;

private:
	//void calculateAABB();
	
	std::optional<VulkanBuffer> m_vertexBuffer;
	std::optional<VulkanBuffer> m_indexBuffer;
};

