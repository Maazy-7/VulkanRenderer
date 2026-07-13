#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <chrono>
#include <stb/stb_image.h>

#include "Window.h"
#include "VulkanDevice.h"
#include "VulkanRenderer.h"
#include "VulkanPipeline.h"
#include "VulkanBuffer.h"
#include "Camera.h"
#include "Model.h"

struct UniformBufferObject {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

class Application; //friendship

class SimpleRenderSystem
{

public:

	SimpleRenderSystem(const SimpleRenderSystem&) = delete;
	SimpleRenderSystem& operator=(const SimpleRenderSystem&) = delete;

	SimpleRenderSystem(VulkanDevice& device, const VulkanRenderer& renderer, VulkanDescriptorPool& descriptorPool);

	void renderGameObjects(VkCommandBuffer commandBuffer, float dt);

	std::vector<VkDescriptorSet>& getDescriptorSets();
	VulkanPipeline& getVulkanPipeline() const;

private:

	void createPipeline(VulkanDescriptorSetLayout& vulkanDescriptorSetLayout);

	VulkanDevice& m_vkDevice;
	const VulkanRenderer& m_vulkanRenderer;
	VulkanDescriptorPool& m_vulkanDescriptorPool;
	std::unique_ptr<VulkanPipeline> m_vkPipeline;
	std::vector<std::unique_ptr<VulkanBuffer>> m_uniformBuffers;
	std::unique_ptr<VulkanImage> m_texture;
	std::vector<VkDescriptorSet> m_vkDescriptorSets;
	std::unique_ptr<VulkanDescriptorSetLayout> m_renderSystemDescriptorSetLayout;
	UniformBufferObject ubo;
	Camera m_camera;
	std::unique_ptr<Model> m_model;

	friend class Application;
};

