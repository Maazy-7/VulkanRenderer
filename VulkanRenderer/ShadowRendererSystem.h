#pragma once

#include "VulkanRenderer.h"
#include "Camera.h"
#include "gameObject.h"

class ShadowRendererSystem
{
public:

	ShadowRendererSystem(const ShadowRendererSystem&) = delete;
	ShadowRendererSystem& operator=(const ShadowRendererSystem&) = delete;

	ShadowRendererSystem(VulkanDevice& vulkanDevice, std::vector<Model>& models);
	~ShadowRendererSystem();

	void renderShadowPass(VkCommandBuffer commandBuffer, const glm::mat4* lightSpaceMatrix);
	void setGameObjectList(std::vector<gameObject>* gameObjects);

	VulkanImage& getShadowMap() const;

private:
	VulkanDevice& m_vulkanDevice;
	std::unique_ptr<VulkanPipeline> m_shadowMapRenderingPipeline;
	std::unique_ptr<VulkanImage> m_shadowMap;
	std::vector<Model>& m_models;
	std::vector<gameObject>* m_gameObjects;
};

