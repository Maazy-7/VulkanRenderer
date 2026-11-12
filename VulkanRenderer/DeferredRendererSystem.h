#pragma once

#include "VulkanRenderer.h"
#include "Texture.h"
#include "Camera.h"
#include "Light.h"
#include "ShadowCaster.h"
#include "gameObject.h"

//temp
struct QuadVertex
{
	glm::vec2 pos;
	glm::vec2 texCoords;

	static VkVertexInputBindingDescription getBindingDescription();
	static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions();
	bool operator==(const Vertex& other) const;
};

class DeferredRendererSystem
{

public:
	DeferredRendererSystem(const DeferredRendererSystem&) = delete;
	DeferredRendererSystem& operator=(const DeferredRendererSystem&) = delete;

	DeferredRendererSystem(VulkanDevice& vulkanDevice, const VulkanRenderer& vulkanRenderer);
	~DeferredRendererSystem();

	void createPipelines(VulkanDescriptorSetLayout& descriptorSetLayout, VulkanDescriptorSetLayout& globalTextureDescriptorSetLayout);

	void executeGeometryPass(VkCommandBuffer commandBuffer);
	void executeLightingPass(VkCommandBuffer commandBuffer);
	void renderLights(VkCommandBuffer commandBuffer, VulkanImage& outputImage);
	void setGameObjectList(std::vector<gameObject>* gameObjects);
	void setLightList(std::vector<Light>* lights);
	void setShadowCasterList(const std::vector<ShadowCaster>* shadowCasters);
	void attachCamera(const Camera* camera);
	void recreateDeferredRendererResources(); //recreates the geometry pass and lighting pass render targets and their descriptors

	VulkanImage& getDepthBuffer();
	VulkanImage& getNormalMetallicBuffer();
	VulkanImage& getAlbedoRoughnessBuffer();
	VulkanImage& getLightingPassOutput();
	VulkanPipeline& getGeometryPassPipeline();

private:
	VulkanDevice& m_vulkanDevice;
	const VulkanRenderer& m_vulkanRenderer;
	std::unique_ptr<VulkanPipeline> m_geometryPassPipeline;
	std::unique_ptr<VulkanPipeline> m_lightingPassPipeline;
	std::unique_ptr<VulkanPipeline> m_lightObjRenderingPipeline;
	std::unique_ptr<VulkanDescriptorPool> m_vulkanDescriptorPool;
	std::vector<VkDescriptorSet> m_geometryPassDescriptorSets;
	std::vector<VkDescriptorSet> m_lightingPassDescriptorSets;
	std::unique_ptr<VulkanDescriptorSetLayout> m_geometryPassSetLayout;
	std::unique_ptr<VulkanDescriptorSetLayout> m_lightingPassSetLayout;
	std::unique_ptr<VulkanImage> m_gDepth;
	std::unique_ptr<VulkanImage> m_gNormalMetallic;
	std::unique_ptr<VulkanImage> m_gAlbedoRoughness;
	std::unique_ptr<VulkanImage> m_lightingPassOutput;
	std::unique_ptr<VulkanBuffer> m_screenQuadVertexBuffer;
	const std::vector<gameObject>* m_gameObjectList = nullptr;
	const std::vector<Light>* m_lightsList = nullptr;
	const std::vector<ShadowCaster>* m_shadowCasterList = nullptr;
	const Camera* m_camera;
};

