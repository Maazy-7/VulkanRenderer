#include "InputManager.h"

std::unordered_map<int, KeyState> InputManager::m_keys;
vec2 InputManager::m_mousePos;
vec2 InputManager::m_scrollOffsets;
bool InputManager::mouseScrolled = false;

void InputManager::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS) 
	{
		m_keys[key] = KeyState::Pressed;
	}
	else if (action == GLFW_RELEASE) 
	{
		m_keys[key] = KeyState::Released;
	}
}

void InputManager::cursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
	m_mousePos.x = static_cast<float>(xpos);
	m_mousePos.y = static_cast<float>(ypos);
}

void InputManager::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	//button+1 since GLFW_BUTTON_1 maps to 0 which something else could also map to
	if (action == GLFW_PRESS)
	{
		m_keys[button+1] = KeyState::Pressed;
	}
	else if (action == GLFW_RELEASE)
	{
		m_keys[button+1] = KeyState::Released;
	}
}

void InputManager::scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	m_scrollOffsets.x = static_cast<float>(xoffset);
	m_scrollOffsets.y = static_cast<float>(yoffset);
	mouseScrolled = true;
}

void InputManager::update() 
{
	for (auto& [key, state] : m_keys)
	{
		if (state == KeyState::Pressed)
		{
			m_keys[key] = KeyState::Held;
		}
		else if (state == KeyState::Released) 
		{
			m_keys[key] = KeyState::Up;
		}
	}
}

bool InputManager::getKeyUp(int key)
{
	return m_keys[key] == KeyState::Released;
}

bool InputManager::getKeyDown(int key)
{

	return m_keys[key] == KeyState::Pressed;
}

bool InputManager::getKey(int key)
{
	return m_keys[key] == KeyState::Held;
}

vec2 InputManager::getCursorPos()
{
	return m_mousePos;
}

void InputManager::getCursorPos(float& xpos, float& ypos)
{
	xpos = m_mousePos.x;
	ypos = m_mousePos.y;
}

bool InputManager::getScrollOffset(float& yoffset)
{
	if (mouseScrolled) 
	{
		yoffset = m_scrollOffsets.y;
		mouseScrolled = false;
		return true;
	}
	else 
	{
		return false;
	}
}