#pragma once

#include "VulkanDevice.h"
#include "VulkanSwapChain.h"
#include "VulkanRenderer.h"
#include "SimpleRenderSystem.h"
#include "ShadowRendererSystem.h"
#include "DeferredRendererSystem.h"
#include "gameObject.h"

class VulkanRenderingBackend
{
public:

	VulkanRenderingBackend(const VulkanRenderingBackend&) = delete;
	const VulkanRenderingBackend& operator=(const VulkanRenderingBackend&) = delete;

	VulkanRenderingBackend(Window& window);
	~VulkanRenderingBackend();

	void render(float dt);

	void attachCamera(const Camera* camera);
	void setGameObjectList(std::vector<gameObject>* gameObjects);
	void setLightsList(std::vector<Light>* lights);
	void createShadowCaster(Light& light, uint32_t shadowWidth, uint32_t shadowHeight, float nearPlane = 0.1f, float farPlane = 25.f);


	float getSwapChainAspect() const;
	VulkanDevice& getVulkanDevice();
	std::vector<Texture>* getTextureList();

private:

	void createBuffersAndDescriptors();

	Window& m_window;
	VulkanDevice m_vulkanDevice;
	VulkanRenderer m_vulkanRenderer;
	std::unique_ptr<VulkanDescriptorPool> m_vulkanDescriptorPool;
	std::unique_ptr<VulkanDescriptorSetLayout> m_globalSetLayout;
	std::unique_ptr<VulkanDescriptorPool> m_globalTextureListDescriptorPool;
	std::unique_ptr<VulkanDescriptorSetLayout> m_globalTextureListDescriptorSetLayout;
	std::vector<VulkanBuffer> m_cameraMatrixBuffers;
	std::vector<VkDescriptorSet> m_cameraMatrixDescriptorSets;
	VkDescriptorSet m_globalTextureDescriptorSet;

	std::unique_ptr<ShadowRendererSystem> m_shadowRendererSystem;
	std::unique_ptr<SimpleRenderSystem> m_simpleRenderSystem;
	std::unique_ptr<DeferredRendererSystem> m_deferredRendererSystem;

	std::vector<Texture> m_textures;//owned by class, but modifiable by scene
	const Camera* m_camera = nullptr;
	std::vector<gameObject>* m_gameObjects = nullptr;
	std::vector<Light>* m_lights = nullptr;

	uint32_t maxTextures = 1024;
};

struct FrameUbo
{
	alignas(16) glm::mat4 viewMat;
	alignas(16) glm::mat4 projMat;
	alignas(16) glm::mat4 lightSpaceMat;
	alignas(16) glm::vec4 viewPos;
	alignas(16) glm::vec4 lightPos;
};