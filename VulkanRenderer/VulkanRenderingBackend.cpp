#include "VulkanRenderingBackend.h"

VulkanRenderingBackend::VulkanRenderingBackend(Window& window)
	: m_window(window), m_vulkanDevice(window), m_vulkanRenderer(window, m_vulkanDevice)
{
	//1 uniform frame buffer per frame in flight (2) and 2 textures, one for albedo other is shadow map
	std::unordered_map<VkDescriptorType, uint32_t> descriptorTypes = { {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2}, {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2} };
	m_vulkanDescriptorPool = std::make_unique<VulkanDescriptorPool>(m_vulkanDevice, descriptorTypes);
	descriptorTypes.clear();
	//1 binding which is a descriptor array of textures (1024 textures for now)
	descriptorTypes = { {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, maxTextures} };
	m_globalTextureListDescriptorPool = std::make_unique<VulkanDescriptorPool>(m_vulkanDevice, descriptorTypes, VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT);
	m_simpleRenderSystem = std::make_unique<SimpleRenderSystem>(m_vulkanDevice, m_vulkanRenderer, *m_vulkanDescriptorPool.get(), nullptr);
	m_deferredRendererSystem = std::make_unique<DeferredRendererSystem>(m_vulkanDevice, m_vulkanRenderer);
	m_shadowRendererSystem = std::make_unique<ShadowRendererSystem>(m_vulkanDevice, m_simpleRenderSystem->getModels());
	m_simpleRenderSystem->setShadowCasterList(m_shadowRendererSystem->getShadowCasters());
	m_deferredRendererSystem->setShadowCasterList(m_shadowRendererSystem->getShadowCasters());
	stbi_set_flip_vertically_on_load(1);
}

VulkanRenderingBackend::~VulkanRenderingBackend()
{
	vkDeviceWaitIdle(m_vulkanDevice.getDevice());
}

void VulkanRenderingBackend::render(float dt)
{
	//renderer update
	VkCommandBuffer commandBuffer = m_vulkanRenderer.beginFrame();


	/*static float acc = 0.f;
	acc += dt;
	float x = 8.f * std::cos(2.f * acc);
	float y = 8.f * std::sin(2.f * acc);*/

	glm::vec4 LightPos = glm::vec4((*m_lights)[0].getPos(), 0.f);
	glm::vec4 ViewPos = glm::vec4(m_camera->getPos(), 1.f);
	LightData lightData{}; lightData.lightPos = LightPos; lightData.viewPos = ViewPos;
	glm::mat4 lightSpaceMat = glm::ortho(-15.f, 15.f, -15.f, 15.f, 0.1f, 25.f);
	lightSpaceMat[1][1] *= -1;
	lightSpaceMat *= glm::lookAt(glm::vec3(LightPos), glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f));

	m_shadowRendererSystem->renderShadowPass(commandBuffer);

	FrameUbo frameUbo{}; frameUbo.lightSpaceMat = lightSpaceMat; frameUbo.viewPos = ViewPos; frameUbo.lightPos = LightPos;

	size_t frameIndex = m_vulkanRenderer.getSwapChain().getCurrentFrameIndex();
	CamMatrices view_proj = m_camera->getCameraMatrices();
	view_proj.proj[1][1] *= -1; frameUbo.projMat = view_proj.proj; frameUbo.viewMat = view_proj.view;

	m_cameraMatrixBuffers[frameIndex].writeToBuffer(0, sizeof(FrameUbo), &frameUbo);
	
	VkDescriptorSet cameraMatrixDescriptorSet = m_cameraMatrixDescriptorSets[frameIndex];
	
	static bool forwardRenderer;

	if (InputManager::getKey(GLFW_KEY_SPACE)) { forwardRenderer = true; }
	else { forwardRenderer = false; }

	if (forwardRenderer)
	{
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_simpleRenderSystem->getVulkanPipeline().getPipelineLayout(), 0,
			1, &cameraMatrixDescriptorSet, 0, nullptr);
		m_vulkanRenderer.beginRendering(commandBuffer);
		m_simpleRenderSystem->renderGameObjects(commandBuffer, dt);
		m_vulkanRenderer.endRendering(commandBuffer);
	}
	else
	{
		VkDescriptorSet descriptorSets[2] = { cameraMatrixDescriptorSet, m_globalTextureDescriptorSet };
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_deferredRendererSystem->getGeometryPassPipeline().getPipelineLayout(),
			0, 2, descriptorSets, 0, nullptr);
		m_deferredRendererSystem->executeGeometryPass(commandBuffer);
		m_deferredRendererSystem->executeLightingPass(commandBuffer);
		m_deferredRendererSystem->renderLights(commandBuffer);
		m_vulkanRenderer.blitVkImageToSwapChain(commandBuffer, m_deferredRendererSystem->getLightingPassOutput());
	}

	m_vulkanRenderer.endFrame();
}

void VulkanRenderingBackend::attachCamera(const Camera* camera) 
{
	m_camera = camera;
	m_simpleRenderSystem->attachCamera(camera);
	m_deferredRendererSystem->attachCamera(camera);
	createBuffersAndDescriptors();
}

void VulkanRenderingBackend::setGameObjectList(std::vector<gameObject>* gameObjects)
{
	m_gameObjects = gameObjects;
	m_simpleRenderSystem->setGameObjectList(gameObjects);
	m_deferredRendererSystem->setGameObjectList(gameObjects);
	m_shadowRendererSystem->setGameObjectList(gameObjects);
}

void VulkanRenderingBackend::setLightsList(std::vector<Light>* lights)
{
	m_lights = lights;
	m_simpleRenderSystem->setLightsList(lights);
	m_deferredRendererSystem->setLightList(lights);
	m_shadowRendererSystem->setLightLists(lights);
}

void VulkanRenderingBackend::createShadowCaster(Light& light, uint32_t shadowWidth, uint32_t shadowHeight, float nearPlane, float farPlane) 
{
	m_shadowRendererSystem->createShadowCaster(light, shadowWidth, shadowHeight, nearPlane, farPlane);
}

float VulkanRenderingBackend::getSwapChainAspect() const 
{
	return m_vulkanRenderer.getSwapChain().getSwapChainAspect();
}

VulkanDevice& VulkanRenderingBackend::getVulkanDevice()
{
	return m_vulkanDevice;
}

std::vector<Texture>* VulkanRenderingBackend::getTextureList()
{
	return &m_textures;
}

void VulkanRenderingBackend::createBuffersAndDescriptors()
{
	//allocating uniform buffers
	for (size_t i = 0; i < m_vulkanRenderer.getSwapChain().MAX_FRAMES_IN_FLIGHT; i++)
	{
		m_cameraMatrixBuffers.emplace_back(m_vulkanDevice, 5, sizeof(FrameUbo),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		m_cameraMatrixBuffers[i].map(0, sizeof(FrameUbo));
	}
	
	//creating descriptor layout and sets
	std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;
	VulkanDescriptorSetLayout::addBinding(bindings, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1);
	m_globalSetLayout = std::make_unique<VulkanDescriptorSetLayout>(m_vulkanDevice, bindings);
	VulkanDescriptorWriter descriptorWriter(*m_vulkanDescriptorPool, *m_globalSetLayout);
	m_simpleRenderSystem->createPipeline(*m_globalSetLayout);

	m_cameraMatrixDescriptorSets.resize(m_vulkanRenderer.getSwapChain().MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < m_cameraMatrixDescriptorSets.size(); i++)
	{
		VkDescriptorBufferInfo descriptorBufferInfo{};
		descriptorBufferInfo.buffer = m_cameraMatrixBuffers[i].getBuffer();
		descriptorBufferInfo.offset = 0;
		descriptorBufferInfo.range = sizeof(FrameUbo);

		descriptorWriter.writeBuffer(&descriptorBufferInfo, 0).build(m_cameraMatrixDescriptorSets[i]);
	}

	bindings.clear();
	VulkanDescriptorSetLayout::addBinding(bindings, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, maxTextures);
	VkDescriptorBindingFlags descBindingFlag = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT | 
		VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | 
		VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT |
		VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT;
	VkDescriptorSetLayoutBindingFlagsCreateInfo layoutBindingFlagsCreateInfo{};
	layoutBindingFlagsCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
	layoutBindingFlagsCreateInfo.bindingCount = 1;
	layoutBindingFlagsCreateInfo.pBindingFlags = &descBindingFlag;
	m_globalTextureListDescriptorSetLayout = std::make_unique<VulkanDescriptorSetLayout>(m_vulkanDevice, bindings, 
		VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT, &layoutBindingFlagsCreateInfo);
	VkDescriptorSetVariableDescriptorCountAllocateInfo variableDescriptorCountAllocInfo{};
	variableDescriptorCountAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
	variableDescriptorCountAllocInfo.descriptorSetCount = 1;
	variableDescriptorCountAllocInfo.pDescriptorCounts = &maxTextures;
	m_globalTextureListDescriptorPool->allocateDescriptorSet(*m_globalTextureListDescriptorSetLayout, m_globalTextureDescriptorSet,
		&variableDescriptorCountAllocInfo);
	m_deferredRendererSystem->createPipelines(*m_globalSetLayout, *m_globalTextureListDescriptorSetLayout);
	
	//allocate inital textures - need to make a better system to load and unload textures dynamically
	std::vector<VkDescriptorImageInfo> descriptorImageInfos;
	for (Texture& tex : m_textures) 
	{
		VkDescriptorImageInfo descriptorImageInfo{};
		descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		descriptorImageInfo.imageView = tex.getVkImageView();
		descriptorImageInfo.sampler = tex.getVkSampler();
		descriptorImageInfos.push_back(descriptorImageInfo);
	}
	VkWriteDescriptorSet globalTextureDescriptorSetWrite{};
	globalTextureDescriptorSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	globalTextureDescriptorSetWrite.dstBinding = 0;
	globalTextureDescriptorSetWrite.dstArrayElement = 0;
	globalTextureDescriptorSetWrite.dstSet = m_globalTextureDescriptorSet;
	globalTextureDescriptorSetWrite.pImageInfo = descriptorImageInfos.data();
	globalTextureDescriptorSetWrite.descriptorCount = static_cast<uint32_t>(descriptorImageInfos.size());
	globalTextureDescriptorSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	vkUpdateDescriptorSets(m_vulkanDevice.getDevice(), 1, &globalTextureDescriptorSetWrite, 0, nullptr);
}