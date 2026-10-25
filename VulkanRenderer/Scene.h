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

	size_t addTexture(const std::string& path, std::string name);
	size_t addTexture(uint8_t r, uint8_t g, uint8_t b, std::string name);

	VulkanRenderingBackend* m_vulkanRenderingBackend;
	VulkanDevice& m_vulkanDevice;

	std::vector<gameObject> m_gameObjects;
	std::vector<Light> m_lights;
	std::vector<Model>* m_models = nullptr;
	std::vector<Texture>* m_textures = nullptr;//owned by VulkanRenderingBackend but can be modified from scene
	std::unordered_map<std::string, size_t> m_textureMapIndexer;//map used to index texture list using texture name
	std::unique_ptr<Camera> m_camera;
};