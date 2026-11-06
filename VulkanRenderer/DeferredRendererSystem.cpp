#include "DeferredRendererSystem.h"

VkVertexInputBindingDescription QuadVertex::getBindingDescription()
{
	VkVertexInputBindingDescription bindingDescription{};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(QuadVertex);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 2> QuadVertex::getAttributeDescriptions()
{
	std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(QuadVertex, pos);

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(QuadVertex, texCoords);

	return attributeDescriptions;
}

DeferredRendererSystem::DeferredRendererSystem(VulkanDevice& vulkanDevice, const VulkanRenderer& vulkanRenderer)
	:m_vulkanDevice(vulkanDevice), m_vulkanRenderer(vulkanRenderer)
{
	uint32_t width = m_vulkanRenderer.getSwapChain().getSwapChainWidth();
	uint32_t height = m_vulkanRenderer.getSwapChain().getSwapChainHeight();
	VkSampler gbufferTexturesSampler = m_vulkanDevice.getDefaultTextureSampler();

	//8MB depth buffer - vertex position will be derived from depth buffer
	m_gDepth = std::make_unique<VulkanImage>(m_vulkanDevice, width, height, VK_FORMAT_D32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, 
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, true, gbufferTexturesSampler);
	//31.6MB normal and metallic buffer, stores xyz normal and w for the metallic material map
	m_gNormalMetallic = std::make_unique<VulkanImage>(m_vulkanDevice, width, height, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, 
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT, true, gbufferTexturesSampler);
	//8MB albedo and roughness buffer, stores xyz albedo/diffuse map and w for the roughness material map
	m_gAlbedoRoughness = std::make_unique<VulkanImage>(m_vulkanDevice, width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, 
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT, true, gbufferTexturesSampler);
	//8MB output buffer for the lighting pass
	m_lightingPassOutput = std::make_unique<VulkanImage>(m_vulkanDevice, width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, 
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT, true, gbufferTexturesSampler);

	//creating screen quad buffer thats used for lighting pass output
	size_t vertSize = sizeof(QuadVertex) * 4;
	VulkanBuffer stagingBuffer(m_vulkanDevice, 4, vertSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	m_screenQuadVertexBuffer = std::make_unique<VulkanBuffer>(m_vulkanDevice, 4, vertSize, 
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	std::array<QuadVertex, 4> quadVertices = {
		QuadVertex{ { -1.f, -1.f }, { 0.f, 0.f } },
		QuadVertex{ { 1.f, -1.f }, { 1.f, 0.f } },
		QuadVertex{ { -1.f, 1.f }, { 0.f, 1.f } },
		QuadVertex{ { 1.f, 1.f }, { 1.f, 1.f } }
	};

	//allocating screen quad vertex buffer with quad vertices
	stagingBuffer.map(0, vertSize);
	stagingBuffer.writeToBuffer(0, vertSize, quadVertices.data());
	stagingBuffer.unMap();
	VulkanBuffer::copyBuffer(stagingBuffer, *m_screenQuadVertexBuffer.get(), vertSize);


	//creating descriptor pool used for both geometry and lighting pass
	//4 combined image samplers - depth buffer, normal buffer, albedo spec buffer and shadow map
	std::unordered_map<VkDescriptorType, uint32_t> descriptorTypes = { {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 6} };
	m_vulkanDescriptorPool = std::make_unique<VulkanDescriptorPool>(m_vulkanDevice, descriptorTypes);
}

DeferredRendererSystem::~DeferredRendererSystem()
{

}

void DeferredRendererSystem::createPipelines(VulkanDescriptorSetLayout& descriptorSetLayout, VulkanDescriptorSetLayout& globalTextureDescriptorSetLayout)
{
	//creating geometry pass pipeline
	//variables and structs for the geometry pass pipeline
	std::vector<std::pair<VkShaderStageFlagBits, const std::string>> geometryShaderStages = {
		{VK_SHADER_STAGE_VERTEX_BIT, "Shaders/deferredGeometryPass.vert.spv"}, {VK_SHADER_STAGE_FRAGMENT_BIT, "Shaders/deferredGeometryPass.frag.spv"} };
	VkVertexInputBindingDescription bindingDescription = Vertex::getBindingDescription();
	std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions = Vertex::getAttributeDescriptions();
	float blendConstants[4] = { 0.f, 0.f, 0.f, 0.f };
	VkPipelineColorBlendAttachmentState colorBlendAttachmentState{};
	colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachmentState.blendEnable = VK_FALSE; colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
	VkPipelineColorBlendAttachmentState colorBlendAttachmentStates[2] = { colorBlendAttachmentState, colorBlendAttachmentState };
	VkDynamicState dynamicStates[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkFormat colorAttachments[1] = { m_vulkanRenderer.getSwapChain().getSwapChainFormat() };
	VkFormat geometryPassAttachments[2] = { VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_R8G8B8A8_SRGB };
	m_geometryPassPipeline = std::make_unique<VulkanPipeline>(m_vulkanDevice, geometryShaderStages);
	
	VkPushConstantRange geometryPassPushConstants{};
	geometryPassPushConstants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	geometryPassPushConstants.offset = 0;
	geometryPassPushConstants.size = 64;//one mat4 being the model matrix

	VkPushConstantRange geometryPassPushConstants2{};//extra push constant to match other pipeline layout
	geometryPassPushConstants2.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	geometryPassPushConstants2.offset = 64;
	geometryPassPushConstants2.size = 16;//will add more later but for now just a singular index for the texture descriptor list
	VkPushConstantRange pushConstants[2] = { geometryPassPushConstants, geometryPassPushConstants2 };

	std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;
	/*VulkanDescriptorSetLayout::addBinding(bindings, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1);
	VulkanDescriptorSetLayout::addBinding(bindings, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1);
	m_geometryPassDescriptorSets.resize(1);
	m_geometryPassSetLayout = std::make_unique<VulkanDescriptorSetLayout>(m_vulkanDevice, bindings);
	VulkanDescriptorWriter descriptorWriter(*m_vulkanDescriptorPool.get(), *m_geometryPassSetLayout.get());
	for (size_t i = 0; i < m_geometryPassDescriptorSets.size(); i++) 
	{
		VkDescriptorImageInfo descriptorImgInfo1{};
		descriptorImgInfo1.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		descriptorImgInfo1.imageView = diffuseTexture->getVkImageView();
		descriptorImgInfo1.sampler = diffuseTexture->getVkSampler();

		VkDescriptorImageInfo descriptorImgInfo2{};
		descriptorImgInfo2.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		descriptorImgInfo2.imageView = specularTexture->getVkImageView();
		descriptorImgInfo2.sampler = specularTexture->getVkSampler();
		
		descriptorWriter.writeImage(&descriptorImgInfo1, 0).writeImage(&descriptorImgInfo2, 1).build(m_geometryPassDescriptorSets[i]);
	}*/

	VkDescriptorSetLayout geometryPassSetLayouts[2] = { descriptorSetLayout.getDescriptorSetLayout(), globalTextureDescriptorSetLayout.getDescriptorSetLayout() };
	m_geometryPassPipeline->createPipelineLayout(2, geometryPassSetLayouts, 2, pushConstants);

	m_geometryPassPipeline->vertexInputState(1, &bindingDescription, static_cast<uint32_t>(attributeDescriptions.size()), attributeDescriptions.data())
		.inputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE)
		.viewportState(1, VK_NULL_HANDLE, 1, VK_NULL_HANDLE)
		.rasterizationState(VK_FALSE, VK_FALSE, VK_POLYGON_MODE_FILL, 1.f, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, 0.f, 0.f, 0.f)
		.multiSampleState(VK_FALSE, VK_SAMPLE_COUNT_1_BIT, 1.f, VK_NULL_HANDLE, VK_FALSE, VK_FALSE)
		.colorBlendState(VK_FALSE, VK_LOGIC_OP_COPY, 2, colorBlendAttachmentStates, blendConstants)
		.depthStencilState(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS, VK_FALSE, 0.f, 1.f, VK_FALSE)
		.dynamicState(2, dynamicStates)
		.pipelineRendering(2, geometryPassAttachments, m_vulkanDevice.findDepthFormat(), VK_FORMAT_UNDEFINED)
		.build();

	//creating lighting pass pipeline
	std::vector<std::pair<VkShaderStageFlagBits, const std::string>> lightingShaderStages = {
		{VK_SHADER_STAGE_VERTEX_BIT, "Shaders/deferredLightingPass.vert.spv"}, {VK_SHADER_STAGE_FRAGMENT_BIT, "Shaders/deferredLightingPass.frag.spv"} };
	m_lightingPassPipeline = std::make_unique<VulkanPipeline>(m_vulkanDevice, lightingShaderStages);
	VkVertexInputBindingDescription quadBindingDescription = QuadVertex::getBindingDescription();
	std::array<VkVertexInputAttributeDescription, 2> quadAttributeDescriptions = QuadVertex::getAttributeDescriptions();

	VkPushConstantRange lightingPassPushConstants1{};
	lightingPassPushConstants1.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	lightingPassPushConstants1.offset = 0;
	lightingPassPushConstants1.size = 64;// 64 mat4 <- inverse projection mat for reconstrucing world pos from depth buffer

	VkPushConstantRange lightingPassPushConstants2{};
	lightingPassPushConstants2.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	lightingPassPushConstants2.offset = 64;
	lightingPassPushConstants2.size = 16;//vec4 view position

	VkPushConstantRange lightingPassPushConstants[2] = { lightingPassPushConstants1, lightingPassPushConstants2 };

	bindings.clear();
	VulkanDescriptorSetLayout::addBinding(bindings, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1);
	VulkanDescriptorSetLayout::addBinding(bindings, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1);
	VulkanDescriptorSetLayout::addBinding(bindings, 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1);
	VulkanDescriptorSetLayout::addBinding(bindings, 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1);
	m_lightingPassDescriptorSets.resize(1);
	m_lightingPassSetLayout = std::make_unique<VulkanDescriptorSetLayout>(m_vulkanDevice, bindings);
	VulkanDescriptorWriter descriptorWriter2(*m_vulkanDescriptorPool.get(), *m_lightingPassSetLayout.get());
	for (size_t i = 0; i < m_lightingPassDescriptorSets.size(); i++) 
	{
		VkDescriptorImageInfo depthImageInfo{};
		depthImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		depthImageInfo.imageView = m_gDepth->getVkImageView();
		depthImageInfo.sampler = m_gDepth->getVkSampler();

		VkDescriptorImageInfo normalImageInfo{};
		normalImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		normalImageInfo.imageView = m_gNormalMetallic->getVkImageView();
		normalImageInfo.sampler = m_gNormalMetallic->getVkSampler();

		VkDescriptorImageInfo albedoSpecImageInfo{};
		albedoSpecImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		albedoSpecImageInfo.imageView = m_gAlbedoRoughness->getVkImageView();
		albedoSpecImageInfo.sampler = m_gAlbedoRoughness->getVkSampler();

		VkDescriptorImageInfo shadowMapImageInfo{};
		shadowMapImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		//for now we will allocate only 1 shadow map
		size_t shadowCasterIndex; (*m_lightsList)[0].hasShadowCaster(shadowCasterIndex);
		const ShadowCaster& shadowCaster = (*m_shadowCasterList)[shadowCasterIndex];
		shadowMapImageInfo.imageView = shadowCaster.getShadowMap().getVkImageView();
		shadowMapImageInfo.sampler = shadowCaster.getShadowMap().getVkSampler();

		descriptorWriter2.writeImage(&depthImageInfo, 0)
			.writeImage(&normalImageInfo, 1)
			.writeImage(&albedoSpecImageInfo, 2)
			.writeImage(&shadowMapImageInfo, 3).build(m_lightingPassDescriptorSets[0]);
	}
	VkFormat lightingPassColorFormat = VK_FORMAT_R8G8B8A8_SRGB;
	VkDescriptorSetLayout lightingPassSetLayouts[2] = { descriptorSetLayout.getDescriptorSetLayout(), m_lightingPassSetLayout->getDescriptorSetLayout() };
	m_lightingPassPipeline->createPipelineLayout(2, lightingPassSetLayouts, 2, lightingPassPushConstants);
	m_lightingPassPipeline->vertexInputState(1, &quadBindingDescription, static_cast<uint32_t>(quadAttributeDescriptions.size()), quadAttributeDescriptions.data())
		.inputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, VK_FALSE)
		.viewportState(1, VK_NULL_HANDLE, 1, VK_NULL_HANDLE)
		.rasterizationState(VK_FALSE, VK_FALSE, VK_POLYGON_MODE_FILL, 1.f, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, 0.f, 0.f, 0.f)
		.multiSampleState(VK_FALSE, VK_SAMPLE_COUNT_1_BIT, 1.f, VK_NULL_HANDLE, VK_FALSE, VK_FALSE)
		.colorBlendState(VK_FALSE, VK_LOGIC_OP_COPY, 1, &colorBlendAttachmentState, blendConstants)
		.dynamicState(2, dynamicStates)
		.pipelineRendering(1, &lightingPassColorFormat, VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED)
		.build();

	//creating pipeline to render the lights, it will write to lighting pass output and use the depth buffer from geometry pass and act as a forward renderer
	std::vector<std::pair<VkShaderStageFlagBits, const std::string>> lightObjRenderShaderStages = {
		{VK_SHADER_STAGE_VERTEX_BIT, "Shaders/shader.vert.spv"}, {VK_SHADER_STAGE_FRAGMENT_BIT, "Shaders/singleColor.frag.spv"} };
	m_lightObjRenderingPipeline = std::make_unique<VulkanPipeline>(m_vulkanDevice, lightObjRenderShaderStages);
	m_lightObjRenderingPipeline->createPipelineLayout(1, &lightingPassSetLayouts[0], 2, pushConstants);//pipeline descriptors match geometry and lighting pipelines
	m_lightObjRenderingPipeline->vertexInputState(1, &bindingDescription, static_cast<uint32_t>(attributeDescriptions.size()), attributeDescriptions.data())
		.inputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE)
		.viewportState(1, VK_NULL_HANDLE, 1, VK_NULL_HANDLE)
		.rasterizationState(VK_FALSE, VK_FALSE, VK_POLYGON_MODE_FILL, 1.f, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, 0.f, 0.f, 0.f)
		.multiSampleState(VK_FALSE, VK_SAMPLE_COUNT_1_BIT, 1.f, VK_NULL_HANDLE, VK_FALSE, VK_FALSE)
		.colorBlendState(VK_FALSE, VK_LOGIC_OP_COPY, 1, &colorBlendAttachmentState, blendConstants)
		.depthStencilState(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS, VK_FALSE, 0.f, 1.f, VK_FALSE)
		.dynamicState(2, dynamicStates)
		.pipelineRendering(1, &lightingPassColorFormat, m_vulkanDevice.findDepthFormat(), VK_FORMAT_UNDEFINED)
		.build();
}

void DeferredRendererSystem::executeGeometryPass(VkCommandBuffer commandBuffer)
{
	//transition gbuffer attachment layout for writing to them
	m_gDepth->transitionImageLayout(VK_FORMAT_D32_SFLOAT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT, 1, commandBuffer);
	m_gNormalMetallic->transitionImageLayout(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, 1, commandBuffer);
	m_gAlbedoRoughness->transitionImageLayout(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, 1, commandBuffer);

	//render attachment infos
	VkRenderingAttachmentInfo depthAttachmentInfo{};
	depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	depthAttachmentInfo.imageView = m_gDepth->getVkImageView();
	depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
	depthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthAttachmentInfo.clearValue = { 1.f, 0.f };

	VkRenderingAttachmentInfo gNormalAttachmentInfo{};
	gNormalAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	gNormalAttachmentInfo.imageView = m_gNormalMetallic->getVkImageView();
	gNormalAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	gNormalAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	gNormalAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	gNormalAttachmentInfo.clearValue.color = { {0.f, 0.f, 0.f, 1.f} };

	VkRenderingAttachmentInfo gAlbedoSpecAttachmentInfo{};
	gAlbedoSpecAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	gAlbedoSpecAttachmentInfo.imageView = m_gAlbedoRoughness->getVkImageView();
	gAlbedoSpecAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	gAlbedoSpecAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	gAlbedoSpecAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	gAlbedoSpecAttachmentInfo.clearValue.color = { {0.f, 0.f, 0.f, 1.f} };

	VkRenderingAttachmentInfo colorAttachments[2] = { gNormalAttachmentInfo, gAlbedoSpecAttachmentInfo };

	VkRenderingInfo renderingInfo{};
	renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	renderingInfo.renderArea.offset = { 0, 0 };
	renderingInfo.renderArea.extent = m_vulkanRenderer.getSwapChain().getSwapChainExtent();
	renderingInfo.layerCount = 1;
	renderingInfo.colorAttachmentCount = 2;
	renderingInfo.pColorAttachments = colorAttachments;
	renderingInfo.pDepthAttachment = &depthAttachmentInfo;
	renderingInfo.pStencilAttachment = VK_NULL_HANDLE;

	vkCmdBeginRendering(commandBuffer, &renderingInfo);

	VkViewport viewport{};
	viewport.width = static_cast<float>(m_vulkanRenderer.getSwapChain().getSwapChainWidth());
	viewport.height = static_cast<float>(m_vulkanRenderer.getSwapChain().getSwapChainHeight());
	viewport.maxDepth = 1.f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{ {0,0}, {m_vulkanRenderer.getSwapChain().getSwapChainWidth(), m_vulkanRenderer.getSwapChain().getSwapChainHeight()} };
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	m_geometryPassPipeline->bindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS);

	glm::mat4 model;
	metallicRoughnessMaterial material;
	for (const gameObject& gameObj : *m_gameObjectList) 
	{
		model = gameObj.getModelMat();
		material = gameObj.getMaterial();
		glm::ivec4 indices = material.getTextureIndices();
		if (indices.w == 0) { indices.w = -1; }
		vkCmdPushConstants(commandBuffer, m_geometryPassPipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &model);
		vkCmdPushConstants(commandBuffer, m_geometryPassPipeline->getPipelineLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 64, sizeof(glm::ivec4), &indices);
		gameObj.render(commandBuffer);
	}
	vkCmdEndRendering(commandBuffer);

	//transition gbuffer attachment layout for reading from them
	m_gDepth->transitionImageLayout(VK_FORMAT_D32_SFLOAT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, 1, commandBuffer);
	m_gNormalMetallic->transitionImageLayout(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
		VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, 1, commandBuffer);
	m_gAlbedoRoughness->transitionImageLayout(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
		VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, 1, commandBuffer);
}

void DeferredRendererSystem::executeLightingPass(VkCommandBuffer commandBuffer)//TODO add support for multiple lights
{
	//gbuffer attachments already transitioned to shader read only attachments from the geometry pass

	//transition lighting pass output to color attachment bit
	m_lightingPassOutput->transitionImageLayout(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, 1, commandBuffer);

	//color attachment info
	VkRenderingAttachmentInfo colorAttachmentInfo{};
	colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	colorAttachmentInfo.imageView = m_lightingPassOutput->getVkImageView();
	colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentInfo.clearValue.color = { {0.f, 0.f, 0.f, 1.f} };

	VkRenderingInfo renderingInfo{};
	renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	renderingInfo.renderArea.offset = { 0, 0 };
	renderingInfo.renderArea.extent = m_vulkanRenderer.getSwapChain().getSwapChainExtent();
	renderingInfo.layerCount = 1;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachments = &colorAttachmentInfo;
	renderingInfo.pDepthAttachment = VK_NULL_HANDLE;
	renderingInfo.pStencilAttachment = VK_NULL_HANDLE;

	vkCmdBeginRendering(commandBuffer, &renderingInfo);

	VkViewport viewport{};
	viewport.width = static_cast<float>(m_vulkanRenderer.getSwapChain().getSwapChainWidth());
	viewport.height = static_cast<float>(m_vulkanRenderer.getSwapChain().getSwapChainHeight());
	viewport.maxDepth = 1.f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{ {0,0}, {m_vulkanRenderer.getSwapChain().getSwapChainWidth(), m_vulkanRenderer.getSwapChain().getSwapChainHeight()} };
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	m_lightingPassPipeline->bindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS);

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_lightingPassPipeline->getPipelineLayout(), 1, 1,
		m_lightingPassDescriptorSets.data(), 0, nullptr);

	glm::mat4 invViewProjMat = m_camera->getProjection(); invViewProjMat[1][1] *= -1;
	invViewProjMat *= m_camera->getView();
	invViewProjMat = glm::inverse(invViewProjMat);
	vkCmdPushConstants(commandBuffer, m_lightingPassPipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &invViewProjMat);

	m_screenQuadVertexBuffer->bindVertexBuffer(commandBuffer);
	vkCmdDraw(commandBuffer, 4, 1, 0, 0);

	vkCmdEndRendering(commandBuffer);
	m_lightingPassOutput->transitionImageLayout(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_2_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT, 
		1, commandBuffer);
}

void DeferredRendererSystem::renderLights(VkCommandBuffer commandBuffer)
{
	//setting lighting pass output to color attachment as it will be written to
	m_lightingPassOutput->transitionImageLayout(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_ACCESS_2_TRANSFER_READ_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT, 
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, 1, commandBuffer);
	//setting depth buffer to depth stencil attachment as it will be read from
	m_gDepth->transitionImageLayout(VK_FORMAT_D32_SFLOAT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 
		VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT, 
		VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT, 1, commandBuffer);

	//reads from depth buffer
	VkRenderingAttachmentInfo depthAttachmentInfo{};
	depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
	depthAttachmentInfo.imageView = m_gDepth->getVkImageView();
	depthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	depthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	//writes to lighting pass output
	VkRenderingAttachmentInfo colorAttachmentInfo{};
	colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	colorAttachmentInfo.imageView = m_lightingPassOutput->getVkImageView();
	colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentInfo.clearValue.color = { {0.f, 0.f, 0.f, 1.f} };

	VkRenderingInfo renderingInfo{};
	renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	renderingInfo.renderArea.offset = { 0, 0 };
	renderingInfo.renderArea.extent = m_vulkanRenderer.getSwapChain().getSwapChainExtent();
	renderingInfo.layerCount = 1;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachments = &colorAttachmentInfo;
	renderingInfo.pDepthAttachment = &depthAttachmentInfo;
	renderingInfo.pStencilAttachment = VK_NULL_HANDLE;

	vkCmdBeginRendering(commandBuffer, &renderingInfo);

	VkViewport viewport{};
	viewport.width = static_cast<float>(m_vulkanRenderer.getSwapChain().getSwapChainWidth());
	viewport.height = static_cast<float>(m_vulkanRenderer.getSwapChain().getSwapChainHeight());
	viewport.maxDepth = 1.f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{ {0,0}, {m_vulkanRenderer.getSwapChain().getSwapChainWidth(), m_vulkanRenderer.getSwapChain().getSwapChainHeight()} };
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	m_lightObjRenderingPipeline->bindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS);

	glm::mat4 model;
	glm::vec4 color;
	for (const Light& light : *m_lightsList) 
	{
		model = glm::translate(glm::mat4(1.f), light.getPos()) * glm::scale(glm::mat4(1.f), glm::vec3(0.1f));
		color = light.getColor();
		vkCmdPushConstants(commandBuffer, m_lightObjRenderingPipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &model);
		vkCmdPushConstants(commandBuffer, m_lightObjRenderingPipeline->getPipelineLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 64, sizeof(glm::vec4), &color);
		light.draw(commandBuffer);
	}

	vkCmdEndRendering(commandBuffer);

	m_lightingPassOutput->transitionImageLayout(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_2_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT,
		1, commandBuffer);
	m_gDepth->transitionImageLayout(VK_FORMAT_D32_SFLOAT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 
		VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT,VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT, 
		VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, 1, commandBuffer);
}

void DeferredRendererSystem::setLightList(std::vector<Light>* lights)
{
	m_lightsList = lights;
}

void DeferredRendererSystem::setGameObjectList(std::vector<gameObject>* gameObjects)
{
	m_gameObjectList = gameObjects;
}

void DeferredRendererSystem::setShadowCasterList(const std::vector<ShadowCaster>* shadowCasters)
{
	m_shadowCasterList = shadowCasters;
}

void DeferredRendererSystem::attachCamera(const Camera* camera) 
{
	m_camera = camera;
}

void DeferredRendererSystem::recreateDeferredRendererResources()
{
	uint32_t width = m_vulkanRenderer.getSwapChain().getSwapChainWidth();
	uint32_t height = m_vulkanRenderer.getSwapChain().getSwapChainHeight();
	VkSampler gbufferTexturesSampler = m_vulkanDevice.getDefaultTextureSampler();

	//8MB depth buffer - vertex position will be derived from depth buffer
	m_gDepth = std::make_unique<VulkanImage>(m_vulkanDevice, width, height, VK_FORMAT_D32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, true, gbufferTexturesSampler);
	//31.6MB normal buffer
	m_gNormalMetallic = std::make_unique<VulkanImage>(m_vulkanDevice, width, height, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT, true, gbufferTexturesSampler);
	//8MB albedo and specular buffer, xyz stores diffuse color whereas w stores specualr intensity
	m_gAlbedoRoughness = std::make_unique<VulkanImage>(m_vulkanDevice, width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT, true, gbufferTexturesSampler);
	//8MB output buffer for the lighting pass
	m_lightingPassOutput = std::make_unique<VulkanImage>(m_vulkanDevice, width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT, true, gbufferTexturesSampler);

	//recreate descriptor sets for the resources that were created
	VulkanDescriptorWriter descriptorWriter2(*m_vulkanDescriptorPool.get(), *m_lightingPassSetLayout.get());
	for (size_t i = 0; i < m_lightingPassDescriptorSets.size(); i++)
	{
		VkDescriptorImageInfo depthImageInfo{};
		depthImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		depthImageInfo.imageView = m_gDepth->getVkImageView();
		depthImageInfo.sampler = m_gDepth->getVkSampler();

		VkDescriptorImageInfo normalImageInfo{};
		normalImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		normalImageInfo.imageView = m_gNormalMetallic->getVkImageView();
		normalImageInfo.sampler = m_gNormalMetallic->getVkSampler();

		VkDescriptorImageInfo albedoSpecImageInfo{};
		albedoSpecImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		albedoSpecImageInfo.imageView = m_gAlbedoRoughness->getVkImageView();
		albedoSpecImageInfo.sampler = m_gAlbedoRoughness->getVkSampler();

		//technically don't need shadow map resources to be recreated - should make recreation be only part of vector
		VkDescriptorImageInfo shadowMapImageInfo{};
		shadowMapImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		//for now we will allocate only 1 shadow map
		size_t shadowCasterIndex; (*m_lightsList)[0].hasShadowCaster(shadowCasterIndex);
		const ShadowCaster& shadowCaster = (*m_shadowCasterList)[shadowCasterIndex];
		shadowMapImageInfo.imageView = shadowCaster.getShadowMap().getVkImageView();
		shadowMapImageInfo.sampler = shadowCaster.getShadowMap().getVkSampler();

		//overwriting existing descrptor set
		descriptorWriter2.writeImage(&depthImageInfo, 0)
			.writeImage(&normalImageInfo, 1)
			.writeImage(&albedoSpecImageInfo, 2)
			.writeImage(&shadowMapImageInfo, 3).overwrite(m_lightingPassDescriptorSets[0]);
	}
}

VulkanImage& DeferredRendererSystem::getDepthBuffer()
{
	return *m_gDepth;
}

VulkanImage& DeferredRendererSystem::getNormalMetallicBuffer()
{
	return *m_gNormalMetallic;
}

VulkanImage& DeferredRendererSystem::getAlbedoRoughnessBuffer()
{
	return *m_gAlbedoRoughness;
}

VulkanImage& DeferredRendererSystem::getLightingPassOutput()
{
	return *m_lightingPassOutput;
}

VulkanPipeline& DeferredRendererSystem::getGeometryPassPipeline()
{
	return *m_geometryPassPipeline;
}
