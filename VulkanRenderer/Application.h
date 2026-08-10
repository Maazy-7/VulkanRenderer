#pragma once

#include <memory>
#include "Window.h"
#include "InputManager.h"
#include "Scene.h"
#include "VulkanRenderingBackend.h"

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

	std::unique_ptr<Window> m_window;
	float m_deltaTime = 0.f;
	float m_prevTime = 0.f;
	VulkanRenderingBackend m_vulkanRenderingBackend;
	Scene m_scene;
};