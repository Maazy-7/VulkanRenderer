#include "VulkanRenderingBackend.h"

VulkanRenderingBackend::VulkanRenderingBackend(Window& window)
	: m_window(window), m_vulkanDevice(window), m_vulkanRenderer(window, m_vulkanDevice)
{
	//1 uniform frame buffer per frame in flight (2) and 2 textures, one for albedo other is shadow map
	std::unordered_map<VkDescriptorType, uint32_t> descriptorTypes = { {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2}, {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2} };
	m_vulkanDescriptorPool = std::make_unique<VulkanDescriptorPool>(m_vulkanDevice, descriptorTypes);
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
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_simpleRenderSystem->getVulkanPipeline().getPipelineLayout(), 0,
		1, &m_cameraMatrixDescriptorSets[frameIndex], 0, nullptr);

	m_deferredRendererSystem->executeGeometryPass(commandBuffer);
	m_deferredRendererSystem->executeLightingPass(commandBuffer);
	
	static bool forwardRenderer;

	if (InputManager::getKey(GLFW_KEY_SPACE)) { forwardRenderer = true; }
	else { forwardRenderer = false; }

	if (forwardRenderer)
	{
		m_vulkanRenderer.beginRendering(commandBuffer);
		m_simpleRenderSystem->renderGameObjects(commandBuffer, dt);
		m_vulkanRenderer.endRendering(commandBuffer);
	}
	else
	{

		//transitioning swapchain image for being blitted to
		VkImageMemoryBarrier2 imgBarrier = {};
		imgBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		imgBarrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
		imgBarrier.srcAccessMask = VK_ACCESS_2_NONE;
		imgBarrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		imgBarrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		imgBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imgBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imgBarrier.image = m_vulkanRenderer.getSwapChain().getSwapChainImage(m_vulkanRenderer.m_currentImageIndex);
		imgBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imgBarrier.subresourceRange.baseMipLevel = 0;
		imgBarrier.subresourceRange.levelCount = 1;
		imgBarrier.subresourceRange.baseArrayLayer = 0;
		imgBarrier.subresourceRange.layerCount = 1;

		VkDependencyInfo dependencyInfo = {};
		dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		dependencyInfo.imageMemoryBarrierCount = 1;
		dependencyInfo.pImageMemoryBarriers = &imgBarrier;

		vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
		int width = m_vulkanRenderer.getSwapChain().getSwapChainWidth();
		int height = m_vulkanRenderer.getSwapChain().getSwapChainHeight();

		VkImageBlit2 region = {};
		region.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;
		region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.srcSubresource.layerCount = 1;
		region.srcOffsets[1] = { width, height, 1 };

		region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.dstSubresource.layerCount = 1;
		region.dstOffsets[1] = { width, height, 1 };

		VkBlitImageInfo2 blitImgInfo{};
		blitImgInfo.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2;
		blitImgInfo.srcImage = m_deferredRendererSystem->getLightingPassOutput().getVkImage();
		blitImgInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		blitImgInfo.dstImage = m_vulkanRenderer.getSwapChain().getSwapChainImage(m_vulkanRenderer.m_currentImageIndex);
		blitImgInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		blitImgInfo.regionCount = 1;
		blitImgInfo.pRegions = &region;

		vkCmdBlitImage2(commandBuffer, &blitImgInfo);

		//transition image layout for presenation
		VkImageMemoryBarrier2 presentBarrier = {};
		presentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		imgBarrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		imgBarrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		presentBarrier.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
		presentBarrier.dstAccessMask = 0;
		presentBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		presentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		presentBarrier.image = m_vulkanRenderer.getSwapChain().getSwapChainImage(m_vulkanRenderer.m_currentImageIndex);
		presentBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		presentBarrier.subresourceRange.baseMipLevel = 0;
		presentBarrier.subresourceRange.levelCount = 1;
		presentBarrier.subresourceRange.baseArrayLayer = 0;
		presentBarrier.subresourceRange.layerCount = 1;

		VkDependencyInfo presentDependencyInfo{};
		presentDependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		presentDependencyInfo.imageMemoryBarrierCount = 1;
		presentDependencyInfo.pImageMemoryBarriers = &presentBarrier;

		vkCmdPipelineBarrier2(commandBuffer, &presentDependencyInfo);
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
	VulkanDescriptorWriter descriptorWriter(*m_vulkanDescriptorPool.get(), *m_globalSetLayout.get());
	m_simpleRenderSystem->createPipeline(*m_globalSetLayout.get());
	m_deferredRendererSystem->createPipelines(*m_globalSetLayout.get());

	m_cameraMatrixDescriptorSets.resize(m_vulkanRenderer.getSwapChain().MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < m_cameraMatrixDescriptorSets.size(); i++)
	{
		VkDescriptorBufferInfo descriptorBufferInfo{};
		descriptorBufferInfo.buffer = m_cameraMatrixBuffers[i].getBuffer();
		descriptorBufferInfo.offset = 0;
		descriptorBufferInfo.range = sizeof(FrameUbo);

		descriptorWriter.writeBuffer(&descriptorBufferInfo, 0).build(m_cameraMatrixDescriptorSets[i]);
	}
}