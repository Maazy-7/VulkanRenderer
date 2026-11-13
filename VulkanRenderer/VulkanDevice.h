#pragma once

#include <optional>
#include <stdexcept>
#include <vector>
#include <iostream>
#include <set>

#include "Window.h"
#include "util.h"


const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
const bool enableValidationLayers = false;

#if DEBUG
enableValidationLayers = true;
#endif


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

namespace RayTracingFunctions
{
	//ray tracing functions
	extern PFN_vkCreateAccelerationStructureKHR m_vkCreateAccelerationStructureKHR;
	extern PFN_vkDestroyAccelerationStructureKHR m_vkDestroyAccelerationStructureKHR;
	extern PFN_vkGetAccelerationStructureBuildSizesKHR m_vkGetAccelerationStructureBuildSizesKHR;
	extern PFN_vkGetAccelerationStructureDeviceAddressKHR m_vkGetAccelerationStructureDeviceAddressKHR;
	extern PFN_vkCmdBuildAccelerationStructuresKHR m_vkCmdBuildAccelerationStructuresKHR;
	extern PFN_vkCreateRayTracingPipelinesKHR m_vkCreateRayTracingPipelinesKHR;
	extern PFN_vkGetRayTracingShaderGroupHandlesKHR m_vkGetRayTracingShaderGroupHandlesKHR;
	extern PFN_vkCmdTraceRaysKHR m_vkCmdTraceRaysKHR;
}

class VulkanDevice
{
public:
	VulkanDevice(const VulkanDevice& vkdevice) = delete;
	VulkanDevice(VulkanDevice& vkdevice) = delete;
	VulkanDevice& operator=(const VulkanDevice& vkDevice) = delete;
	VulkanDevice& operator=(VulkanDevice& vkDevice) = delete;

	VulkanDevice(Window& window);
	VulkanDevice(Window& window, bool useIgpu);
	~VulkanDevice();

	SwapChainSupportDetails querySwapChainSupportDetails() const;
	bool checkValidationLayerSupport() const;
	std::vector<const char*> getRequiredExtensions() const;
	QueueFamilyIndices findQueueIndices();
	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling imgTiling, VkFormatFeatureFlags2 formatFeatureFlags, 
		const std::string errorMsg = "failed to find supported format!") const;
	VkFormat findDepthFormat() const;
	

	VkInstance getVkInstance() const;
	VkDevice getDevice() const;
	VkSurfaceKHR getSurface() const;
	float getAspect() const;
	VkQueue getGraphicsQueue() const;
	VkQueue getPresentQueue() const;
	VkCommandPool getCommandPool() const;
	VkPhysicalDeviceProperties getPhysicalDeviceProperties() const;
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR getPhysicalDeviceRTPipelineProperties() const;
	VkPhysicalDeviceAccelerationStructurePropertiesKHR getPhysicalDeviceAccelerationStructureProperties() const;
	void getWindowFrameBufferSize(int* width, int* height) const;
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	[[nodiscard]] VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);
	bool getShader(const std::string& shaderPath, VkShaderModule& shaderOut);
	void addShader(const std::string& shaderPath, VkShaderModule shader);
	VkSampler getDefaultTextureSampler() const;
	VkSampler getShadowMapSampler() const;

private:

	QueueFamilyIndices findQueueIndices(VkPhysicalDevice dev);

	void createVkInstance();
	void setupDebugMessenger();
	void createSurface();
	void pickPhysicalDevice();
	void createDevice();
	void createCommandPool();
	void createSamplers();
	void loadRayTracingFunctions();

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);

	VkInstance m_vkInstance;
	VkDebugUtilsMessengerEXT m_vkDebugMessenger;
	VkPhysicalDevice m_vkPhysicalDevice = VK_NULL_HANDLE;
	VkDevice m_vkDevice;
	VkPhysicalDeviceProperties m_vkPhysicalDeviceProperties;
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR m_vkPhysicalDeviceRTPiplineProperties;
	VkPhysicalDeviceAccelerationStructurePropertiesKHR m_vkPhysicalDeviceAccelerationProperties;
	Window& m_window;
	VkSurfaceKHR m_vkSurfaceKHR;
	VkQueue m_graphicsQueue;
	VkQueue m_presentQueue;
	VkCommandPool m_vkCommandPool;

	std::vector<const char*> m_deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	std::unordered_map<std::string, VkShaderModule> m_shaderCache;
	std::vector<VkSampler> m_vkSamplers;

	//quick switch used to determine wether to use igpu or not
	bool m_useIgpu = false;
};

