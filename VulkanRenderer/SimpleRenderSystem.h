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
#include "gameObject.h"
#include "ShadowRendererSystem.h"

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
class VulkanRenderingBackend; //friendship

class SimpleRenderSystem
{

public:

	SimpleRenderSystem(const SimpleRenderSystem&) = delete;
	SimpleRenderSystem& operator=(const SimpleRenderSystem&) = delete;

	SimpleRenderSystem(VulkanDevice& device, const VulkanRenderer& renderer, VulkanDescriptorPool& descriptorPool, const Camera* camera);

	void renderGameObjects(VkCommandBuffer commandBuffer, float dt);

	std::vector<VkDescriptorSet>& getDescriptorSets();
	VulkanPipeline& getVulkanPipeline() const;
	std::vector<Model>& getModels();
	void setShadowMap(VulkanImage& vulkanImage);//could put in constructor but for now will just pass it through this function
	void setGameObjectList(std::vector<gameObject>* gameObjects);
	void attachCamera(const Camera* camera);

private:

	void createPipeline(VulkanDescriptorSetLayout& vulkanDescriptorSetLayout);

	VulkanDevice& m_vkDevice;
	const VulkanRenderer& m_vulkanRenderer;
	VulkanDescriptorPool& m_vulkanDescriptorPool;
	std::unique_ptr<VulkanPipeline> m_vkPipeline;
	std::unique_ptr<VulkanPipeline> m_singleColorObjectPipeline;
	std::unique_ptr<Texture> m_texture;
	std::vector<VkDescriptorSet> m_vkDescriptorSets;
	VulkanImage* m_shadowMap = nullptr;
	std::unique_ptr<VulkanDescriptorSetLayout> m_renderSystemDescriptorSetLayout;
	UniformBufferObject ubo;
	glm::mat4 cubeMat = glm::scale(glm::mat4(1.f), glm::vec3(0.5f));
	const Camera* m_camera = nullptr;
	std::vector<Model> m_models;
	std::vector<gameObject>* m_gameObjects = nullptr;

	friend class Application;
	friend class VulkanRenderingBackend;
};

