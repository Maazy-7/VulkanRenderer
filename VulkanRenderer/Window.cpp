#include "Window.h"

Window::Window(int width, int height, std::string name) 
	:m_width(width), m_height(height), m_name(name)
{
	Window::innitWindow();
}

Window::~Window() 
{
	glfwDestroyWindow(m_window);
	glfwTerminate();
}

void Window::innitWindow() 
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	m_window = glfwCreateWindow(m_width, m_height, m_name.c_str(), nullptr, nullptr);
}

bool Window::windowShouldClose() const 
{
	return glfwWindowShouldClose(m_window);
}

glm::vec2 Window::getDimensions() const 
{
	return glm::vec2(m_width, m_height);
}

float Window::getAspect() const
{
	return (float)m_width / (float)m_height;
}

float Window::getTime() const 
{
	return (float)glfwGetTime();
}

GLFWwindow* Window::getWindow() const
{
	return m_window;
}

void Window::setKeyCallback(void (*keyCallback)(GLFWwindow*, int, int, int, int)) 
{
	glfwSetKeyCallback(m_window, keyCallback);
}

void Window::setWindowClose(bool close) 
{
	glfwSetWindowShouldClose(m_window, close);
}

void Window::pollEvents() 
{
	glfwPollEvents();
}