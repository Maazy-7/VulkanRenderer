#include "SimpleRenderSystem.h"

SimpleRenderSystem::SimpleRenderSystem(VulkanDevice& device, const VulkanRenderer& renderer)
	:m_vkDevice(device),
	m_vulkanRenderer(renderer),
	m_camera(Camera(glm::vec3(0.f, 0.f, 2.f), m_vulkanRenderer.getSwapChain().getSwapChainAspect(), 0.1f, 100.f)),
	m_model(std::make_unique<Model>(m_vkDevice, "Assets/viking_room.obj"))
{
	//allocating uniform buffers
	for (size_t i = 0; i < m_vulkanRenderer.getSwapChain().MAX_FRAMES_IN_FLIGHT; i++) 
	{
		m_uniformBuffers.push_back(std::make_unique<VulkanBuffer>(m_vkDevice, 3/*temp*/, sizeof(UniformBufferObject),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
		m_uniformBuffers[i]->map(0, sizeof(UniformBufferObject));
	}

	ubo.model = glm::mat4(1.f);
	ubo.view = m_camera.getView();
	ubo.proj = m_camera.getProjection();
	ubo.proj[1][1] *= -1;
	
	m_vkDescriptorSets.resize(m_vulkanRenderer.getSwapChain().MAX_FRAMES_IN_FLIGHT);
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
	const VkDescriptorSetLayout* setLayout = &vulkanDescriptorSetLayout.getDescriptorSetLayout();
	m_vkPipeline->createPipelineLayout(1, const_cast<VkDescriptorSetLayout*>(setLayout), 0, nullptr);

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
}

void SimpleRenderSystem::renderGameObjects(VkCommandBuffer commandBuffer, float dt)
{
	m_camera.update(dt, m_vulkanRenderer.getSwapChain().getSwapChainAspect());
	//ubo.model = glm::rotate(ubo.model, dt * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = m_camera.getProjection();
	ubo.proj[1][1] *= -1;
	ubo.view = m_camera.getView();
	m_uniformBuffers[m_vulkanRenderer.getSwapChain().getCurrentFrameIndex()]->writeToBuffer(0, sizeof(UniformBufferObject), &ubo);

	m_vkPipeline->bindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vkPipeline->getPipelineLayout(), 0, 1, 
		&m_vkDescriptorSets[m_vulkanRenderer.getSwapChain().getCurrentFrameIndex()], 0, nullptr);
	m_model->draw(commandBuffer);
}

std::vector<VkDescriptorSet>& SimpleRenderSystem::getDescriptorSets() 
{
	return m_vkDescriptorSets;
}