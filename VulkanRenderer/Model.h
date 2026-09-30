#pragma once

#include <string>
#include <unordered_map>

#include "Mesh.h"

class Model
{
public:
	Model(const Model&) = delete;
	const Model& operator=(const Model&) = delete;

	//move operators allowed so class can be stored within a vector
	Model(Model&&) noexcept = default;
	Model& operator=(Model&&) noexcept = default;

	Model(VulkanDevice& vulkanDevice, const std::string filepath, size_t modelID);
	~Model();

	void draw(VkCommandBuffer commandBuffer);
	const Mesh& getMesh(size_t meshNumber = 0) const;
	size_t getModelID() const;

	static void createDefaultCubeModel(VulkanDevice& vulkanDevice, const std::string filepath);
	static Model& getDefaultCubeModel();

private:

	void loadModel(VulkanDevice& vulkanDevice, std::string filepath);

	std::vector<Mesh> m_meshes;
	size_t m_modelID = SIZE_MAX;//for now this ID is just the array index of the vector this model is stored in, should eventually change to a map

	//default model types
	static std::optional<Model> DEFAULT_MODEL_CUBE;
};

