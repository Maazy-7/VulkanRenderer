#pragma once

#include "VulkanRenderer.h"
#include "Texture.h"
#include "Camera.h"
#include "Light.h"

class DeferredRendererSystem
{

public:
	DeferredRendererSystem(const DeferredRendererSystem&) = delete;
	DeferredRendererSystem& operator=(const DeferredRendererSystem&) = delete;

	DeferredRendererSystem(VulkanDevice& vulkanDevice, const VulkanRenderer& vulkanRenderer, Camera& camera);
	~DeferredRendererSystem();

	void createPipelines(VulkanDescriptorSetLayout& descriptorSetLayout);

	void executeGeometryPass();
	void executeLightingPass();
	void setLightLists(std::vector<Light>* m_lights);

	VulkanImage& getDepthBuffer();
	VulkanImage& getLightingPassOutput();

private:
	VulkanDevice& m_vulkanDevice;
	const VulkanRenderer& m_vulkanRenderer;
	std::unique_ptr<VulkanPipeline> m_geometryPassPipeline;
	std::unique_ptr<VulkanPipeline> m_lightingPassPipeline;
	std::unique_ptr<VulkanDescriptorPool> m_vulkanDescriptorPool;
	std::vector<VkDescriptorSet> m_geometryPassDescriptorSets;
	std::vector<VkDescriptorSet> m_lightingPassDescriptorSets;
	std::unique_ptr<VulkanDescriptorSetLayout> m_geometryPassSetLayout;
	std::unique_ptr<VulkanDescriptorSetLayout> m_lightingPassSetLayout;
	std::unique_ptr<VulkanImage> m_gDepth;
	std::unique_ptr<VulkanImage> m_gNormal;
	std::unique_ptr<VulkanImage> m_gAlbedoSpec;
	std::unique_ptr<VulkanImage> m_lightingPassOutput;
	std::vector<Light>* m_lightsList;
	Camera& m_camera;
	//temp
	std::unique_ptr<Texture> specularTexture;
	std::unique_ptr<Texture> diffuseTexture;
};

