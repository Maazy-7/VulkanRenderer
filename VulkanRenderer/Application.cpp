#include "Application.h"

Application::Application()
	:m_window(std::make_unique<Window>(DEFAULT_WIDTH, DEFUALT_HEIGHT, "Application")),
	m_vkDevice(*m_window),
	m_vkRenderer(*m_window, m_vkDevice),
	m_camera(Camera(glm::vec3(0.f, 0.f, 2.f), m_vkRenderer.getSwapChain().getSwapChainAspect(), 0.1f, 100.f))
{
	m_window->setKeyCallback(&InputManager::keyCallback);
	m_window->setCursorPosCallBack(&InputManager::cursorPosCallback);
	m_window->setMouseButtonCallBack(&InputManager::mouseButtonCallback);
	m_window->setScrollCallback(&InputManager::scrollCallback);

	std::unordered_map<uint32_t, VkDescriptorType> descriptorTypes = { {2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER}, {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER} };
	m_vkDescriptorPool = std::make_unique<VulkanDescriptorPool>(m_vkDevice, descriptorTypes);
	m_simpleRenderSystem = std::make_unique<SimpleRenderSystem>(m_vkDevice, m_vkRenderer, *m_vkDescriptorPool.get(), m_camera);
	m_shadowRendererSystem = std::make_unique<ShadowRendererSystem>(m_vkDevice, m_camera, m_simpleRenderSystem->getModels());
	m_simpleRenderSystem->setShadowMap(m_shadowRendererSystem->getShadowMap());
	createBuffersAndDescriptors();
}

Application::~Application() 
{

}

void Application::run() 
{
	while (!m_window->windowShouldClose()) 
	{
		if (InputManager::getKey(GLFW_KEY_ESCAPE)) { m_window->setWindowClose(true); }

		float dt = getDeltaTime();

		m_window->pollEvents();

		//scene update
		m_camera.update(dt, m_vkRenderer.getSwapChain().getSwapChainAspect());

		InputManager::update();

		//physics update

		//renderer update
		auto commandBuffer = m_vkRenderer.beginFrame();

		glm::vec4 LightPos = glm::vec4(2.f, 2.f, -1.5f, 0.f);
		glm::vec4 ViewPos = glm::vec4(m_camera.getPos(), 1.f);
		LightData lightData{}; lightData.lightPos = LightPos; lightData.viewPos = ViewPos;
		glm::mat4 lightSpaceMat = glm::ortho(-20.f, 20.f, -20.f, 20.f, 0.1f, 25.f);
		lightSpaceMat[1][1] *= -1;
		lightSpaceMat *= glm::lookAt(glm::vec3(LightPos), glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f));

		m_shadowRendererSystem->renderShadowPass(commandBuffer, &lightSpaceMat);

		size_t frameIndex = m_vkRenderer.getSwapChain().getCurrentFrameIndex();
		CamMatrices view_proj = m_camera.getCameraMatrices();
		view_proj.proj[1][1] *= -1;
		m_camaraMatrixBuffers[frameIndex]->writeToBuffer(0, sizeof(glm::mat4) * 2, &view_proj);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_simpleRenderSystem->getVulkanPipeline().getPipelineLayout(), 0, 
			1, &m_camaraMatrixDescriptorSets[frameIndex], 0, nullptr);

		m_vkRenderer.beginRendering(commandBuffer);
		m_simpleRenderSystem->renderGameObjects(commandBuffer, dt);
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

void Application::createBuffersAndDescriptors()
{
	//allocating uniform buffers
	for (size_t i = 0; i < m_vkRenderer.getSwapChain().MAX_FRAMES_IN_FLIGHT; i++)
	{
		m_camaraMatrixBuffers.push_back(std::make_unique<VulkanBuffer>(m_vkDevice, 2, sizeof(glm::mat4) * 2,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
		m_camaraMatrixBuffers[i]->map(0, sizeof(glm::mat4) * 2);
	}

	//creating descriptor layout and sets
	std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;
	VulkanDescriptorSetLayout::addBinding(bindings, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1);
	m_globalSetLayout = std::make_unique<VulkanDescriptorSetLayout>(m_vkDevice, bindings);
	VulkanDescriptorWriter descriptorWriter(*m_vkDescriptorPool.get(), *m_globalSetLayout.get());
	m_simpleRenderSystem->createPipeline(*m_globalSetLayout.get());

	m_camaraMatrixDescriptorSets.resize(m_vkRenderer.getSwapChain().MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < m_camaraMatrixDescriptorSets.size(); i++)
	{
		VkDescriptorBufferInfo descriptorBufferInfo{};
		descriptorBufferInfo.buffer = m_camaraMatrixBuffers[i]->getBuffer();
		descriptorBufferInfo.offset = 0;
		descriptorBufferInfo.range = sizeof(glm::mat4) * 2;

		descriptorWriter.writeBuffer(&descriptorBufferInfo, 0).build(m_camaraMatrixDescriptorSets[i]);
	}
}
