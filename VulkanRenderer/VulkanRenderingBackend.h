#pragma once

#include "VulkanDevice.h"
#include "VulkanSwapChain.h"
#include "VulkanRenderer.h"
#include "SimpleRenderSystem.h"
#include "ShadowRendererSystem.h"

class VulkanRenderingBackend
{
public:

	VulkanRenderingBackend(const VulkanRenderingBackend&) = delete;
	const VulkanRenderingBackend& operator=(const VulkanRenderingBackend&) = delete;

	VulkanRenderingBackend(Window& window);
	~VulkanRenderingBackend();

	void render(float dt);

	void attachCamera(const Camera* camera);
	//void attachScene(Scene* scene);

	float getSwapChainAspect() const;

private:

	void createBuffersAndDescriptors();

	Window& m_window;
	VulkanDevice m_vulkanDevice;
	VulkanRenderer m_vulkanRenderer;
	std::unique_ptr<VulkanDescriptorPool> m_vulkanDescriptorPool;
	std::unique_ptr<VulkanDescriptorSetLayout> m_globalSetLayout;
	std::vector<VulkanBuffer> m_cameraMatrixBuffers;
	std::vector<VkDescriptorSet> m_cameraMatrixDescriptorSets;

	std::unique_ptr<ShadowRendererSystem> m_shadowRendererSystem;
	std::unique_ptr<SimpleRenderSystem> m_simpleRenderSystem;

	const Camera* m_camera = nullptr;
};

struct FrameUbo
{
	alignas(16) glm::mat4 viewMat;
	alignas(16) glm::mat4 projMat;
	alignas(16) glm::mat4 lightSpaceMat;
	alignas(16) glm::vec4 viewPos;
	alignas(16) glm::vec4 lightPos;
};