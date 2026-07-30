#pragma once

#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
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
	bool isFrameBufferResized() const;
	glm::vec2 getDimensions() const;
	float getAspect() const;
	float getTime() const;
	GLFWwindow* getWindow() const;

	void setKeyCallback(void (*keyCallback)(GLFWwindow*, int, int, int, int));
	void setCursorPosCallBack(void (*cursorCallback)(GLFWwindow* window, double xpos, double ypos));
	void setMouseButtonCallBack(void (*mouseButtonCallback)(GLFWwindow* window, int button, int action, int mods));
	void setScrollCallback(void (*scrollCallback)(GLFWwindow* window, double xoffset, double yoffset));
	void setWindowClose(bool close);
	void pollEvents();

private:
	
	void innitWindow();
	static void frameBufferResizeCallback(GLFWwindow*, int, int);
	void resetFrameBufferResizeFlag();

	GLFWwindow* m_window = nullptr;
	int m_width, m_height;
	std::string m_name;
	bool m_frameBufferResized = false;
};

