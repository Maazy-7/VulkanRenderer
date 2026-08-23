#pragma once

#include "VulkanRenderer.h"
#include "Camera.h"
#include "gameObject.h"
#include "ShadowCaster.h"

class ShadowRendererSystem
{
public:

	ShadowRendererSystem(const ShadowRendererSystem&) = delete;
	ShadowRendererSystem& operator=(const ShadowRendererSystem&) = delete;

	ShadowRendererSystem(VulkanDevice& vulkanDevice, std::vector<Model>& models);
	~ShadowRendererSystem();

	void renderShadowPass(VkCommandBuffer commandBuffer);
	void setGameObjectList(std::vector<gameObject>* gameObjects);
	void setLightLists(std::vector<Light>* lights);
	void createShadowCaster(Light& light, uint32_t shadowWidth, uint32_t shadowHeight, float nearPlane = 0.1f, float farPlane = 25.f);

	const std::vector<ShadowCaster>* getShadowCasters() const;

private:
	VulkanDevice& m_vulkanDevice;
	std::unique_ptr<VulkanPipeline> m_shadowMapRenderingPipeline;
	std::unique_ptr<VulkanPipeline> m_cubeMapShadowRenderingPipeline;
	std::unique_ptr<VulkanDescriptorPool> m_ShadowRendererDescriptorPool;
	std::unique_ptr<VulkanDescriptorSetLayout> m_cubemapShadowMatrixDescriptorSetLayout;
	std::optional<VulkanBuffer> m_cubemapMatrixBuffer;
	VkDescriptorSet m_shadowMatrixDescriptorSet;
	std::unique_ptr<VulkanImage> m_shadowMap;
	std::vector<ShadowCaster> m_shadowCasters;
	std::vector<Model>& m_models;
	std::vector<gameObject>* m_gameObjects = nullptr;
	std::vector<Light>* m_lights = nullptr;
};

