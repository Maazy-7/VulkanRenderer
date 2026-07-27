#include "SimpleRenderSystem.h"

SimpleRenderSystem::SimpleRenderSystem(VulkanDevice& device, const VulkanRenderer& renderer, VulkanDescriptorPool& descriptorPool, Camera& camera)
	:m_vkDevice(device),
	m_vulkanRenderer(renderer),
	m_vulkanDescriptorPool(descriptorPool),
	m_camera(camera),
	m_model(std::make_unique<Model>(m_vkDevice, "Assets/metalCube.obj"))
{

	ubo.model = glm::mat4(1.f);
	ubo.view = m_camera.getView();
	ubo.proj = m_camera.getProjection();
	ubo.proj[1][1] *= -1;
	
	m_vkDescriptorSets.resize(1);//descriptor set containing 1 texture

	m_texture = std::make_unique<Texture>(m_vkDevice, "Assets/cubeMetalTexture.png", VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	//temp sampler for shadows
	VkSamplerCreateInfo shadowSamplerCreateInfo{};
	shadowSamplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	shadowSamplerCreateInfo.minFilter = VK_FILTER_LINEAR;
	shadowSamplerCreateInfo.magFilter = VK_FILTER_LINEAR;
	shadowSamplerCreateInfo.anisotropyEnable = VK_FALSE;
	shadowSamplerCreateInfo.maxAnisotropy = 1.f;
	shadowSamplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	shadowSamplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	shadowSamplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	shadowSamplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	shadowSamplerCreateInfo.compareEnable = VK_TRUE;
	shadowSamplerCreateInfo.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	
	VkSampler sampler;
	if (vkCreateSampler(m_vkDevice.getDevice(), &shadowSamplerCreateInfo, nullptr, &sampler) != VK_SUCCESS) 
	{
		throw std::runtime_error("failed to create vulkan Sampler");
	}
	m_shadowMap = std::make_unique<VulkanImage>(m_vkDevice, 1024, 1024, VK_FORMAT_D32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, true, sampler);
	
	//creating descriptor layout and sets
	std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;
	VulkanDescriptorSetLayout::addBinding(bindings, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1);
	m_renderSystemDescriptorSetLayout = std::make_unique<VulkanDescriptorSetLayout>(m_vkDevice, bindings);
	VulkanDescriptorWriter descriptorWriter(m_vulkanDescriptorPool, *m_renderSystemDescriptorSetLayout.get());
	for (size_t i = 0; i < m_vkDescriptorSets.size(); i++)
	{
		VkDescriptorImageInfo descriptorImageInfo{};
		descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		descriptorImageInfo.imageView = m_texture->getVkImageView();
		descriptorImageInfo.sampler = m_texture->getVkSampler();

		descriptorWriter.writeImage(&descriptorImageInfo, 0).build(m_vkDescriptorSets[i]);
	}
}

void SimpleRenderSystem::createPipeline(VulkanDescriptorSetLayout& vulkanDescriptorSetLayout) 
{
	//variables and structs for pipeline create info structs
	std::vector<std::pair<VkShaderStageFlagBits, const std::string>> shaderStages = {
		{VK_SHADER_STAGE_VERTEX_BIT, "Shaders/shader.vert.spv"}, {VK_SHADER_STAGE_FRAGMENT_BIT, "Shaders/shader.frag.spv"} };
	VkVertexInputBindingDescription bindingDescription = Vertex::getBindingDescription();
	std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = Vertex::getAttributeDescriptions();
	float blendConstants[4] = {0.f, 0.f, 0.f, 0.f};
	VkPipelineColorBlendAttachmentState colorBlendAttachmentState{};
	colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachmentState.blendEnable = VK_FALSE; colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
	VkDynamicState dynamicStates[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkFormat colorAttachments[1] = { m_vulkanRenderer.getSwapChain().getSwapChainFormat() };


	m_vkPipeline = std::make_unique<VulkanPipeline>(m_vkDevice, shaderStages, m_vulkanRenderer.getSwapChain());
	
	//pipeline layout
	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(glm::mat4);

	VkPushConstantRange lightPushConstantRange{};
	lightPushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	lightPushConstantRange.offset = 64;
	lightPushConstantRange.size = 48;//3 vec4, light pos, view pos and object color so that can be used instead of texture

	VkPushConstantRange pushConstants[2] = { pushConstantRange, lightPushConstantRange};

	VkDescriptorSetLayout descriptorSetLayouts[2] =
	{ vulkanDescriptorSetLayout.getDescriptorSetLayout(), m_renderSystemDescriptorSetLayout->getDescriptorSetLayout() };
	m_vkPipeline->createPipelineLayout(2, descriptorSetLayouts, 2, pushConstants);

	//pipeline create info structs
	m_vkPipeline->vertexInputState(1, &bindingDescription, static_cast<uint32_t>(attributeDescriptions.size()), Vertex::getAttributeDescriptions().data())
		.inputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE)
		.viewportState(1, VK_NULL_HANDLE, 1, VK_NULL_HANDLE)
		.rasterizationState(VK_FALSE, VK_FALSE, VK_POLYGON_MODE_FILL, 1.f, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, 0.f, 0.f, 0.f)
		.multiSampleState(VK_FALSE, VK_SAMPLE_COUNT_1_BIT, 1.f, VK_NULL_HANDLE, VK_FALSE, VK_FALSE)
		.colorBlendState(VK_FALSE, VK_LOGIC_OP_COPY, 1, &colorBlendAttachmentState, blendConstants)
		.depthStencilState(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS, VK_FALSE, 0.f, 1.f, VK_FALSE)
		.dynamicState(2, dynamicStates)
		.pipelineRendering(1, colorAttachments, m_vkDevice.findDepthFormat(), VK_FORMAT_UNDEFINED)
		.build();

	//creating pipeline to render lights
	std::vector<std::pair<VkShaderStageFlagBits, const std::string>> singleColorShaderStages = {
		{VK_SHADER_STAGE_VERTEX_BIT, "Shaders/shader.vert.spv"}, {VK_SHADER_STAGE_FRAGMENT_BIT, "Shaders/singleColor.frag.spv"} };
	m_singleColorObjectPipeline = std::make_unique<VulkanPipeline>(m_vkDevice, singleColorShaderStages, m_vulkanRenderer.getSwapChain());
	VkPushConstantRange singleColorPushConstantRange{};
	singleColorPushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	singleColorPushConstantRange.offset = 64;
	singleColorPushConstantRange.size = 48;//matching other pipeline's push constant size

	VkPushConstantRange singlColorPushConstants[2] = { pushConstantRange, singleColorPushConstantRange };

	m_singleColorObjectPipeline->createPipelineLayout(1, descriptorSetLayouts, 2, singlColorPushConstants);//uses only the camera matrices descriptor set

	m_singleColorObjectPipeline->vertexInputState(1, &bindingDescription, static_cast<uint32_t>(attributeDescriptions.size()), Vertex::getAttributeDescriptions().data())
		.inputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE)
		.viewportState(1, VK_NULL_HANDLE, 1, VK_NULL_HANDLE)
		.rasterizationState(VK_FALSE, VK_FALSE, VK_POLYGON_MODE_FILL, 1.f, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, 0.f, 0.f, 0.f)
		.multiSampleState(VK_FALSE, VK_SAMPLE_COUNT_1_BIT, 1.f, VK_NULL_HANDLE, VK_FALSE, VK_FALSE)
		.colorBlendState(VK_FALSE, VK_LOGIC_OP_COPY, 1, &colorBlendAttachmentState, blendConstants)
		.depthStencilState(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS, VK_FALSE, 0.f, 1.f, VK_FALSE)
		.dynamicState(2, dynamicStates)
		.pipelineRendering(1, colorAttachments, m_vkDevice.findDepthFormat(), VK_FORMAT_UNDEFINED)
		.build();
}

void SimpleRenderSystem::renderGameObjects(VkCommandBuffer commandBuffer, float dt)
{
	
	m_vkPipeline->bindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vkPipeline->getPipelineLayout(), 1, 1, 
		&m_vkDescriptorSets[0], 0, nullptr);
	
	glm::vec4 red = glm::vec4(0.65f, 0.05f, 0.05f, 1.f);
	glm::vec4 green = glm::vec4(0.12f, 0.45f, 0.15f, 1.f);
	glm::vec4 white = glm::vec4(0.73f);

	static float count = 0.f;
	count += dt;
	ubo.model = glm::mat4(1.f);//reset model matrix
	float x = std::cos(2.f*count);
	float y = std::sin(2.f*count);
	cubeMat = glm::translate(glm::mat4(1.f), glm::vec3(x, 0.f, y)) * glm::scale(glm::mat4(1.f), glm::vec3(0.5f));
	vkCmdPushConstants(commandBuffer, m_vkPipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &cubeMat);
	
	glm::vec4 LightPos = glm::vec4(0.f, 2.f, 0.f, 0.f);
	glm::vec4 ViewPos = glm::vec4(m_camera.getPos(), 1.f);
	LightData lightData{}; lightData.lightPos = LightPos; lightData.viewPos = ViewPos;
	vkCmdPushConstants(commandBuffer, m_vkPipeline->getPipelineLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 64, sizeof(LightData), &lightData);
	vkCmdPushConstants(commandBuffer, m_vkPipeline->getPipelineLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 96, sizeof(glm::vec4), &white);//offset for color at 96
	m_model->draw(commandBuffer);

	//bottom wall
	ubo.model = glm::translate(glm::mat4(1.f), glm::vec3(0.f, -1.f, 0.f)) * glm::scale(glm::mat4(1.f), glm::vec3(3.f, 0.15f, 3.f));
	vkCmdPushConstants(commandBuffer, m_vkPipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &ubo.model);
	vkCmdPushConstants(commandBuffer, m_vkPipeline->getPipelineLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 96, sizeof(glm::vec4), &white);//offset for color at 96
	m_model->draw(commandBuffer);
	
	//top wall
	ubo.model = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 3.15f, 0.f)) * glm::scale(glm::mat4(1.f), glm::vec3(3.f, 0.15f, 3.f));
	vkCmdPushConstants(commandBuffer, m_vkPipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &ubo.model);
	vkCmdPushConstants(commandBuffer, m_vkPipeline->getPipelineLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 96, sizeof(glm::vec4), &white);//offset for color at 96
	m_model->draw(commandBuffer);
	
	//right wall
	ubo.model = glm::translate(glm::mat4(1.f), glm::vec3(3.15f, 1.f, 0.f)) * glm::scale(glm::mat4(1.f), glm::vec3(0.15f, 2.f, 3.f));
	vkCmdPushConstants(commandBuffer, m_vkPipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &ubo.model);
	vkCmdPushConstants(commandBuffer, m_vkPipeline->getPipelineLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 96, sizeof(glm::vec4), &red);//offset for color at 96
	m_model->draw(commandBuffer);
	//left wall
	ubo.model = glm::translate(glm::mat4(1.f), glm::vec3(-3.15f, 1.f, 0.f)) * glm::scale(glm::mat4(1.f), glm::vec3(0.15f, 2.f, 3.f));
	vkCmdPushConstants(commandBuffer, m_vkPipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &ubo.model);
	vkCmdPushConstants(commandBuffer, m_vkPipeline->getPipelineLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 96, sizeof(glm::vec4), &green);//offset for color at 96
	m_model->draw(commandBuffer);
	//back wall
	ubo.model = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 1.f, -3.15f)) * glm::scale(glm::mat4(1.f), glm::vec3(3.f, 2.f, 0.15f));
	vkCmdPushConstants(commandBuffer, m_vkPipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &ubo.model);
	vkCmdPushConstants(commandBuffer, m_vkPipeline->getPipelineLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 96, sizeof(glm::vec4), &white);//offset for color at 96
	m_model->draw(commandBuffer);

	m_singleColorObjectPipeline->bindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS);
	
	ubo.model = glm::translate(glm::mat4(1.f), glm::vec3(lightData.lightPos)) * glm::scale(glm::mat4(1.f), glm::vec3(0.1f));
	vkCmdPushConstants(commandBuffer, m_singleColorObjectPipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &ubo.model);
	glm::vec4 lightColor = glm::vec4(1.f);
	vkCmdPushConstants(commandBuffer, m_singleColorObjectPipeline->getPipelineLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 64, sizeof(LightData), &lightColor);
	m_model->draw(commandBuffer);
}

std::vector<VkDescriptorSet>& SimpleRenderSystem::getDescriptorSets() 
{
	return m_vkDescriptorSets;
}

VulkanPipeline& SimpleRenderSystem::getVulkanPipeline() const
{
	return *m_vkPipeline.get();
}
