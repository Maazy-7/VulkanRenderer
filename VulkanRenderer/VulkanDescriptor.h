#pragma once

#include <glm/glm.hpp>

#include "VulkanDevice.h"

class VulkanDescriptorSetLayout
{
public:
    VulkanDescriptorSetLayout(const VulkanDescriptorSetLayout&) = delete;
    VulkanDescriptorSetLayout& operator=(const VulkanDescriptorSetLayout&) = delete;

    VulkanDescriptorSetLayout(VulkanDevice& device);
    ~VulkanDescriptorSetLayout();

    const VkDescriptorSetLayout& getDescriptorSetLayout() const;

private:

    VulkanDevice& m_vkDevice;
    VkDescriptorSetLayout m_vkDescriptorSetLayout;
};

class VulkanDescriptorPool 
{
public:
    VulkanDescriptorPool(const VulkanDescriptorPool&) = delete;
    VulkanDescriptorPool& operator=(const VulkanDescriptorPool&) = delete;

    VulkanDescriptorPool(VulkanDevice& device, uint32_t descriptorCount);
    ~VulkanDescriptorPool();

    void allocateDescriptorSet();

private:

    VulkanDevice& m_vkDevice;
    VkDescriptorPool m_vkDescriptorPool;
    uint32_t m_descriptorCount;
};
