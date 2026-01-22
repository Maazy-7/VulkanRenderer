#pragma once

#define GLFW_INCLUDE_VULKAN

#include <glfw/glfw3.h>
#include <glm/glm.hpp>
#include <string>

class Window
{
public:
	
	Window(int width, int height, std::string name);
	~Window();
	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;
	
	bool windowShouldClose() const;
	glm::vec2 getDimensions() const;
	float getAspect() const;
	float getTime() const;
	GLFWwindow* getWindow() const;

	void setKeyCallback(void (*keyCallback)(GLFWwindow*, int, int, int, int));
	void setWindowClose(bool close);
	void pollEvents();

private:
	
	void innitWindow();

	GLFWwindow* m_window = nullptr;
	int m_width, m_height;
	std::string m_name;
};

