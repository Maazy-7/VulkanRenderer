#pragma once

#include "VulkanRenderer.h"
#include "Texture.h"
#include "Camera.h"

class DeferredRendererSystem
{

public:
	DeferredRendererSystem(const DeferredRendererSystem&) = delete;
	DeferredRendererSystem& operator=(const DeferredRendererSystem&) = delete;

	DeferredRendererSystem();
	~DeferredRendererSystem();

private:
	VulkanDevice& m_vulkanDevice;
	VulkanRenderer& m_vulkanRenderer;
	std::unique_ptr<VulkanPipeline> m_vulkanPipeline;
	VulkanDescriptorPool& m_vulkanDescriptorPool;
	std::vector<VkDescriptorSet> m_vkDescriptorSets;
	std::unique_ptr<VulkanDescriptorSetLayout> m_deferredRenderSystemSetLayout;
	std::unique_ptr<Texture> m_gPosition;
	std::unique_ptr<Texture> m_gNormal;
	std::unique_ptr<Texture> m_gAlbedoSpec;
	Camera& camera;
};

