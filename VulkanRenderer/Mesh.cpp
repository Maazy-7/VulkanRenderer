#include "Mesh.h"

VkVertexInputBindingDescription Vertex::getBindingDescription()
{
	VkVertexInputBindingDescription bindingDescription{};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(Vertex);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 3> Vertex::getAttributeDescriptions()
{
	std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(Vertex, pos);

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Vertex, color);

	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

	return attributeDescriptions;
}

bool Vertex::operator==(const Vertex& other) const
{
	return pos == other.pos && color == other.color && texCoord == other.texCoord;
}

size_t vertexHasher::operator()(Vertex const& vertex) const
{
	return ((std::hash<glm::vec3>()(vertex.pos) ^
		(std::hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
		(std::hash<glm::vec2>()(vertex.texCoord) << 1);
}

Mesh::Mesh(VulkanDevice& vulkanDevice, const std::vector<Vertex>& vertices, const std::vector<uint32_t> indices)
	:m_vertices(vertices), m_indices(indices)
{
	VkDeviceSize verticesSize = sizeof(m_vertices[0]) * m_vertices.size();
	VkDeviceSize indicesSize = sizeof(m_indices[0]) * m_indices.size();
	
	VulkanBuffer stagingBuffer(vulkanDevice, m_vertices.size(), verticesSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	m_vertexBuffer = std::make_unique<VulkanBuffer>(vulkanDevice, m_vertices.size(), verticesSize, 
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_indexBuffer = std::make_unique<VulkanBuffer>(vulkanDevice, m_indices.size(), indicesSize, 
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	//allocating vertex buffer using a staging buffer
	stagingBuffer.map(0, verticesSize);
	stagingBuffer.writeToBuffer(0, verticesSize, m_vertices.data());
	stagingBuffer.unMap();
	VulkanBuffer::copyBuffer(stagingBuffer, *m_vertexBuffer.get(), verticesSize);

	//allocating index buffer using staging buffer
	stagingBuffer.map(0, indicesSize);
	stagingBuffer.writeToBuffer(0, indicesSize, m_indices.data()); //should make a global staging buffer
	stagingBuffer.unMap();
	VulkanBuffer::copyBuffer(stagingBuffer, *m_indexBuffer.get(), indicesSize);
	
}

Mesh::~Mesh()
{

}

void Mesh::draw(VkCommandBuffer commandBufer)
{
	m_vertexBuffer->bindVertexBuffer(commandBufer);
	m_indexBuffer->bindIndexBuffer(commandBufer);
	vkCmdDrawIndexed(commandBufer, m_indexBuffer->getBufferElementCount(), 1, 0, 0, 0);
}
