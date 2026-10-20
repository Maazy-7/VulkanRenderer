#include "Scene.h"

Scene::Scene(VulkanRenderingBackend* vulkanRenderingBackend)
	:m_vulkanRenderingBackend(vulkanRenderingBackend),
	m_vulkanDevice(vulkanRenderingBackend->getVulkanDevice()),
	m_camera(std::make_unique<Camera>(glm::vec3(0.f, 2.f, -1.f), vulkanRenderingBackend->getSwapChainAspect(), 0.1f, 100.f)),
	m_textures(vulkanRenderingBackend->getTextureList()),
	m_models(vulkanRenderingBackend->getModelList())
{
	//m_vulkanRenderingBackend->attachCamera(m_camera.get());
	m_vulkanRenderingBackend->setGameObjectList(&m_gameObjects);
	m_vulkanRenderingBackend->setLightsList(&m_lights);
	m_models->reserve(2);
	Model& cube = m_models->emplace_back(m_vulkanDevice, "Assets/woodenContainer.obj", m_models->size());
	Model& sphere = m_models->emplace_back(m_vulkanDevice, "Assets/sphere.obj", m_models->size());

	m_vulkanRenderingBackend->updateModelList();

	addTexture("NO_LOCATION", "DEFAULT_ERROR_TEXTURE");
	size_t container2Diffuse = addTexture("Assets/container2_clean.png", "container2Diffuse");
	size_t container2Specular = addTexture("Assets/container2Specular.jpeg", "container2dSpecular");
	size_t container2Metallic = addTexture("Assets/metallic_clean.png", "container2Metallic");
	size_t container2Roughness = addTexture("Assets/container2_roughness.png", "container2Roughness");
	size_t sphere2Diffuse = addTexture("Assets/sphere2Diffuse.jpg", "sphere2Diffuse");
	size_t sphere2Metallic = addTexture("Assets/sphere2Metallic.jpg", "sphere2Metallic");
	size_t sphere2Roughness = addTexture("Assets/sphere2Roughness.jpg", "sphere2Roughness");
	size_t rtRed = addTexture("Assets/rtRed.jpg", "rtRed");
	size_t rtGreen = addTexture("Assets/rtGreen.jpg", "rtGreen");
	size_t rtWhite = addTexture("Assets/rtWhite.jpg", "rTWhite");
	size_t blackColor = addTexture("Assets/blackColor.jpg", "blackColor");
	size_t whiteColor = addTexture("Assets/whiteColor.jpg", "whiteColor");

	diffuseSpecMapMaterial simpleDiffuseSpecMapMaterial{ container2Diffuse, container2Specular };
	metallicRoughnessMaterial simplePBRMaterial{ container2Diffuse, container2Metallic, container2Roughness};
	metallicRoughnessMaterial sphere2PBRMaterial{ sphere2Diffuse, sphere2Metallic, sphere2Roughness};
	metallicRoughnessMaterial redWallMaterial{ rtRed, blackColor, whiteColor};
	metallicRoughnessMaterial greenWallMaterial{ rtGreen, blackColor, whiteColor};
	metallicRoughnessMaterial whiteWallMaterial{ rtWhite, blackColor, whiteColor};

	Light& light1 = m_lights.emplace_back(glm::vec3(0.f, 2.f, 0.f), glm::vec4(1.f), sphere, LightType::Point, m_lights.size());
	m_vulkanRenderingBackend->createShadowCaster(light1, 1024, 1024);
	m_vulkanRenderingBackend->attachCamera(m_camera.get());
	glm::vec4 red = glm::vec4(0.65f, 0.05f, 0.05f, 1.f);
	glm::vec4 green = glm::vec4(0.12f, 0.45f, 0.15f, 1.f);
	glm::vec4 white = glm::vec4(0.73f);
	
	//m_gameObjects.emplace_back(glm::vec3(-1.5f, 0.f, -1.5f), glm::vec3(0.f), glm::vec3(0.4f), sphere).constantMaterial = glm::vec4(0.5f, 0.4f, 1.f, 1.f);
	//m_gameObjects.emplace_back(glm::vec3(1.5f, 0.f, -1.5f), glm::vec3(0.f), glm::vec3(0.4f), sphere).constantMaterial = glm::vec4(0.f, 1.f, 1.f, 1.f);
	m_gameObjects.emplace_back(glm::vec3(1.5f, 0.f, 1.5f), glm::vec3(0.f), glm::vec3(0.4f), sphere).constantMaterial = glm::vec4(0.7f, 0.f, 1.f, 1.f);
	//m_gameObjects.emplace_back(glm::vec3(-0.5f, 0.f, 0.5f), glm::vec3(0.f), glm::vec3(0.4f), sphere).constantMaterial = glm::vec4(1.f, 1.f, 1.f, 1.f);
	m_gameObjects.emplace_back(glm::vec3(-1.5f, 0.f, 1.5f), glm::vec3(0.f), glm::vec3(0.4f), sphere).setMaterial(sphere2PBRMaterial);
	
	//cube
	m_gameObjects.emplace_back(glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f), glm::vec3(0.5f), cube).setMaterial(whiteWallMaterial);
	//bottom wall
	m_gameObjects.emplace_back(glm::vec3(0.f, -1.f, 0.f), glm::vec3(0.f), glm::vec3(3.f, 0.15f, 3.f), cube).setMaterial(whiteWallMaterial);
	//back wall
	m_gameObjects.emplace_back(glm::vec3(0.f, 1.f, -3.15f), glm::vec3(0.f), glm::vec3(3.f, 2.f, 0.15f), cube).setMaterial(whiteWallMaterial);
	//right wall
	m_gameObjects.emplace_back(glm::vec3(3.15f, 1.f, 0.f), glm::vec3(0.f), glm::vec3(0.15f, 2.f, 3.f), cube).setMaterial(redWallMaterial);
	//left wall
	m_gameObjects.emplace_back(glm::vec3(-3.15f, 1.f, 0.f), glm::vec3(0.f), glm::vec3(0.15f, 2.f, 3.f), cube).setMaterial(greenWallMaterial);
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

size_t Scene::addTexture(const std::string& path, std::string name)//adds texture to texture list and adds its index to textureMapIndexer
{
	Texture& tex = m_textures->emplace_back(m_vulkanDevice, path);
	size_t index = m_textures->size() - 1;
	m_textureMapIndexer[name] = index;
	return index;
}
