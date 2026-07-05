#include "Camera.h"

Camera::Camera(glm::vec3 pos, float aspect, float near, float far, float pitch, float yaw, glm::vec3 up)
	: m_position(pos), m_aspect(aspect), m_pitch(pitch), m_yaw(yaw), m_up(up), m_near(near), m_far(far),
	m_front(initialLookDirection), m_right(glm::normalize(glm::cross(m_front,m_up)))
{
	calculateFront();
	calculateProjection();
}

void Camera::update(float dt) 
{
	processMouse();

	float prevY = m_position.y;

	if (InputManager::getKey(GLFW_KEY_W)) 
	{
		m_position += m_front * m_speed * dt;
	}
	if (InputManager::getKey(GLFW_KEY_S))
	{
		m_position -= m_front * m_speed * dt;
	}
	if (InputManager::getKey(GLFW_KEY_A))
	{
		m_position -= glm::normalize(glm::cross(m_front, m_up)) * m_speed * dt;
	}
	if (InputManager::getKey(GLFW_KEY_D))
	{
		m_position += glm::normalize(glm::cross(m_front, m_up)) * m_speed * dt;
	}
	
	m_position.y = prevY;

	if (InputManager::getKey(GLFW_KEY_LEFT_SHIFT))
	{
		m_position += m_up * m_speed * dt;
	}
	if (InputManager::getKey(GLFW_KEY_LEFT_CONTROL))
	{
		m_position -= m_up * m_speed * dt;
	}

	m_view = glm::lookAt(m_position,m_position+m_front,m_up);
}


void Camera::processMouse() 
{
	float xpos, ypos;
	
	static bool firstMouse = true;
	static float prevX, prevY = 0;
	static float scrollOffset = 0.f;
	
	InputManager::getCursorPos(xpos, ypos);

	if (firstMouse) 
	{
		prevX = xpos;
		prevY = ypos;
		firstMouse = false;
	}

	float offsetX = xpos - prevX;
	float offsetY = ypos - prevY;

	m_yaw -= offsetX * m_sens;
	m_pitch -= offsetY * m_sens;

	calculateFront();

	prevX = xpos;
	prevY = ypos;

	if (InputManager::getScrollOffset(scrollOffset)) 
	{
		m_fov -= scrollOffset * 3.f;
		if (m_fov < 1.f)
		{
			m_fov = 1.f;
		}
		if (m_fov > 120.f)
		{
			m_fov = 120.f;
		}
		calculateProjection();
	}
}

void Camera::calculateFront() 
{
	if (m_pitch > 89.f) 
	{
		m_pitch = 89.f;
	}
	if (m_pitch < -89.f) 
	{
		m_pitch = -89.f;
	}
	glm::mat4 yawMat = glm::rotate(glm::mat4(1.f), glm::radians(m_yaw),glm::vec3(0.f,1.f,0.f));
	glm::mat4 pitchMat = glm::rotate(glm::mat4(1.f), glm::radians(m_pitch), glm::vec3(1.f, 0.f, 0.f));
	glm::vec4 lookDir = yawMat * pitchMat * glm::vec4(initialLookDirection, 1.f);

	m_front = glm::normalize(glm::vec3(lookDir));
}

void Camera::calculateProjection() 
{
	m_proj = glm::perspective(glm::radians(m_fov), m_aspect, m_near, m_far);
}

const glm::mat4& Camera::getProjection() const
{
	return m_proj;
}

const glm::mat4& Camera::getView() const
{
	return m_view;
}