#pragma once

#include <memory>
#include "Window.h"
#include "InputManager.h"
#include "VulkanDevice.h"
#include "VulkanSwapChain.h"
#include "VulkanPipeline.h"
#include "VulkanRenderer.h"
#include "SimpleRenderSystem.h"

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

	//temp
	VulkanDevice m_vkDevice;
	VulkanRenderer  m_vkRenderer;
	SimpleRenderSystem m_simpleRenderSystem;
};

