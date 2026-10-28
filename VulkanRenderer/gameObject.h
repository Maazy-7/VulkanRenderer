#pragma once

#include "Model.h"
#include <glm/gtx/euler_angles.hpp>

struct diffuseSpecMapMaterial //a simple container for holding texture indices for a diffuse map and specular map
{
	size_t m_diffuseTextureIndex = 0;
	size_t m_specularTextureIndex = 0;
	
	glm::ivec2 getTextureIndices() const;
};

struct metallicRoughnessMaterial //material that holds indices for a diffuse,metallic and roughness map
{
	size_t m_diffuseTextureIndex = 0;
	size_t m_metallicTextureIndex = 0;
	size_t m_roughnessTextureIndex = 0;
	size_t m_NormalMapTextureIndex = 0;

	glm::ivec4 getTextureIndices() const;
};

//temp gameobject class
class gameObject
{
public:

	gameObject();
	gameObject(glm::vec3 pos, Model& model);
	gameObject(glm::vec3 pos, glm::vec3 orientation, glm::vec3 scale, Model& model);
	~gameObject();

	glm::mat4 getModelMat() const;
	glm::vec4 getColor() const;
	metallicRoughnessMaterial getMaterial() const;
	size_t getModelID() const;

	void setMaterial(metallicRoughnessMaterial& material);

	void setPos(glm::vec3 pos);
	void setOrientation(glm::vec3 orientation);
	void setScale(glm::vec3 scale);
	void translate(glm::vec3 translation);
	void rotate(glm::vec3 axis, float angle);
	void scale(float scale);
	void scale(glm::vec3 scale);

	void setColor(glm::vec4 color);
	void render(VkCommandBuffer commandBuffer) const;

private:
	glm::vec3 m_pos;
	glm::vec3 m_orientation;
	glm::vec3 m_scale;
	Model& m_model;//fine for now but soon should just store pointer to list and index with model ID
	size_t m_modelID = SIZE_MAX;
	//temp
	glm::vec4 m_color;
	metallicRoughnessMaterial m_material;
};