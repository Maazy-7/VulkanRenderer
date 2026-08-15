#pragma once

#include "Model.h"
#include "Texture.h"
#include "Camera.h"
#include "VulkanRenderingBackend.h"
#include "gameObject.h"
#include "Light.h"


//Scene will speak to VulkanBackend but VulkanBackend wont speak back, Scene will only modify lists of renderable objects stored within VulkanBackend
class Scene
{
public:

	Scene(const Scene&) = delete;
	Scene& operator=(const Scene&) = delete;

	Scene(VulkanRenderingBackend* vulkanRenderingBackend);
	~Scene();

	void update(float dt);

	const std::vector<gameObject>& getGameObjects() const;
	const Camera& getCamera() const;

private:
	VulkanRenderingBackend* m_vulkanRenderingBackend;
	VulkanDevice& m_vulkanDevice;

	std::vector<gameObject> m_gameObjects;
	std::vector<Light> m_lights;
	std::vector<Model> m_models;
	std::vector<Texture> m_textures;
	std::unique_ptr<Camera> m_camera;
};

