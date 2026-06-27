#include "VulkanDescriptor.h"

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(VulkanDevice& device, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings)
	:m_vkDevice(device), m_bindings(bindings)
{
	std::vector<VkDescriptorSetLayoutBinding> descSetLayoutBindings;
	
	for (auto& binding : m_bindings) 
	{
		descSetLayoutBindings.push_back(binding.second);
	}
	
	VkDescriptorSetLayoutCreateInfo descSetLayoutCreateInfo{};
	descSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(descSetLayoutBindings.size());
	descSetLayoutCreateInfo.pBindings = descSetLayoutBindings.data();
	if (vkCreateDescriptorSetLayout(m_vkDevice.getDevice(), &descSetLayoutCreateInfo, nullptr, &m_vkDescriptorSetLayout) != VK_SUCCESS) 
	{
		throw std::runtime_error("failed to create Vulkan Descriptor set layout");
	}
}

VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout() 
{
	vkDestroyDescriptorSetLayout(m_vkDevice.getDevice(), m_vkDescriptorSetLayout, nullptr);
}

void VulkanDescriptorSetLayout::addBinding(std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding>& bindings, uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags shaderStageFlags, uint32_t descriptorCount)
{
	VkDescriptorSetLayoutBinding descSetLayoutBinding{};
	descSetLayoutBinding.binding = binding;
	descSetLayoutBinding.descriptorType = descriptorType;
	descSetLayoutBinding.stageFlags = shaderStageFlags;
	descSetLayoutBinding.descriptorCount = descriptorCount;
	bindings[binding] = descSetLayoutBinding;
}

const VkDescriptorSetLayout& VulkanDescriptorSetLayout::getDescriptorSetLayout() const 
{
	return m_vkDescriptorSetLayout;
}

VulkanDescriptorPool::VulkanDescriptorPool(VulkanDevice& device, uint32_t descriptorCount)
	:m_vkDevice(device), m_descriptorCount(descriptorCount)
{
	VkDescriptorPoolSize descriptorPoolSize{};
	descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorPoolSize.descriptorCount = descriptorCount;

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.poolSizeCount = 1;
	descriptorPoolCreateInfo.pPoolSizes = &descriptorPoolSize;
	descriptorPoolCreateInfo.maxSets = descriptorCount;

	if (vkCreateDescriptorPool(m_vkDevice.getDevice(), &descriptorPoolCreateInfo, nullptr, &m_vkDescriptorPool) != VK_SUCCESS) 
	{
		throw std::runtime_error("failed to create Vulkan descriptor pool");
	}
}

VulkanDescriptorPool::~VulkanDescriptorPool() 
{
	vkDestroyDescriptorPool(m_vkDevice.getDevice(), m_vkDescriptorPool, nullptr);
}

bool VulkanDescriptorPool::allocateDescriptorSet(const VulkanDescriptorSetLayout& descriptorLayout, VkDescriptorSet& descriptorSet) 
{
	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
	descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocateInfo.descriptorPool = m_vkDescriptorPool;
	descriptorSetAllocateInfo.descriptorSetCount = 1;
	descriptorSetAllocateInfo.pSetLayouts = &descriptorLayout.getDescriptorSetLayout();

	if (vkAllocateDescriptorSets(m_vkDevice.getDevice(), &descriptorSetAllocateInfo, &descriptorSet) != VK_SUCCESS) 
	{
		std::cout << "failed to allocate descriptor set to pool\n";
		return false;
	}
	return true;
}

VulkanDescriptorWriter::VulkanDescriptorWriter(VulkanDescriptorPool& vulkanDescriptorPool, VulkanDescriptorSetLayout& vulkanDescriptorSetLayout)
	:m_vulkanDescriptorPool(vulkanDescriptorPool), m_vulkanDescriptorSetLayout(vulkanDescriptorSetLayout)
{

}

VulkanDescriptorWriter::~VulkanDescriptorWriter() 
{

}

VulkanDescriptorWriter& VulkanDescriptorWriter::writeBuffer(VkDescriptorBufferInfo* bufferInfo, uint32_t binding) 
{
	VkWriteDescriptorSet writeDescriptorSet{};
	writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSet.dstBinding = binding;
	writeDescriptorSet.dstArrayElement = 0;
	writeDescriptorSet.pBufferInfo = bufferInfo;
	writeDescriptorSet.descriptorType = m_vulkanDescriptorSetLayout.m_bindings[binding].descriptorType;
	writeDescriptorSet.descriptorCount = 1;
	m_descriptorWrites.push_back(writeDescriptorSet);
	return *this;
}

bool VulkanDescriptorWriter::build(VkDescriptorSet& descriptorSet) 
{
	if (m_vulkanDescriptorPool.allocateDescriptorSet(m_vulkanDescriptorSetLayout, descriptorSet)) 
	{
		overwrite(descriptorSet);
		return true;
	}
	else 
	{
		return false;
	}
}

void VulkanDescriptorWriter::overwrite(VkDescriptorSet& descriptorSet) 
{
	for (auto& write : m_descriptorWrites) 
	{
		write.dstSet = descriptorSet;
	}
	vkUpdateDescriptorSets(m_vulkanDescriptorSetLayout.m_vkDevice.getDevice(), static_cast<uint32_t>(m_descriptorWrites.size()), m_descriptorWrites.data(), 0, nullptr);
}