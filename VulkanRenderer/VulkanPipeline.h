#pragma once

#include <vector>

#include "VulkanDevice.h"
#include "VulkanSwapChain.h"
#include "VulkanBuffer.h"
#include "VulkanImage.h"
#include "VulkanDescriptor.h"
#include "Model.h"

struct PipelineConfigurationInfo
{

};

//TODO: so one make the pipeline creation similar to that of the descriptor writer thing ie pipeline.descriptor(args).multisample(args).blendstate.(args) etc
// second to also make shaders have only 1 file instead of seperate vert and frag etc - a quick parser will create the necessary shaders needed



class VulkanPipeline
{
public:

	VulkanPipeline(const VulkanPipeline&) = delete;
	VulkanPipeline& operator=(const VulkanPipeline&) = delete;

	VulkanPipeline(VulkanDevice& device, const std::string& vertexPath, const std::string& fragmentPath, const VulkanSwapChain& swapChain, 
		VulkanDescriptorSetLayout& vulkanDescriptorSetLayout);
	~VulkanPipeline();

	VkPipelineLayout getPipelineLayout() const;

	VkPipeline getPipeline() const;

	VulkanPipeline& vertexInputState(uint32_t vertexBindingDescriptionCount, VkVertexInputBindingDescription* pVertexInputBindingDescriptions, 
		uint32_t vertexAttributeDescriptionCount, VkVertexInputAttributeDescription* pVertexAttributeDescriptions, const void* pNext = nullptr);
	VulkanPipeline& inputAssemblyState(VkPrimitiveTopology topology, VkBool32 primitiveRestartEnable, const void* pNext = nullptr);
	VulkanPipeline& rasterizationState(VkBool32 depthClampEnable, VkBool32 rasterizeDiscardEnable, VkPolygonMode polygonMode, float lineWidth, 
		VkCullModeFlags cullMode, VkFrontFace frontFace, VkBool32 depthBiasEnable, float depthBiasConstFactor, float depthBiasClamp, float depthBiasSlopeFactor, const void* pNext = nullptr);
	VulkanPipeline& viewportState(uint32_t viewportCount, VkViewport* pViewport, uint32_t scissorCount, VkRect2D* pScissor);
	VulkanPipeline& multiSampleState(VkBool32 sampleShadingEnable, VkSampleCountFlagBits rasterizationSamples, float minSampleShading, VkSampleMask* pSampleMask,
		VkBool32 alphaToConvergeEnable, VkBool32 alphaToOneEnable, const void* = nullptr);
	VulkanPipeline& colorBlendState(VkBool32 logicOpEnable, VkLogicOp logicOp, uint32_t attachmentCount, 
		const VkPipelineColorBlendAttachmentState* pAttachments, float* blendConstants, const void* pNext = nullptr);
	VulkanPipeline& depthStencilState(VkBool32 depthTestEnable, VkBool32 depthWriteEnable, VkCompareOp compareOp, VkBool32 depthBoundsTestEnable,
		float minDepthBounds, float maxDepthBounds, VkBool32 stencilTestEnable, const void* pNext);
	VulkanPipeline& dynamicState(uint32_t dynamicStateCount, VkDynamicState* pDynamicStates, const void* pNext = nullptr);
	void build();
	

	void createPipelineLayout(uint32_t setLayoutCount, VkDescriptorSetLayout* pSetLayouts, uint32_t pushConstantRangeCount, VkPushConstantRange* pPushConstantRanges);

private:

	void createGraphicsPipeline(const std::string& vertexPath, const std::string& fragmentPath, const VulkanSwapChain& swapChain);
	VkShaderModule createShaderModule(const std::vector<char>& shaderCode);
	std::vector<char> readFromFile(const std::string& filename);

	VulkanDevice& m_vkDevice;
	VkPipeline m_vkPipeline;
	VkGraphicsPipelineCreateInfo m_vkgraphicsPipelineCreateInfo;
	VkPipelineLayout m_vkPipelineLayout;
	VulkanDescriptorSetLayout& m_vulkanDescriptorSetLayout;
	std::vector<const void*> m_graphicsPipelineCreateInfoStructs;
};

