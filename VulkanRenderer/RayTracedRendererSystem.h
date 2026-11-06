#pragma once

#include "VulkanDevice.h"
#include "VulkanPipeline.h"
#include "VulkanRenderer.h"
#include "Camera.h"
#include "VulkanAccelerationStructure.h"

class RayTracedRendererSystem
{
public:

	RayTracedRendererSystem(const RayTracedRendererSystem&) = delete;
	RayTracedRendererSystem operator=(const RayTracedRendererSystem&) = delete;

	RayTracedRendererSystem(VulkanDevice& vulkanDevice, VulkanRenderer& vulkanRenderer);

	void setModelList(const std::vector<Model>* modelList);
	void setGameObjectList(const std::vector<gameObject>* gameObjectList);
	void attachCamera(const Camera* camera);

	void createPipeline(VulkanDescriptorSetLayout& frameUBOLayout, VulkanDescriptorSetLayout& globalTextureDescriptorLayout, 
		VulkanImage& gDepth, VulkanImage& gNormalMetallic, VulkanImage& gAlbedoRoughness);
	void buildBottomLevelAccelerationStructures(VkCommandBuffer commandBuffer);
	void buildTopLevelAccelerationStructure(VkCommandBuffer commandBuffer, size_t swapchainFrameIndex);
	void executeRayTracingPass(VkCommandBuffer commandBuffer, size_t swapchainFrameIndex, VkDescriptorSet* descriptorSets);
	void recreateRayTracerSystemResources();//only recreates the output image buffer
	VulkanImage& getRayTracerOutputImage(size_t frameIndex);

private:

	void setupShaderBindingTable();

	VulkanDevice& m_vulkanDevice;
	VulkanRenderer& m_vulkanRenderer;
	std::unique_ptr<VulkanPipeline> m_raytracingPipeline;
	std::unique_ptr<VulkanDescriptorPool> m_raytracingDescriptorPool;
	std::unique_ptr<VulkanDescriptorSetLayout> m_raytracingDescriptorSetLayout;
	std::vector<VkDescriptorSet> m_raytracingDescriptorSet;
	std::vector<VulkanImage> m_raytracingOutputImage;
	std::optional<VulkanBuffer> m_vulkanBLASbuffer;//all blas stored within this buffer
	std::optional<VulkanBuffer> m_vulkanBLASScratchBuildBuffer;
	VkDeviceSize m_totalBlasSize = 0;//will store combined size of all blas to be used when blas buffer size needs to be increased/decreased
	std::vector<VulkanBLAS> m_vulkanBLASlist;
	size_t m_swapchainFramesInFlight;
	uint32_t m_maxTLASInstances = 2048;//upper bound on the maximum instances to be stored within the TLAS buffer
	std::optional<VulkanTLAS> m_vulkanTLAS;//stores list of instances of existing BLAS
	std::optional<VulkanBuffer> m_shaderBindingTableBuffer;//buffer for the ray tracer's shader binding table
	std::vector<VkStridedDeviceAddressRegionKHR> m_stridedDeviceAddressRegions;//stores each strided device address per region within the sbt
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR m_vkPhysicalDeviceRTPipelineProperties;
	VkPhysicalDeviceAccelerationStructurePropertiesKHR m_vkPhysicalDeviceAccelerationProperties;

	const std::vector<Model>* m_modelList = nullptr;
	const std::vector<gameObject>* m_gameObjectList = nullptr;
	const Camera* m_camera = nullptr;
};

