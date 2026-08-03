#pragma once

#include <memory>
#include "Window.h"
#include "InputManager.h"
#include "VulkanDevice.h"
#include "VulkanSwapChain.h"
#include "VulkanPipeline.h"
#include "VulkanRenderer.h"
#include "SimpleRenderSystem.h"
#include "VulkanRenderingBackend.h"
#include "Scene.h"

#define DEFAULT_WIDTH 1920
#define DEFUALT_HEIGHT 1080

class Application
{
public:

	Application();
	~Application();
	
	void run();
	float getDeltaTime();

private:

	void createBuffersAndDescriptors();

	std::unique_ptr<Window> m_window;
	float m_deltaTime = 0.f;
	float m_prevTime = 0.f;

	//temp
	//VulkanDevice m_vkDevice;
	//VulkanRenderer  m_vkRenderer;
	std::unique_ptr<VulkanDescriptorPool> m_vkDescriptorPool;
	std::unique_ptr<SimpleRenderSystem> m_simpleRenderSystem;
	std::unique_ptr<ShadowRendererSystem> m_shadowRendererSystem;
	std::unique_ptr<VulkanDescriptorSetLayout> m_globalSetLayout;
	std::vector<std::unique_ptr<VulkanBuffer>> m_camaraMatrixBuffers;
	std::vector<VkDescriptorSet> m_camaraMatrixDescriptorSets;
	//Camera m_camera;

	VulkanRenderingBackend m_vulkanRenderingBackend;
	Scene m_scene;
};

//temp
//struct FrameUbo 
//{
//	alignas(16) glm::mat4 viewMat;
//	alignas(16) glm::mat4 projMat;
//	alignas(16) glm::mat4 lightSpaceMat;
//	alignas(16) glm::vec4 viewPos;
//	alignas(16) glm::vec4 lightPos;
//};