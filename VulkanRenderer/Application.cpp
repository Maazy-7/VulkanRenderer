#include "Application.h"


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