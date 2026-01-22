#include "InputManager.h"

std::unordered_map<int, KeyState> InputManager::m_Keys;

void InputManager::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS) 
	{
		m_Keys[key] = KeyState::Pressed;
	}
	else if (action == GLFW_RELEASE) 
	{
		m_Keys[key] = KeyState::Released;
	}
}

void InputManager::update() 
{
	for (auto& [key, state] : m_Keys)
	{
		if (state == KeyState::Pressed)
		{
			m_Keys[key] = KeyState::Held;
		}
		else if (state == KeyState::Released) 
		{
			m_Keys[key] = KeyState::Up;
		}
	}
}

bool InputManager::getKeyUp(int key)
{
	return m_Keys[key] == KeyState::Released;
}

bool InputManager::getKeyDown(int key)
{

	return m_Keys[key] == KeyState::Pressed;
}

bool InputManager::getKey(int key)
{
	return m_Keys[key] == KeyState::Held;
}