#include "Scene.h"

gameObject::gameObject(glm::vec3 pos, Model& model)
	:m_pos(pos), m_model(model)
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

Scene::Scene(VulkanRenderingBackend* vulkanRenderingBackend)
	:m_vulkanRenderingBackend(vulkanRenderingBackend),
	m_camera(std::make_unique<Camera>(glm::vec3(0.f, 0.f, 0.2f), vulkanRenderingBackend->getSwapChainAspect(), 0.1f, 100.f))
{
	m_vulkanRenderingBackend->attachCamera(m_camera.get());
}

Scene::~Scene()
{
}

void Scene::update(float dt)
{
	m_camera->update(dt, m_vulkanRenderingBackend->getSwapChainAspect());
}

const std::vector<gameObject>& Scene::getGameObjects() const
{
	return m_gameObjects;
}

const Camera& Scene::getCamera() const
{
	return *m_camera.get();
}
