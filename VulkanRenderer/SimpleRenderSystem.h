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
#include "Texture.h"
#include "Model.h"

struct UniformBufferObject {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

struct LightData
{
	alignas(16) glm::vec4 lightPos;
	alignas(16) glm::vec4 viewPos;
};

class Application; //friendship

class SimpleRenderSystem
{

public:

	SimpleRenderSystem(const SimpleRenderSystem&) = delete;
	SimpleRenderSystem& operator=(const SimpleRenderSystem&) = delete;

	SimpleRenderSystem(VulkanDevice& device, const VulkanRenderer& renderer, VulkanDescriptorPool& descriptorPool, Camera& camera);

	void renderGameObjects(VkCommandBuffer commandBuffer, float dt);

	std::vector<VkDescriptorSet>& getDescriptorSets();
	VulkanPipeline& getVulkanPipeline() const;

private:

	void createPipeline(VulkanDescriptorSetLayout& vulkanDescriptorSetLayout);

	VulkanDevice& m_vkDevice;
	const VulkanRenderer& m_vulkanRenderer;
	VulkanDescriptorPool& m_vulkanDescriptorPool;
	std::unique_ptr<VulkanPipeline> m_vkPipeline;
	std::unique_ptr<Texture> m_texture;
	std::vector<VkDescriptorSet> m_vkDescriptorSets;
	std::unique_ptr<VulkanDescriptorSetLayout> m_renderSystemDescriptorSetLayout;
	UniformBufferObject ubo;
	Camera& m_camera;
	std::unique_ptr<Model> m_model;

	friend class Application;
};

