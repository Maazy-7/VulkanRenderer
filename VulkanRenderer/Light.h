#pragma once

#include "glm/glm.hpp"
#include "Model.h"

enum class LightType
{
	Directional,
	Point,
	Spot
};

class Light
{
public:
	Light(glm::vec3 pos, glm::vec4 color, LightType lightType);
	Light(glm::vec3 pos, glm::vec4 color, Model& model, LightType lightType);
	~Light();

	//void attachModel(Model& model);

	glm::vec3 getPos() const;
	glm::vec4 getColor() const;
	LightType getLightType() const;

private:
	glm::vec3 m_pos;
	glm::vec4 m_color;//xyz color and w intensity
	LightType m_lightType;
	Model& m_model;
};