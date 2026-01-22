#pragma once

#include <memory>
#include "Window.h"
#include "VulkanDevice.h"
#include "InputManager.h"

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
};

