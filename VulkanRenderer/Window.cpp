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
	//glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	m_window = glfwCreateWindow(m_width, m_height, m_name.c_str(), nullptr, nullptr);
	glfwSetWindowUserPointer(m_window, this);
	glfwSetFramebufferSizeCallback(m_window, frameBufferResizeCallback);
}

bool Window::windowShouldClose() const 
{
	return glfwWindowShouldClose(m_window);
}

bool Window::isFrameBufferResized() const 
{
	return m_frameBufferResized;
}

void Window::resetFrameBufferResizeFlag() 
{
	m_frameBufferResized = false;
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

void Window::setCursorPosCallBack(void(*cursorPosCallback)(GLFWwindow* window, double xpos, double ypos))
{
	glfwSetCursorPosCallback(m_window, cursorPosCallback);
}

void Window::setMouseButtonCallBack(void(*mouseButtonCallback)(GLFWwindow* window, int button, int action, int mods))
{
	glfwSetMouseButtonCallback(m_window, mouseButtonCallback);
}

void Window::setScrollCallback(void(*scrollCallback)(GLFWwindow* window, double xoffset, double yoffset))
{
	glfwSetScrollCallback(m_window, scrollCallback);
}

void Window::setWindowClose(bool close) 
{
	glfwSetWindowShouldClose(m_window, close);
}

void Window::pollEvents() 
{
	glfwPollEvents();
}

void Window::frameBufferResizeCallback(GLFWwindow* glfw_window, int width, int height) 
{
	auto window = reinterpret_cast<Window*>(glfw_window);
	window->m_frameBufferResized = true;
	window->m_width = width;
	window->m_height = height;
}