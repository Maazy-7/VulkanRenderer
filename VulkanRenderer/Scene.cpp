#include "Scene.h"

Scene::Scene(VulkanRenderingBackend* vulkanRenderingBackend)
	:m_vulkanRenderingBackend(vulkanRenderingBackend),
	m_vulkanDevice(vulkanRenderingBackend->getVulkanDevice()),
	m_camera(std::make_unique<Camera>(glm::vec3(0.f, 2.f, -1.f), vulkanRenderingBackend->getSwapChainAspect(), 0.1f, 100.f))
{
	//m_vulkanRenderingBackend->attachCamera(m_camera.get());
	m_vulkanRenderingBackend->setGameObjectList(&m_gameObjects);
	m_vulkanRenderingBackend->setLightsList(&m_lights);

	m_models.emplace_back(m_vulkanDevice, "Assets/woodenContainer.obj");

	Model& cube = m_models[0];

	Light& light1 = m_lights.emplace_back(glm::vec3(0.f, 2.f, -1.f), glm::vec4(1.f), cube, LightType::Point, m_lights.size());
	m_vulkanRenderingBackend->createShadowCaster(light1, 1024, 1024);
	m_vulkanRenderingBackend->attachCamera(m_camera.get());
	glm::vec4 red = glm::vec4(0.65f, 0.05f, 0.05f, 1.f);
	glm::vec4 green = glm::vec4(0.12f, 0.45f, 0.15f, 1.f);
	glm::vec4 white = glm::vec4(0.73f);
	//cube
	m_gameObjects.emplace_back(glm::vec3(0.f,-0.2f,0.f), glm::vec3(0.f), glm::vec3(0.5f), cube).setColor(white);
	//bottom wall
	m_gameObjects.emplace_back(glm::vec3(0.f, -1.f, 0.f), glm::vec3(0.f), glm::vec3(3.f, 0.15f, 3.f), cube).setColor(white);
	//back wall
	m_gameObjects.emplace_back(glm::vec3(0.f, 1.f, -3.15f), glm::vec3(0.f), glm::vec3(3.f, 2.f, 0.15f), cube).setColor(white);
	//right wall
	m_gameObjects.emplace_back(glm::vec3(3.15f, 1.f, 0.f), glm::vec3(0.f), glm::vec3(0.15f, 2.f, 3.f), cube).setColor(red);
	//left wall
	m_gameObjects.emplace_back(glm::vec3(-3.15f, 1.f, 0.f), glm::vec3(0.f), glm::vec3(0.15f, 2.f, 3.f), cube).setColor(green);


}

Scene::~Scene()
{

}

void Scene::update(float dt)
{
	m_camera->update(dt, m_vulkanRenderingBackend->getSwapChainAspect());

	Light& l1 = m_lights[0];
	glm::vec3 newPos = glm::vec3(0.f, 3.f, 0.f);
	static float d;
	d += 2.5f*dt;
	newPos.x = std::cos(d);
	newPos.z = std::sin(d);
	l1.setPos(newPos);
}

const std::vector<gameObject>& Scene::getGameObjects() const
{
	return m_gameObjects;
}

const Camera& Scene::getCamera() const
{
	return *m_camera.get();
}
