#include "Application.h"


Application::Application() 
	:m_window(std::make_unique<Window>(DEFAULT_WIDTH, DEFUALT_HEIGHT, "Application"))
{
	m_window->setKeyCallback(&InputManager::keyCallback);
	VulkanDevice vkd(*m_window);
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

	}
}

float Application::getDeltaTime() 
{
	float currentTime = m_window->getTime();
	m_deltaTime = currentTime - m_prevTime;
	float m_prevTime = m_deltaTime;
	return m_deltaTime;
}