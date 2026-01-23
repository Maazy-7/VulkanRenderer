#pragma once

#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>
#include "VulkanDevice.h"

class VulkanSwapChain
{
public:

	VulkanSwapChain(const VulkanSwapChain&) = delete;
	VulkanSwapChain& operator=(const VulkanSwapChain&) = delete;

	VulkanSwapChain(VulkanDevice& device);


	~VulkanSwapChain();
};