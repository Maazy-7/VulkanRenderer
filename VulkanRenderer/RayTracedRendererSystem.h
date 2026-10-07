#pragma once

#include "VulkanDevice.h"
#include "VulkanPipeline.h"
#include "VulkanRenderer.h"
#include "Camera.h"
#include "VulkanAccelerationStructure.h"

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

class RayTracedRendererSystem
{
public:

	RayTracedRendererSystem(const RayTracedRendererSystem&) = delete;
	RayTracedRendererSystem operator=(const RayTracedRendererSystem&) = delete;

	RayTracedRendererSystem(VulkanDevice& vulkanDevice, VulkanRenderer& vulkanRenderer);

	void setModelList(const std::vector<Model>* modelList);
	void setGameObjectList(const std::vector<gameObject>* gameObjectList);
	void attachCamera(const Camera* camera);

	void buildBottomLevelAccelerationStructures(VkCommandBuffer commandBuffer);
	void buildTopLevelAccelerationStructure(VkCommandBuffer commandBuffer, size_t swapchainFrameIndex);

private:

	void loadRayTracingFunctions();

	VulkanDevice& m_vulkanDevice;
	VulkanRenderer& m_vulkanRenderer;
	std::unique_ptr<VulkanPipeline> m_raytracingPipeline;//TODO implement ray tracing pipeline within VulkanPipeline
	std::unique_ptr<VulkanDescriptorPool> m_raytracingDescriptorPool;
	std::unique_ptr<VulkanDescriptorSetLayout> m_raytracingDescriptorSetLayout;
	VkDescriptorSet m_raytracingDescriptorSet;
	std::optional<VulkanBuffer> m_vulkanBLASbuffer;//all blas stored within this buffer
	std::optional<VulkanBuffer> m_vulkanBLASScratchBuildBuffer;
	VkDeviceSize m_totalBlasSize = 0;//will store combined size of all blas to be used when blas buffer size needs to be increased/decreased
	std::vector<VulkanBLAS> m_vulkanBLASlist;
	size_t m_swapchainFramesInFlight;
	uint32_t m_maxTLASInstances = 2048;//upper bound on the maximum instances to be stored within the TLAS buffer
	std::optional<VulkanTLAS> m_vulkanTLAS;//stores list of instances of existing BLAS
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR m_vkPhysicalDeviceRTPipelineProperties;
	VkPhysicalDeviceAccelerationStructurePropertiesKHR m_vkPhysicalDeviceAccelerationProperties;

	const std::vector<Model>* m_modelList;
	const std::vector<gameObject>* m_gameObjectList;
	const Camera* m_camera;
};

