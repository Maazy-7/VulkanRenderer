#pragma once

#include <glm/glm.hpp>
#include <unordered_map>

#include "VulkanDevice.h"

class VulkanDescriptorSetLayout
{
public:
    VulkanDescriptorSetLayout(const VulkanDescriptorSetLayout&) = delete;
    VulkanDescriptorSetLayout& operator=(const VulkanDescriptorSetLayout&) = delete;

    VulkanDescriptorSetLayout(VulkanDevice& device, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings, 
        VkDescriptorSetLayoutCreateFlags flags = 0, VkDescriptorSetLayoutBindingFlagsCreateInfo* bindingFlagCreateInfo = VK_NULL_HANDLE);
    ~VulkanDescriptorSetLayout();

    static void addBinding(std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding>& bindings, uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags shaderStageFlags, uint32_t descriptorCount);

    const VkDescriptorSetLayout& getDescriptorSetLayout() const;

private:

    VulkanDevice& m_vkDevice;
    VkDescriptorSetLayout m_vkDescriptorSetLayout;
    std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> m_bindings;

    friend class VulkanDescriptorWriter;
};

class VulkanDescriptorPool 
{
public:
    VulkanDescriptorPool(const VulkanDescriptorPool&) = delete;
    VulkanDescriptorPool& operator=(const VulkanDescriptorPool&) = delete;

    VulkanDescriptorPool(VulkanDevice& device, std::unordered_map<VkDescriptorType, uint32_t> descriptorCountAndType,
        VkDescriptorPoolCreateFlags descriptorPoolCreateFlags = 0);
    ~VulkanDescriptorPool();

    bool allocateDescriptorSet(const VulkanDescriptorSetLayout& descriptorLayout, VkDescriptorSet& descriptorSet,
        VkDescriptorSetVariableDescriptorCountAllocateInfo* variableDescCountAllocInfo = VK_NULL_HANDLE);

private:

    VulkanDevice& m_vkDevice;
    VkDescriptorPool m_vkDescriptorPool;
};

class VulkanDescriptorWriter 
{
public:
    VulkanDescriptorWriter(const VulkanDescriptorWriter&) = delete;
    VulkanDescriptorWriter& operator=(const VulkanDescriptorWriter&) = delete;

    VulkanDescriptorWriter(VulkanDescriptorPool& vulkanDescriptorPool, VulkanDescriptorSetLayout& vulkanDescriptorSetLayout);
    ~VulkanDescriptorWriter();

    [[nodiscard]] VulkanDescriptorWriter& writeBuffer(VkDescriptorBufferInfo* bufferInfo, uint32_t binding);
    [[nodiscard]] VulkanDescriptorWriter& writeImage(VkDescriptorImageInfo* imageInfo, uint32_t binding);

    bool build(VkDescriptorSet& descriptorSet);
    void overwrite(VkDescriptorSet& descriptorSet);
private:

    VulkanDescriptorPool& m_vulkanDescriptorPool;
    VulkanDescriptorSetLayout& m_vulkanDescriptorSetLayout;
    std::vector<VkWriteDescriptorSet> m_descriptorWrites;
};
