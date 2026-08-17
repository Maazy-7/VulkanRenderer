#pragma once

#include <vector>

#include "VulkanDevice.h"
#include "VulkanSwapChain.h"
#include "VulkanBuffer.h"
#include "VulkanImage.h"
#include "VulkanDescriptor.h"
#include "Model.h"

class VulkanPipeline
{
public:

	VulkanPipeline(const VulkanPipeline&) = delete;
	VulkanPipeline& operator=(const VulkanPipeline&) = delete;

	VulkanPipeline(VulkanDevice& device, std::vector<std::pair<VkShaderStageFlagBits, const std::string>>& shaderPaths);
	~VulkanPipeline();

	VkPipelineLayout getPipelineLayout() const;

	VkPipeline getPipeline() const;

	void bindPipeline(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint) const;

	[[nodiscard]] VulkanPipeline& vertexInputState(uint32_t vertexBindingDescriptionCount, VkVertexInputBindingDescription* pVertexBindingDescriptions, 
		uint32_t vertexAttributeDescriptionCount, VkVertexInputAttributeDescription* pVertexAttributeDescriptions, const void* pNext = nullptr);
	[[nodiscard]] VulkanPipeline& inputAssemblyState(VkPrimitiveTopology topology, VkBool32 primitiveRestartEnable, const void* pNext = nullptr);
	[[nodiscard]] VulkanPipeline& rasterizationState(VkBool32 depthClampEnable, VkBool32 rasterizeDiscardEnable, VkPolygonMode polygonMode, float lineWidth,
		VkCullModeFlags cullMode, VkFrontFace frontFace, VkBool32 depthBiasEnable, float depthBiasConstFactor, float depthBiasClamp, float depthBiasSlopeFactor, const void* pNext = nullptr);
	[[nodiscard]] VulkanPipeline& viewportState(uint32_t viewportCount, VkViewport* pViewport, uint32_t scissorCount, VkRect2D* pScissor, const void* pNext = nullptr);
	[[nodiscard]] VulkanPipeline& multiSampleState(VkBool32 sampleShadingEnable, VkSampleCountFlagBits rasterizationSamples, float minSampleShading, VkSampleMask* pSampleMask,
		VkBool32 alphaToConvergeEnable, VkBool32 alphaToOneEnable, const void* pNext = nullptr);
	[[nodiscard]] VulkanPipeline& colorBlendState(VkBool32 logicOpEnable, VkLogicOp logicOp, uint32_t attachmentCount,
		const VkPipelineColorBlendAttachmentState* pAttachments, float* blendConstants, const void* pNext = nullptr);
	[[nodiscard]] VulkanPipeline& depthStencilState(VkBool32 depthTestEnable, VkBool32 depthWriteEnable, VkCompareOp compareOp, VkBool32 depthBoundsTestEnable,
		float minDepthBounds, float maxDepthBounds, VkBool32 stencilTestEnable, const void* pNext = nullptr);
	[[nodiscard]] VulkanPipeline& dynamicState(uint32_t dynamicStateCount, VkDynamicState* pDynamicStates, const void* pNext = nullptr);
	[[nodiscard]] VulkanPipeline& pipelineRendering(uint32_t colorAttachmentCount, VkFormat* pColorAttachmentFormats, VkFormat depthAttachmentFormat,
		VkFormat stencilAttachmentFormat, uint32_t viewMask = 0, const void* pNext = nullptr);
	
	void createPipelineLayout(uint32_t setLayoutCount, VkDescriptorSetLayout* pSetLayouts, uint32_t pushConstantRangeCount, 
		VkPushConstantRange* pPushConstantRanges, const void* pNext = nullptr);
	void build();

private:
	
	VkShaderModule createShaderModule(const std::vector<char>& shaderCode);
	std::vector<char> readFromFile(const std::string& filename);

	VulkanDevice& m_vkDevice;
	VkPipeline m_vkPipeline;
	VkGraphicsPipelineCreateInfo* m_vkgraphicsPipelineCreateInfo;//create info struct and all the other structs it links to deleted after pipeline creation
	VkPipelineLayout m_vkPipelineLayout;
	std::vector<VkPipelineShaderStageCreateInfo>* m_shaderStages;
};

