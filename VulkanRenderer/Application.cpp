#include "Application.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

Application::Application()
	:m_window(std::make_unique<Window>(DEFAULT_WIDTH, DEFUALT_HEIGHT, "Application")),
	m_vkDevice(*m_window),
	m_vkRenderer(*m_window, m_vkDevice),
	m_simpleRenderSystem(m_vkDevice, m_vkRenderer.getSwapChain()),
	m_vkDescriptorPool(m_vkDevice, m_vkRenderer.getSwapChain().MAX_FRAMES_IN_FLIGHT)
{
	m_window->setKeyCallback(&InputManager::keyCallback);
}

Application::~Application() 
{

}

void Application::run() 
{
	//temp
	std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;
	VulkanDescriptorSetLayout::addBinding(bindings, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1);
	VulkanDescriptorSetLayout globalSetLayout(m_vkDevice, bindings);
	VulkanDescriptorWriter descriptorWriter(m_vkDescriptorPool, globalSetLayout);
	m_simpleRenderSystem.createPipeline(globalSetLayout);//VkPipeline needs to know about layout, so its created after descriptor layout
	
	for (size_t i = 0; i < 2; i++)
	{
		VkDescriptorBufferInfo descriptorBufferInfo{};
		descriptorBufferInfo.buffer = m_simpleRenderSystem.m_uniformBuffers[i]->getBuffer();
		descriptorBufferInfo.offset = 0;
		descriptorBufferInfo.range = sizeof(m_simpleRenderSystem.ubo);

		descriptorWriter.writeBuffer(&descriptorBufferInfo, 0).build(m_simpleRenderSystem.m_vkDescriptorSets[i]);
	}
	//temp
	while (!m_window->windowShouldClose()) 
	{
		if (InputManager::getKey(GLFW_KEY_ESCAPE)) { m_window->setWindowClose(true); }

		float dt = getDeltaTime();

		m_window->pollEvents();

		//scene update

		InputManager::update();

		//physics update

		//renderer update
		auto commandBuffer = m_vkRenderer.beginFrame();
		m_vkRenderer.beginRendering(commandBuffer);
		m_simpleRenderSystem.renderGameObjects(commandBuffer, dt);
		m_vkRenderer.endRendering(commandBuffer);
		m_vkRenderer.endFrame();

	}
	vkDeviceWaitIdle(m_vkDevice.getDevice());
}

float Application::getDeltaTime() 
{
	float currentTime = m_window->getTime();
	m_deltaTime = currentTime - m_prevTime;
	m_prevTime = currentTime;
	return m_deltaTime;
}