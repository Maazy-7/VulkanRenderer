#include "Application.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

Application::Application()
	:m_window(std::make_unique<Window>(DEFAULT_WIDTH, DEFUALT_HEIGHT, "Application")),
	m_vkDevice(*m_window),
	m_vkRenderer(*m_window, m_vkDevice),
	m_simpleRenderSystem(m_vkDevice, m_vkRenderer.getSwapChain()),
	m_vkDescriptorPool(m_vkDevice, m_vkRenderer.getSwapChain().MAX_FRAMES_IN_FLIGHT)
{
	m_window->setKeyCallback(&InputManager::keyCallback);
	m_window->setCursorPosCallBack(&InputManager::cursorPosCallback);
	m_window->setMouseButtonCallBack(&InputManager::mouseButtonCallback);
	m_window->setScrollCallback(&InputManager::scrollCallback);
}

Application::~Application() 
{

}

void Application::run() 
{
	//temp
	
	//allocating texture and staging buffer for texture
	int width, height, channels;
	stbi_uc* pixels = stbi_load("Assets/statue.jpg", &width, &height, &channels, STBI_rgb_alpha);
	if (!pixels) { throw std::runtime_error("failed to load texture image"); }
	VkDeviceSize imageSize = static_cast<uint64_t>(width * height * 4);

	VulkanBuffer stagingBuffer(m_vkDevice, 1, imageSize, VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	stagingBuffer.map(0, imageSize);
	stagingBuffer.writeToBuffer(0, imageSize, pixels);
	stbi_image_free(pixels);

	VulkanImage textureImg(m_vkDevice, width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	textureImg.transitionImageLayout(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	textureImg.copyBufferToImage(stagingBuffer, width, height);
	textureImg.transitionImageLayout(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	
	//creating descriptor layout and sets
	std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;
	VulkanDescriptorSetLayout::addBinding(bindings, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1);
	VulkanDescriptorSetLayout::addBinding(bindings, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1);
	VulkanDescriptorSetLayout globalSetLayout(m_vkDevice, bindings);
	VulkanDescriptorWriter descriptorWriter(m_vkDescriptorPool, globalSetLayout);
	m_simpleRenderSystem.createPipeline(globalSetLayout);//VkPipeline needs to know about layout, so its created after descriptor layout
	
	for (size_t i = 0; i < 2; i++)
	{
		VkDescriptorBufferInfo descriptorBufferInfo{};
		descriptorBufferInfo.buffer = m_simpleRenderSystem.m_uniformBuffers[i]->getBuffer();
		descriptorBufferInfo.offset = 0;
		descriptorBufferInfo.range = sizeof(m_simpleRenderSystem.ubo);

		VkDescriptorImageInfo descriptorImageInfo{};
		descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		descriptorImageInfo.imageView = textureImg.getVkImageView();
		descriptorImageInfo.sampler = textureImg.getVkSampler();

		descriptorWriter.writeBuffer(&descriptorBufferInfo, 0).writeImage(&descriptorImageInfo, 1).build(m_simpleRenderSystem.m_vkDescriptorSets[i]);
	}
	//Camera camera(glm::vec3(0.f), m_window->getAspect());
	//temp
	while (!m_window->windowShouldClose()) 
	{
		if (InputManager::getKey(GLFW_KEY_ESCAPE)) { m_window->setWindowClose(true); }

		float dt = getDeltaTime();

		m_window->pollEvents();

		//scene update

		InputManager::update();

		//physics update

		//renderer update
		auto commandBuffer = m_vkRenderer.beginFrame();
		m_vkRenderer.beginRendering(commandBuffer);
		m_simpleRenderSystem.renderGameObjects(commandBuffer, dt);
		m_vkRenderer.endRendering(commandBuffer);
		m_vkRenderer.endFrame();

	}
	vkDeviceWaitIdle(m_vkDevice.getDevice());
}

float Application::getDeltaTime() 
{
	float currentTime = m_window->getTime();
	m_deltaTime = currentTime - m_prevTime;
	m_prevTime = currentTime;
	return m_deltaTime;
}