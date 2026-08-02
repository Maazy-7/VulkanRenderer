#pragma once

#include "Model.h"

//temp gameobject class
class gameObject 
{
public:

	gameObject(glm::vec3 pos);
	gameObject(glm::vec3 pos, glm::vec3 orientation, glm::vec3 scale);
	~gameObject();

	glm::mat4 getModelMat() const;

	void setPos(glm::vec3 pos);
	void setOrientation(glm::vec3 orientation);
	void setScale(glm::vec3 scale);
	void translate(glm::vec3 translation);
	void rotate(glm::vec3 axis, float angle);
	void scale(float scale);
	void scale(glm::vec3 scale);

private:
	glm::vec3 m_pos;
	glm::vec3 m_orientation;
	glm::vec3 m_scale;
	Model& m_model;
};

class Scene
{
public:

	Scene(const Scene&) = delete;
	Scene& operator=(const Scene&) = delete;

	Scene();
	~Scene();

private:
	std::vector<gameObject> m_gameObjects;
};

