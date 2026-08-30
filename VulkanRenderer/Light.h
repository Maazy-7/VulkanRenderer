#pragma once

#include "glm/glm.hpp"
#include "Model.h"

enum class LightType
{
	Directional,
	Point,
	Spot
};

class ShadowCaster;

class Light
{
public:
	Light(glm::vec3 pos, glm::vec4 color, LightType lightType, size_t lightIndex);
	Light(glm::vec3 pos, glm::vec4 color, Model& model, LightType lightType, size_t lightIndex);
	~Light();

	//void attachModel(Model& model);

	glm::vec3 getPos() const;
	glm::vec4 getColor() const;
	LightType getLightType() const;
	bool hasShadowCaster(size_t& shadowCasterIndexOut) const;
	size_t getLightIndex() const;

	void draw(VkCommandBuffer commandBuffer) const;

	void setPos(glm::vec3 pos);

private:
	glm::vec3 m_pos;
	glm::vec4 m_color;//xyz color and w intensity
	LightType m_lightType;
	Model& m_model;
	size_t m_lightIndex; 
	size_t m_shadowCasterIndex = SIZE_MAX;//max size_t represents no shadow caster attached, could also try int and -1 for no shadow caster

	friend ShadowCaster;
};