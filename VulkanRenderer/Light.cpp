#include "Light.h"

Light::Light(glm::vec3 pos, glm::vec4 color, LightType lightType)
	:m_pos(pos), m_color(color), m_lightType(lightType), m_model(Model::getDefaultCubeModel())
{

}

Light::Light(glm::vec3 pos, glm::vec4 color, Model& model, LightType lightType)
	:m_pos(pos), m_color(color), m_model(model), m_lightType(lightType)
{

}

Light::~Light() 
{

}

glm::vec3 Light::getPos() const { return m_pos; }
glm::vec4 Light::getColor() const { return m_color; }

LightType Light::getLightType() const
{
	return m_lightType;
}
