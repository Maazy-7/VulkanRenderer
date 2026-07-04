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

struct vec2 { float x, y; };

class InputManager
{
	//int stores glfw key, KeyState stores state of that key
	static std::unordered_map<int, KeyState> m_keys;
	static vec2 m_mousePos;
	static vec2 m_scrollOffsets;

	InputManager() = delete;

public:

	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
	static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
	static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
	static void update();
	static bool getKeyUp(int key);//returns true if the key has been released in the current frame (key goes from held or pressed to released)
	static bool getKeyDown(int key);//returns true if the the key has been pressed in the current frame (key goes from up or released to pressed)
	static bool getKey(int key);//returns true if key is in held state which happens when key has been in pressed state for more than 1 frame
	static vec2 getCursorPos();//returns x and y position of cursor relative to window
	static void getCursorPos(float& xpos, float& ypos);//sets x and y pos variables to cursor x and y pos
	static float getScrollOffset();//returns the y offset of the scroll wheel
};

