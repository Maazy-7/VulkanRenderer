#pragma once

#include <unordered_map>
#include <glfw/glfw3.h>

enum class KeyState 
{
	Up,
	Pressed,
	Held,
	Released
};

class InputManager
{
	//int stores glfw key, KeyState stores state of that key
	static std::unordered_map<int, KeyState> m_Keys;

	InputManager() = delete;

public:

	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void update();
	static bool getKeyUp(int key);//returns true if the key has been released in the current frame (key goes from held or pressed to released)
	static bool getKeyDown(int key);//returns true if the the key has been pressed in the current frame (key goes from up or released to pressed)
	static bool getKey(int key);//returns true if key is in held state which happens when key has been in pressed state for more than 1 frame
};

