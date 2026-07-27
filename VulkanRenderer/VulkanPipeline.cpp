#include "VulkanPipeline.h"

VulkanPipeline::VulkanPipeline(VulkanDevice& device, std::vector<std::pair<VkShaderStageFlagBits, const std::string>>& shaderPaths)
	:m_vkDevice(device), m_vkPipeline(VK_NULL_HANDLE), m_vkPipelineLayout(VK_NULL_HANDLE)
{
	m_vkgraphicsPipelineCreateInfo = new VkGraphicsPipelineCreateInfo{};
	m_vkgraphicsPipelineCreateInfo->sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	m_vkgraphicsPipelineCreateInfo->stageCount = static_cast<uint32_t>(shaderPaths.size());
	m_shaderStages = new std::vector<VkPipelineShaderStageCreateInfo>;//deleted after pipeline creation
	for (auto& path : shaderPaths) //shader paths should be in order, vertex, geom, tess, fragment
	{
		//path.first = shader type, path.second = shader path
		VkShaderModule shaderModule;
		if (m_vkDevice.getShader(path.second, shaderModule)) 
		{
			VkPipelineShaderStageCreateInfo shaderStageCreateInfo{};
			shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStageCreateInfo.stage = path.first;
			shaderStageCreateInfo.module = shaderModule;
			shaderStageCreateInfo.pName = "main";
			m_shaderStages->push_back(shaderStageCreateInfo);
		}
		else 
		{
			auto shaderCode = readFromFile(path.second);
			shaderModule = createShaderModule(shaderCode);
			m_vkDevice.addShader(path.second, shaderModule);

			VkPipelineShaderStageCreateInfo shaderStageCreateInfo{};
			shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStageCreateInfo.stage = path.first;
			shaderStageCreateInfo.module = shaderModule;
			shaderStageCreateInfo.pName = "main";
			m_shaderStages->push_back(shaderStageCreateInfo);
		}
	}
	m_vkgraphicsPipelineCreateInfo->pStages = m_shaderStages->data();
}

VulkanPipeline::~VulkanPipeline() 
{
	if (!m_vkPipelineLayout) 
	{
		throw std::runtime_error("vulkan pipeline layout object was never created");
	}
	vkDestroyPipelineLayout(m_vkDevice.getDevice(), m_vkPipelineLayout, nullptr);
	if (!m_vkPipeline)
	{
		throw std::runtime_error("vulkan pipeline object was never created");
	}
	vkDestroyPipeline(m_vkDevice.getDevice(), m_vkPipeline, nullptr);
}

VkShaderModule VulkanPipeline::createShaderModule(const std::vector<char>& shaderCode)
{
	VkShaderModuleCreateInfo shaderCreateMooduleInfo = {};
	shaderCreateMooduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderCreateMooduleInfo.codeSize = shaderCode.size();
	shaderCreateMooduleInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());
	VkShaderModule shaderModule;
	if (vkCreateShaderModule(m_vkDevice.getDevice(), &shaderCreateMooduleInfo, nullptr, &shaderModule) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Vulkan shader module");
	}
	return shaderModule;
}

std::vector<char> VulkanPipeline::readFromFile(const std::string& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);
	if (!file.is_open())
	{
		throw std::runtime_error("Failed to open shader file");
	}
	size_t fileSize = static_cast<uint32_t>(file.tellg());
	std::vector<char> buffer(fileSize);
	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();
	return buffer;
}

VkPipeline VulkanPipeline::getPipeline() const 
{
	return m_vkPipeline;
}

void VulkanPipeline::bindPipeline(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint) const
{
	vkCmdBindPipeline(commandBuffer, pipelineBindPoint, m_vkPipeline);
}

VulkanPipeline& VulkanPipeline::vertexInputState(uint32_t vertexBindingDescriptionCount, VkVertexInputBindingDescription* pVertexBindingDescriptions, 
	uint32_t vertexAttributeDescriptionCount, VkVertexInputAttributeDescription* pVertexAttributeDescriptions, const void* pNext)
{
	VkPipelineVertexInputStateCreateInfo* vertexInputStateCreateInfo = new VkPipelineVertexInputStateCreateInfo{};//deleted after pipeline creation
	vertexInputStateCreateInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputStateCreateInfo->vertexAttributeDescriptionCount = vertexAttributeDescriptionCount;
	vertexInputStateCreateInfo->pVertexAttributeDescriptions = pVertexAttributeDescriptions;
	vertexInputStateCreateInfo->vertexBindingDescriptionCount = vertexBindingDescriptionCount;
	vertexInputStateCreateInfo->pVertexBindingDescriptions = pVertexBindingDescriptions;
	vertexInputStateCreateInfo->pNext = pNext;
	m_vkgraphicsPipelineCreateInfo->pVertexInputState = vertexInputStateCreateInfo;
	return *this;
}

VulkanPipeline& VulkanPipeline::inputAssemblyState(VkPrimitiveTopology topology, VkBool32 primitiveRestartEnable, const void* pNext)
{
	VkPipelineInputAssemblyStateCreateInfo* inputAssemblyStateCreateInfo = new VkPipelineInputAssemblyStateCreateInfo{};//deleted after pipeline creation
	inputAssemblyStateCreateInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyStateCreateInfo->topology = topology;
	inputAssemblyStateCreateInfo->primitiveRestartEnable = primitiveRestartEnable;
	inputAssemblyStateCreateInfo->pNext = pNext;
	m_vkgraphicsPipelineCreateInfo->pInputAssemblyState = inputAssemblyStateCreateInfo;
	return *this;
}

VulkanPipeline& VulkanPipeline::rasterizationState(VkBool32 depthClampEnable, VkBool32 rasterizeDiscardEnable, VkPolygonMode polygonMode, float lineWidth, 
	VkCullModeFlags cullMode, VkFrontFace frontFace, VkBool32 depthBiasEnable, float depthBiasConstFactor, float depthBiasClamp, float depthBiasSlopeFactor, const void* pNext)
{
	VkPipelineRasterizationStateCreateInfo* rasterizationStateCreateInfo = new VkPipelineRasterizationStateCreateInfo{};//deleted after pipeline creation
	rasterizationStateCreateInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationStateCreateInfo->depthClampEnable = depthClampEnable;
	rasterizationStateCreateInfo->rasterizerDiscardEnable = rasterizeDiscardEnable;
	rasterizationStateCreateInfo->polygonMode = polygonMode;
	rasterizationStateCreateInfo->lineWidth = lineWidth;
	rasterizationStateCreateInfo->cullMode = cullMode;
	rasterizationStateCreateInfo->frontFace = frontFace;
	rasterizationStateCreateInfo->depthBiasEnable = depthBiasEnable;
	rasterizationStateCreateInfo->depthBiasConstantFactor = depthBiasConstFactor;
	rasterizationStateCreateInfo->depthBiasClamp = depthBiasClamp;
	rasterizationStateCreateInfo->depthBiasSlopeFactor = depthBiasSlopeFactor;
	rasterizationStateCreateInfo->pNext = pNext;
	m_vkgraphicsPipelineCreateInfo->pRasterizationState = rasterizationStateCreateInfo;
	return *this;
}

VulkanPipeline& VulkanPipeline::viewportState(uint32_t viewportCount, VkViewport* pViewport, uint32_t scissorCount, VkRect2D* pScissor, const void* pNext)
{
	VkPipelineViewportStateCreateInfo* viewportStateCreateInfo = new VkPipelineViewportStateCreateInfo{};//deleted after pipeline creation
	viewportStateCreateInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo->viewportCount = viewportCount;
	viewportStateCreateInfo->pViewports = pViewport;
	viewportStateCreateInfo->scissorCount = scissorCount;
	viewportStateCreateInfo->pScissors = pScissor;
	viewportStateCreateInfo->pNext = pNext;
	m_vkgraphicsPipelineCreateInfo->pViewportState = viewportStateCreateInfo;
	return *this;
}

VulkanPipeline& VulkanPipeline::multiSampleState(VkBool32 sampleShadingEnable, VkSampleCountFlagBits rasterizationSamples, float minSampleShading, 
	VkSampleMask* pSampleMask, VkBool32 alphaToConvergeEnable, VkBool32 alphaToOneEnable, const void* pNext)
{
	VkPipelineMultisampleStateCreateInfo* multisampleStateCreateInfo = new VkPipelineMultisampleStateCreateInfo{};//deleted after pipeline creation
	multisampleStateCreateInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleStateCreateInfo->sampleShadingEnable = sampleShadingEnable;
	multisampleStateCreateInfo->rasterizationSamples = rasterizationSamples;
	multisampleStateCreateInfo->minSampleShading = minSampleShading;
	multisampleStateCreateInfo->pSampleMask = pSampleMask;
	multisampleStateCreateInfo->alphaToCoverageEnable = alphaToConvergeEnable;
	multisampleStateCreateInfo->alphaToOneEnable = alphaToOneEnable;
	multisampleStateCreateInfo->pNext = pNext;
	m_vkgraphicsPipelineCreateInfo->pMultisampleState = multisampleStateCreateInfo;
	return *this;
}

VulkanPipeline& VulkanPipeline::colorBlendState(VkBool32 logicOpEnable, VkLogicOp logicOp, uint32_t attachmentCount, 
	const VkPipelineColorBlendAttachmentState* pAttachments, float* blendConstants, const void* pNext)
{
	VkPipelineColorBlendStateCreateInfo* colorBlendStateCreateInfo = new VkPipelineColorBlendStateCreateInfo{};//deleted after pipeline creation
	colorBlendStateCreateInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendStateCreateInfo->logicOpEnable = logicOpEnable;
	colorBlendStateCreateInfo->logicOp = logicOp;
	colorBlendStateCreateInfo->attachmentCount = attachmentCount;
	colorBlendStateCreateInfo->pAttachments = pAttachments;
	colorBlendStateCreateInfo->blendConstants[0] = blendConstants[0];
	colorBlendStateCreateInfo->blendConstants[1] = blendConstants[1];
	colorBlendStateCreateInfo->blendConstants[2] = blendConstants[2];
	colorBlendStateCreateInfo->blendConstants[3] = blendConstants[3];
	colorBlendStateCreateInfo->pNext = pNext;
	m_vkgraphicsPipelineCreateInfo->pColorBlendState = colorBlendStateCreateInfo;
	return *this;
}

VulkanPipeline& VulkanPipeline::depthStencilState(VkBool32 depthTestEnable, VkBool32 depthWriteEnable, VkCompareOp compareOp, 
	VkBool32 depthBoundsTestEnable, float minDepthBounds, float maxDepthBounds, VkBool32 stencilTestEnable, const void* pNext)
{
	VkPipelineDepthStencilStateCreateInfo* depthStencilStateCreateInfo = new VkPipelineDepthStencilStateCreateInfo{};//deleted after pipeline creation
	depthStencilStateCreateInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilStateCreateInfo->depthTestEnable = depthTestEnable;
	depthStencilStateCreateInfo->depthWriteEnable = depthWriteEnable;
	depthStencilStateCreateInfo->depthCompareOp = compareOp;
	depthStencilStateCreateInfo->depthBoundsTestEnable = depthBoundsTestEnable;
	depthStencilStateCreateInfo->minDepthBounds = minDepthBounds;
	depthStencilStateCreateInfo->maxDepthBounds = maxDepthBounds;
	depthStencilStateCreateInfo->stencilTestEnable = stencilTestEnable;
	depthStencilStateCreateInfo->pNext = pNext;
	m_vkgraphicsPipelineCreateInfo->pDepthStencilState = depthStencilStateCreateInfo;
	return *this;
}

VulkanPipeline& VulkanPipeline::dynamicState(uint32_t dynamicStateCount, VkDynamicState* pDynamicStates, const void* pNext)
{
	VkPipelineDynamicStateCreateInfo* dynamicStateCreateInfo = new VkPipelineDynamicStateCreateInfo{};//deleted after pipeline creation
	dynamicStateCreateInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCreateInfo->dynamicStateCount = dynamicStateCount;
	dynamicStateCreateInfo->pDynamicStates = pDynamicStates;
	dynamicStateCreateInfo->pNext = pNext;
	m_vkgraphicsPipelineCreateInfo->pDynamicState = dynamicStateCreateInfo;
	return *this;
}

VulkanPipeline& VulkanPipeline::pipelineRendering(uint32_t colorAttachmentCount, VkFormat* pColorAttachmentFormats, VkFormat depthAttachmentFormat, 
	VkFormat stencilAttachmentFormat, const void* pNext)
{
	VkPipelineRenderingCreateInfo* renderingCreateInfo = new VkPipelineRenderingCreateInfo{};//deleted after pipeline creation
	renderingCreateInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	renderingCreateInfo->colorAttachmentCount = colorAttachmentCount;
	renderingCreateInfo->pColorAttachmentFormats = pColorAttachmentFormats;
	renderingCreateInfo->depthAttachmentFormat = depthAttachmentFormat;
	renderingCreateInfo->stencilAttachmentFormat = stencilAttachmentFormat;
	renderingCreateInfo->pNext = pNext;
	m_vkgraphicsPipelineCreateInfo->pNext = renderingCreateInfo;
	return *this;
}

void VulkanPipeline::createPipelineLayout(uint32_t setLayoutCount, VkDescriptorSetLayout* pSetLayouts, uint32_t pushConstantRangeCount, 
	VkPushConstantRange* pPushConstantRanges, const void* pNext)
{
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = setLayoutCount;
	pipelineLayoutCreateInfo.pSetLayouts = pSetLayouts;
	pipelineLayoutCreateInfo.pushConstantRangeCount = pushConstantRangeCount;
	pipelineLayoutCreateInfo.pPushConstantRanges = pPushConstantRanges;
	pipelineLayoutCreateInfo.pNext = pNext;
	if (vkCreatePipelineLayout(m_vkDevice.getDevice(), &pipelineLayoutCreateInfo, nullptr, &m_vkPipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create pipeline layout");
	}
}

void VulkanPipeline::build()
{
	if (!m_vkPipelineLayout) 
	{
		throw std::runtime_error("pipeline layout was not created");
	}
	m_vkgraphicsPipelineCreateInfo->layout = m_vkPipelineLayout;
	m_vkgraphicsPipelineCreateInfo->basePipelineHandle = VK_NULL_HANDLE;
	m_vkgraphicsPipelineCreateInfo->basePipelineIndex = -1;
	if (vkCreateGraphicsPipelines(m_vkDevice.getDevice(), VK_NULL_HANDLE, 1, m_vkgraphicsPipelineCreateInfo, nullptr, &m_vkPipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Vulkan graphics pipeline");
	}

	//deleting all heap allocated pipeline create info structs
	m_shaderStages->clear(); //deletes the m_vkgraphicsPipelineCreateInfo->pStages data
	delete m_shaderStages;
	delete m_vkgraphicsPipelineCreateInfo->pVertexInputState;
	delete m_vkgraphicsPipelineCreateInfo->pInputAssemblyState;
	delete m_vkgraphicsPipelineCreateInfo->pRasterizationState;
	delete m_vkgraphicsPipelineCreateInfo->pViewportState;
	delete m_vkgraphicsPipelineCreateInfo->pMultisampleState;
	delete m_vkgraphicsPipelineCreateInfo->pDepthStencilState;
	delete m_vkgraphicsPipelineCreateInfo->pColorBlendState;
	delete m_vkgraphicsPipelineCreateInfo->pDynamicState;
	delete static_cast<const VkPipelineRenderingCreateInfo*>(m_vkgraphicsPipelineCreateInfo->pNext);

	//and finally deleting the graphics pipeline create info struct
	delete m_vkgraphicsPipelineCreateInfo;
}

VkPipelineLayout VulkanPipeline::getPipelineLayout() const 
{
	return m_vkPipelineLayout;
}