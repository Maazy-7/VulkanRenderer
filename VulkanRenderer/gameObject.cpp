#include "gameObject.h"

gameObject::gameObject()
	:m_pos(glm::vec3(0.f)), m_model(Model::getDefaultCubeModel()), m_scale(glm::vec3(1.f)), m_orientation(glm::vec3(0.f))
{

}

gameObject::gameObject(glm::vec3 pos, Model& model)
	:m_pos(pos), m_model(model), m_scale(glm::vec3(1.f)), m_orientation(glm::vec3(0.f))
{

}

gameObject::gameObject(glm::vec3 pos, glm::vec3 orientation, glm::vec3 scale, Model& model)
	:m_pos(pos), m_orientation(orientation), m_scale(scale), m_model(model)
{

}

gameObject::~gameObject()
{
}

glm::mat4 gameObject::getModelMat() const
{
	glm::mat4 rot_mat = glm::eulerAngleXYZ(m_orientation.x, m_orientation.y, m_orientation.z);
	return glm::translate(glm::mat4(1.f), m_pos) * rot_mat * glm::scale(glm::mat4(1.f), m_scale);
}

glm::vec4 gameObject::getColor() const
{
	return m_color;
}

void gameObject::setPos(glm::vec3 pos)
{
	m_pos = pos;
}

void gameObject::setOrientation(glm::vec3 orientation)
{
	m_orientation = orientation;
}

void gameObject::setScale(glm::vec3 scale)
{
	m_scale = scale;
}

void gameObject::translate(glm::vec3 translation)
{
	m_pos += translation;
}

void gameObject::rotate(glm::vec3 axis, float angle)
{
	//todo
}

void gameObject::scale(float scale)
{
	m_scale *= scale;
}

void gameObject::scale(glm::vec3 scale)
{
	m_scale *= scale;
}

void gameObject::setColor(glm::vec4 color)
{
	m_color = color;
}

void gameObject::render(VkCommandBuffer commandBuffer)
{
	m_model.draw(commandBuffer);
}
