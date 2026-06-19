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

