#include "Model.h"

#define TINYOBJLOADER_IMPLEMENTATION
#define TINYOBJLOADER_DISABLE_FAST_FLOAT
#include <tiny_obj_loader/tiny_obj_loader.h>

Model::Model(VulkanDevice& vulkanDevice, const std::string filepath)
{
	loadModel(vulkanDevice, filepath);
}

Model::~Model() 
{

}

void Model::loadModel(VulkanDevice& vulkanDevice, const std::string filepath)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;
	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, nullptr, &err, filepath.c_str()))
	{
		throw std::runtime_error(err);
	}

	std::unordered_map<Vertex, uint32_t, vertexHasher> uniqueVertices;

	for (const auto& shape : shapes) //for each mesh per model
	{
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		for (const auto& index : shape.mesh.indices) 
		{
			Vertex vertex{};
			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			vertex.texCoord = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			vertex.color = { 1.f, 1.f, 1.f };

			if (uniqueVertices.count(vertex) == 0) 
			{
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}

			indices.push_back(uniqueVertices[vertex]);
		}

		m_meshes.push_back(std::make_unique<Mesh>(vulkanDevice, vertices, indices));
	}
}

void Model::draw(VkCommandBuffer commandBuffer) 
{
	for (const auto& mesh : m_meshes) 
	{
		mesh->draw(commandBuffer);
	}
}
