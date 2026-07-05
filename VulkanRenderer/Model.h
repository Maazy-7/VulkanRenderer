#pragma once

#include <string>
#include <unordered_map>

#include "Mesh.h"

class Model
{
public:
	Model(const Model&) = delete;
	const Model& operator=(const Model&) = delete;

	Model(VulkanDevice& vulkanDevice, const std::string filepath);
	~Model();

	void draw(VkCommandBuffer commandBuffer);

private:

	void loadModel(VulkanDevice& vulkanDevice, std::string filepath);

	std::vector<std::unique_ptr<Mesh>> m_meshes;
};

