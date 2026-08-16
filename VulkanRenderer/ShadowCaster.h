#pragma once

#include "VulkanImage.h"
#include "Light.h"

class ShadowRendererSystem;//friendship

class ShadowCaster
{
public:
	ShadowCaster(const ShadowCaster&) = delete;
	ShadowCaster operator=(const ShadowCaster&) = delete;

	//move constructor
	ShadowCaster(ShadowCaster&& other) noexcept = default;

	ShadowCaster(VulkanDevice& vulkanDevice, Light& light, uint32_t shadowWidth, uint32_t shadowHeight, float nearPlane = 0.1f, float farPlane = 25.f);
	~ShadowCaster();

	uint32_t getShadowWidth() const;
	uint32_t getShadowHeight() const;
	const VulkanImage& getShadowMap() const;

private:
	Light& m_light;
	uint32_t m_shadowWidth;
	uint32_t m_shadowHeight;
	float m_nearPlane;
	float m_farPlane;
	std::vector<glm::mat4> m_lightSpaceMatrix;//either a singular matrix or 6 for cubemap shadows
	std::optional<VulkanImage> m_shadowMap;
	VkImageView m_cubemapLayerRenderImageView;//image view that hold view type of 2d image array so that it can be accessed within the geometry shader through layers

	friend ShadowRendererSystem;
};

