#pragma once

#include "VulkanRenderer.h"
#include "Camera.h"

struct PosOnlyVertex //struct to hold a vertex object with just vertex position, used for shadow maps ie depth rendering
{
	glm::vec3 pos;

	static VkVertexInputBindingDescription getBindingDescription();
	static VkVertexInputAttributeDescription getAttributeDescription();
	bool operator==(const PosOnlyVertex& other) const;
};

class ShadowRendererSystem
{
public:

	ShadowRendererSystem(const ShadowRendererSystem&) = delete;
	ShadowRendererSystem& operator=(const ShadowRendererSystem&) = delete;

	ShadowRendererSystem(VulkanDevice& vulkanDevice, Camera& camera, std::vector<Model>& models);
	~ShadowRendererSystem();

	void renderShadowPass(VkCommandBuffer commandBuffer, const glm::mat4* lightSpaceMatrix);

	VulkanImage& getShadowMap() const;

private:
	VulkanDevice& m_vulkanDevice;
	Camera& m_camera;
	std::unique_ptr<VulkanPipeline> m_shadowMapRenderingPipeline;
	std::unique_ptr<VulkanImage> m_shadowMap;
	std::vector<Model>& m_models;
};

