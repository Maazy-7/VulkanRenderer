#include "Application.h"

Application::Application()
	:m_window(std::make_unique<Window>(DEFAULT_WIDTH, DEFUALT_HEIGHT, "Application")),
	m_vulkanRenderingBackend(*m_window),
	m_scene(&m_vulkanRenderingBackend)
{
	m_window->setKeyCallback(&InputManager::keyCallback);
	m_window->setCursorPosCallBack(&InputManager::cursorPosCallback);
	m_window->setMouseButtonCallBack(&InputManager::mouseButtonCallback);
	m_window->setScrollCallback(&InputManager::scrollCallback);
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
		m_scene.update(dt);

		InputManager::update();

		//physics update

		//renderer update
		m_vulkanRenderingBackend.render(dt);

	}
}

float Application::getDeltaTime() 
{
	float currentTime = m_window->getTime();
	m_deltaTime = currentTime - m_prevTime;
	m_prevTime = currentTime;
	return m_deltaTime;
}