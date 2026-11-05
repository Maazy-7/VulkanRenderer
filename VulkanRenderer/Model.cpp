#include "Model.h"

#define TINYOBJLOADER_IMPLEMENTATION
#define TINYOBJLOADER_DISABLE_FAST_FLOAT
#include <tiny_obj_loader/tiny_obj_loader.h>

std::optional<Model> Model::DEFAULT_MODEL_CUBE;

Model::Model(VulkanDevice& vulkanDevice, const std::string filepath, size_t modelID)
	:m_modelID(modelID)
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
				attrib.vertices[3 * index.vertex_index + 2],
				0//padding
			};

			vertex.texCoord = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.f - attrib.texcoords[2 * index.texcoord_index + 1],
				0,//padding 
				0//padding
			};

			vertex.normal = { 
				attrib.normals[3 * index.normal_index + 0],
				attrib.normals[3 * index.normal_index + 1],
				attrib.normals[3 * index.normal_index + 2],
				0//
			};

			vertex.tangent = { 0.f, 0.f, 0.f, 0.f };

			if (uniqueVertices.count(vertex) == 0) 
			{
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}

			indices.push_back(uniqueVertices[vertex]);
		}
		
		//second pass over created vertices to compute then accumulate tangents for each vertex
		for (size_t i = 0; i < indices.size(); i+= 3) 
		{

			uint32_t index0 = indices[i + 0];
			uint32_t index1 = indices[i + 1];
			uint32_t index2 = indices[i + 2];

			Vertex& v0 = vertices[index0];
			Vertex& v1 = vertices[index1];
			Vertex& v2 = vertices[index2];

			glm::vec3 edge1 = glm::vec3(v1.pos - v0.pos);
			glm::vec3 edge2 = glm::vec3(v2.pos - v0.pos);

			glm::vec2 deltaUV1 = v1.texCoord - v0.texCoord;
			glm::vec2 deltaUV2 = v2.texCoord - v0.texCoord;

			float f = 1.f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);

			glm::vec4 tangent{};
			tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
			tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
			tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

			v0.tangent += tangent;
			v1.tangent += tangent;
			v2.tangent += tangent;
		}

		m_meshes.emplace_back(vulkanDevice, vertices, indices);
	}
}

void Model::draw(VkCommandBuffer commandBuffer) 
{
	for (const auto& mesh : m_meshes) 
	{
		mesh.draw(commandBuffer);
	}
}

const Mesh& Model::getMesh(size_t meshNumber) const
{
	return m_meshes[meshNumber];
}

size_t Model::getModelID() const
{
	return m_modelID;
}

void Model::createDefaultCubeModel(VulkanDevice& vulkanDevice, const std::string filepath)
{
	DEFAULT_MODEL_CUBE = Model(vulkanDevice, filepath, SIZE_MAX);//need to add this and other default models to the main model list
}

Model& Model::getDefaultCubeModel()
{
	return DEFAULT_MODEL_CUBE.value();
}
