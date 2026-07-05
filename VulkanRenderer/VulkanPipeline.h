#pragma once

#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>
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

private:

	void createGraphicsPipeline(const std::string& vertexPath, const std::string& fragmentPath, const VulkanSwapChain& swapChain);
	VkShaderModule createShaderModule(const std::vector<char>& shaderCode);
	std::vector<char> readFromFile(const std::string& filename);

	VulkanDevice& m_vkDevice;
	VkPipeline m_vkPipeline;
	VkPipelineLayout m_vkPipelineLayout;
	VulkanDescriptorSetLayout& m_vulkanDescriptorSetLayout;
};

