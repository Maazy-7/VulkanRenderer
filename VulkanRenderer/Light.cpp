#include "Light.h"

Light::Light(glm::vec3 pos, glm::vec4 color, LightType lightType, size_t lightIndex)
	:m_pos(pos), m_color(color), m_lightType(lightType), m_model(Model::getDefaultCubeModel()), m_lightIndex(lightIndex)
{

}

Light::Light(glm::vec3 pos, glm::vec4 color, Model& model, LightType lightType, size_t lightIndex)
	:m_pos(pos), m_color(color), m_model(model), m_lightType(lightType), m_lightIndex(lightIndex)
{

}

Light::~Light() 
{

}

glm::vec3 Light::getPos() const { return m_pos; }
glm::vec4 Light::getColor() const { return m_color; }

void Light::setPos(glm::vec3 pos) 
{
	m_pos = pos;
}

LightType Light::getLightType() const
{
	return m_lightType;
}

bool Light::hasShadowCaster(size_t& shadowCasterIndexOut) const
{
	shadowCasterIndexOut = m_shadowCasterIndex;
	return m_shadowCasterIndex != UINT32_MAX;//if true then light has shadow caster
}

size_t Light::getLightIndex() const
{
	return m_lightIndex;
}
