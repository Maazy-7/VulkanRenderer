#include "VulkanDescriptor.h"


VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(VulkanDevice& device)
	:m_vkDevice(device)
{
	VkDescriptorSetLayoutBinding descSetLayoutBinding{};
	descSetLayoutBinding.binding = 0;
	descSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descSetLayoutBinding.descriptorCount = 1;
	descSetLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	descSetLayoutBinding.pImmutableSamplers = VK_NULL_HANDLE;

	VkDescriptorSetLayoutCreateInfo descSetLayoutCreateInfo{};
	descSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descSetLayoutCreateInfo.bindingCount = 1;
	descSetLayoutCreateInfo.pBindings = &descSetLayoutBinding;
	if (vkCreateDescriptorSetLayout(m_vkDevice.getDevice(), &descSetLayoutCreateInfo, nullptr, &m_vkDescriptorSetLayout) != VK_SUCCESS) 
	{
		throw std::runtime_error("failed to create Vulkan Descriptor set layout");
	}
}

VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout() 
{
	vkDestroyDescriptorSetLayout(m_vkDevice.getDevice(), m_vkDescriptorSetLayout, nullptr);
}

const VkDescriptorSetLayout& VulkanDescriptorSetLayout::getDescriptorSetLayout() const 
{
	return m_vkDescriptorSetLayout;
}