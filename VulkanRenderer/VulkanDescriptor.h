#pragma once

#include <glm/glm.hpp>
#include <unordered_map>

#include "VulkanDevice.h"

class VulkanDescriptorSetLayout
{
public:
    VulkanDescriptorSetLayout(const VulkanDescriptorSetLayout&) = delete;
    VulkanDescriptorSetLayout& operator=(const VulkanDescriptorSetLayout&) = delete;

    VulkanDescriptorSetLayout(VulkanDevice& device, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings);
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

    VulkanDescriptorPool(VulkanDevice& device, uint32_t descriptorCount);
    ~VulkanDescriptorPool();

    bool allocateDescriptorSet(const VulkanDescriptorSetLayout& descriptorLayout, VkDescriptorSet& descriptorSet);

private:

    VulkanDevice& m_vkDevice;
    VkDescriptorPool m_vkDescriptorPool;
    uint32_t m_descriptorCount;
};

class VulkanDescriptorWriter 
{
public:
    VulkanDescriptorWriter(const VulkanDescriptorWriter&) = delete;
    VulkanDescriptorWriter& operator=(const VulkanDescriptorWriter&) = delete;

    VulkanDescriptorWriter(VulkanDescriptorPool& vulkanDescriptorPool, VulkanDescriptorSetLayout& vulkanDescriptorSetLayout);
    ~VulkanDescriptorWriter();

    VulkanDescriptorWriter& writeBuffer(VkDescriptorBufferInfo* bufferInfo, uint32_t binding);
    VulkanDescriptorWriter& writeImage(VkDescriptorImageInfo* imageInfo, uint32_t binding);

    bool build(VkDescriptorSet& descriptorSet);
    void overwrite(VkDescriptorSet& descriptorSet);
private:

    VulkanDescriptorPool& m_vulkanDescriptorPool;
    VulkanDescriptorSetLayout& m_vulkanDescriptorSetLayout;
    std::vector<VkWriteDescriptorSet> m_descriptorWrites;
};
