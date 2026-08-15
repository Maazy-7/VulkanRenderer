#include "ShadowRendererSystem.h"

ShadowRendererSystem::ShadowRendererSystem(VulkanDevice& vulkanDevice, std::vector<Model>& models)
	:m_vulkanDevice(vulkanDevice), m_models(models)
{	
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

void ShadowRendererSystem::renderShadowPass(VkCommandBuffer commandBuffer)
{
	for (ShadowCaster& shadowCaster : m_shadowCasters) 
	{
		VulkanImage& shadowMap = *shadowCaster.m_shadowMap;
		shadowMap.transitionImageLayout(VK_FORMAT_D32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			0, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT, commandBuffer);

		VkRenderingAttachmentInfo depthAttachmentInfo{};
		depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		depthAttachmentInfo.imageView = shadowMap.getVkImageView();
		depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		depthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		depthAttachmentInfo.clearValue = { 1.f, 0.f };

		VkRenderingInfo renderingInfo{};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderingInfo.renderArea = { {0, 0}, {shadowCaster.m_shadowWidth, shadowCaster.m_shadowHeight} };
		renderingInfo.layerCount = 1;
		renderingInfo.pDepthAttachment = &depthAttachmentInfo;
		renderingInfo.colorAttachmentCount = 0;

		vkCmdBeginRendering(commandBuffer, &renderingInfo);

		VkViewport viewport{};
		viewport.width = static_cast<float>(shadowCaster.m_shadowWidth);
		viewport.height = static_cast<float>(shadowCaster.m_shadowHeight);
		viewport.maxDepth = 1.f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor{ {0,0}, {shadowCaster.m_shadowWidth, shadowCaster.m_shadowHeight} };
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		m_shadowMapRenderingPipeline->bindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS);

		//pushing light space matrix
		vkCmdPushConstants(commandBuffer, m_shadowMapRenderingPipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &shadowCaster.m_lightSpaceMatrix[0]);

		glm::mat4 model;
		for (gameObject& gameObj : *m_gameObjects)
		{
			model = gameObj.getModelMat();
			vkCmdPushConstants(commandBuffer, m_shadowMapRenderingPipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 64, sizeof(glm::mat4), &model);
			gameObj.render(commandBuffer);
		}
		vkCmdEndRendering(commandBuffer);

		shadowMap.transitionImageLayout(VK_FORMAT_D32_SFLOAT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
			VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, commandBuffer);
	}
}

void ShadowRendererSystem::setGameObjectList(std::vector<gameObject>* gameObjects)
{
	m_gameObjects = gameObjects;
}

void ShadowRendererSystem::createShadowCaster(Light& light, uint32_t shadowWidth, uint32_t shadowHeight, float nearPlane, float farPlane)
{
	m_shadowCasters.emplace_back(m_vulkanDevice, light, shadowWidth, shadowHeight, nearPlane, farPlane);
}

const std::vector<ShadowCaster>* ShadowRendererSystem::getShadowCasters() const
{
	return &m_shadowCasters;
}