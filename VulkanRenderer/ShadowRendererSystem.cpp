#include "ShadowRendererSystem.h"

VkVertexInputBindingDescription PosOnlyVertex::getBindingDescription()
{
	VkVertexInputBindingDescription bindingDescription{};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(PosOnlyVertex);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return bindingDescription;
}

VkVertexInputAttributeDescription PosOnlyVertex::getAttributeDescription() 
{
	VkVertexInputAttributeDescription vertexInputAttributeDescription{};
	vertexInputAttributeDescription.binding = 0;
	vertexInputAttributeDescription.location = 0;
	vertexInputAttributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
	vertexInputAttributeDescription.offset = offsetof(PosOnlyVertex, pos);
	return vertexInputAttributeDescription;
}

ShadowRendererSystem::ShadowRendererSystem(VulkanDevice& vulkanDevice, Camera& camera, std::vector<Model>& models)
	:m_vulkanDevice(vulkanDevice), m_camera(camera), m_models(models)
{
	//creating shadow map image

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
	if (vkCreateSampler(m_vulkanDevice.getDevice(), &shadowSamplerCreateInfo, nullptr, &sampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create vulkan Sampler");
	}
	m_shadowMap = std::make_unique<VulkanImage>(m_vulkanDevice, 2048, 2048, VK_FORMAT_D32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, true, sampler);
	
	//variables for pipeline create info structs
	std::vector<std::pair<VkShaderStageFlagBits, const std::string>> shaderStages = {
		{VK_SHADER_STAGE_VERTEX_BIT, "Shaders/shadowMapDepthShader.vert.spv"}, {VK_SHADER_STAGE_FRAGMENT_BIT, "Shaders/shadowMapDepthShader.frag.spv"} };
	VkVertexInputBindingDescription bindingDescription = Vertex::getBindingDescription();
	std::array<VkVertexInputAttributeDescription, 3> attributeDescription = Vertex::getAttributeDescriptions();
	VkDynamicState dynamicStates[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = 128;//2 matrices model and light matrix

	m_shadowMapRenderingPipeline = std::make_unique<VulkanPipeline>(m_vulkanDevice, shaderStages);
	m_shadowMapRenderingPipeline->createPipelineLayout(0, nullptr, 1, &pushConstantRange);

	m_shadowMapRenderingPipeline->vertexInputState(1, &bindingDescription, 3, attributeDescription.data())
		.inputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE)
		.viewportState(1, VK_NULL_HANDLE, 1, VK_NULL_HANDLE)
		.rasterizationState(VK_FALSE, VK_FALSE, VK_POLYGON_MODE_FILL, 1.0f, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, 0.f, 0.f, 0.f)
		.multiSampleState(VK_FALSE, VK_SAMPLE_COUNT_1_BIT, 1.f, VK_NULL_HANDLE, VK_FALSE, VK_FALSE)
		.depthStencilState(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL, VK_FALSE, 0.f, 1.f, VK_FALSE)
		.dynamicState(2, dynamicStates)
		.pipelineRendering(0, VK_NULL_HANDLE, VK_FORMAT_D32_SFLOAT, VK_FORMAT_UNDEFINED)
		.build();
}

ShadowRendererSystem::~ShadowRendererSystem()
{

}

void ShadowRendererSystem::renderShadowPass(VkCommandBuffer commandBuffer, const glm::mat4* lightSpaceMatrix)
{
	m_shadowMap->transitionImageLayout(VK_FORMAT_D32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		0, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT);

	VkRenderingAttachmentInfo depthAttachmentInfo{};
	depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	depthAttachmentInfo.imageView = m_shadowMap->getVkImageView();
	depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthAttachmentInfo.clearValue = { 1.f, 0.f };

	VkRenderingInfo renderingInfo{};
	renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	renderingInfo.renderArea = { {0, 0}, {2048, 2048} };
	renderingInfo.layerCount = 1;
	renderingInfo.pDepthAttachment = &depthAttachmentInfo;
	renderingInfo.colorAttachmentCount = 0;

	vkCmdBeginRendering(commandBuffer, &renderingInfo);

	VkViewport viewport{};
	viewport.width = 2048;
	viewport.height = 2048;
	viewport.maxDepth = 1.f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{ {0,0}, {2048, 2048} };
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	m_shadowMapRenderingPipeline->bindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS);
	//pushing light space matrix
	vkCmdPushConstants(commandBuffer, m_shadowMapRenderingPipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), lightSpaceMatrix);


	glm::mat4 model = glm::scale(glm::mat4(1.f), glm::vec3(0.5f));
	vkCmdPushConstants(commandBuffer, m_shadowMapRenderingPipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 64, sizeof(glm::mat4), &model);
	m_models[0].draw(commandBuffer);
	
	//bottom wall
	model = glm::translate(glm::mat4(1.f), glm::vec3(0.f, -1.f, 0.f)) * glm::scale(glm::mat4(1.f), glm::vec3(3.f, 0.15f, 3.f));
	vkCmdPushConstants(commandBuffer, m_shadowMapRenderingPipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 64, sizeof(glm::mat4), &model);
	m_models[0].draw(commandBuffer);

	////top wall
	//ubo.model = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 3.15f, 0.f)) * glm::scale(glm::mat4(1.f), glm::vec3(3.f, 0.15f, 3.f));
	//vkCmdPushConstants(commandBuffer, m_shadowMapRenderingPipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &ubo.model);
	//vkCmdPushConstants(commandBuffer, m_shadowMapRenderingPipeline->getPipelineLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 96, sizeof(glm::vec4), &white);//offset for color at 96
	//m_models[0].draw(commandBuffer);

	//right wall
	model = glm::translate(glm::mat4(1.f), glm::vec3(3.15f, 1.f, 0.f)) * glm::scale(glm::mat4(1.f), glm::vec3(0.15f, 2.f, 3.f));
	vkCmdPushConstants(commandBuffer, m_shadowMapRenderingPipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 64, sizeof(glm::mat4), &model);
	m_models[0].draw(commandBuffer);
	
	//left wall
	model = glm::translate(glm::mat4(1.f), glm::vec3(-3.15f, 1.f, 0.f)) * glm::scale(glm::mat4(1.f), glm::vec3(0.15f, 2.f, 3.f));
	vkCmdPushConstants(commandBuffer, m_shadowMapRenderingPipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 64, sizeof(glm::mat4), &model);
	m_models[0].draw(commandBuffer);
	
	//back wall
	model = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 1.f, -3.15f)) * glm::scale(glm::mat4(1.f), glm::vec3(3.f, 2.f, 0.15f));
	vkCmdPushConstants(commandBuffer, m_shadowMapRenderingPipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 64, sizeof(glm::mat4), &model);
	m_models[0].draw(commandBuffer);

	vkCmdEndRendering(commandBuffer);

	//m_shadowMap->transitionImageLayout(VK_FORMAT_D32_SFLOAT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		//VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
}

VulkanImage& ShadowRendererSystem::getShadowMap() const
{
	return *m_shadowMap.get();
}
