#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <optional>
#include <stdexcept>
#include <vector>
#include <iostream>
#include <set>

#include "Window.h"


struct SwapChainSupportDetails 
{
	VkSurfaceCapabilitiesKHR m_surfaceCapabilites;
	std::vector<VkSurfaceFormatKHR> m_formats;
	std::vector<VkPresentModeKHR> m_presentModes;
};

struct QueueFamilyIndices 
{
	std::optional<uint32_t> m_graphicsQueue;
	std::optional<uint32_t> m_presentQueue;
	bool isComplete() const { return m_graphicsQueue.has_value() && m_presentQueue.has_value(); }
};

class VulkanDevice
{
public:
	VulkanDevice(const VulkanDevice& vkdevice) = delete;
	VulkanDevice(VulkanDevice& vkdevice) = delete;
	VulkanDevice& operator=(const VulkanDevice& vkDevice) = delete;
	VulkanDevice& operator=(VulkanDevice& vkDevice) = delete;

	VulkanDevice(Window& window);
	~VulkanDevice();

	SwapChainSupportDetails querySwapChainSupportDetails();
	//bool checkDeviceExtensionSupport();

	VkInstance getVkInstance() const;
	VkDevice getDevice() const;
	VkSurfaceKHR getSurface() const;
	VkQueue getGraphicsQueue() const;
	VkQueue getPresentQueue() const;
	VkCommandPool getCommandPool() const;

private:

	QueueFamilyIndices findQueueIndices(VkPhysicalDevice vk);

	void createVkInstance();
	void createSurface();
	void pickPhysicalDevice();
	void createDevice();
	void createCommandBuffer();

	VkInstance m_vkInstance;
	VkPhysicalDevice m_vkPhysicalDevice = VK_NULL_HANDLE;
	VkDevice m_vkDevice;
	VkPhysicalDeviceProperties m_vkPhysicalDeviceProperties;
	Window& m_window;
	VkSurfaceKHR m_vkSurfaceKHR;
	VkQueue m_graphicsQueue;
	VkQueue m_presentQueue;
	VkCommandPool m_vkCommandPool;

	std::vector<const char*> m_deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
};

