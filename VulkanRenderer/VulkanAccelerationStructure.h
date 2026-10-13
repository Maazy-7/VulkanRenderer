#pragma once

#include "VulkanBuffer.h"
#include "gameObject.h"

class RayTracedRendererSystem;

class VulkanBLAS
{
public:

	VulkanBLAS(const VulkanBLAS&) = delete;
	VulkanBLAS& operator=(const VulkanBLAS&) = delete;

	//move constructor
	VulkanBLAS(VulkanBLAS&&) noexcept = default;//TODO need to implement support for models with multiple meshes

	VulkanBLAS(const Model& model, VkGeometryFlagsKHR geometryFlags = VK_GEOMETRY_OPAQUE_BIT_KHR, 
		VkBuildAccelerationStructureFlagsKHR buildGeometryInfo = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
	~VulkanBLAS();

	void createBLAS(VulkanBuffer& blasBuffer, VulkanBuffer& blasScratchBuildBuffer);//creates the acceleration structure but doesn't build it, thats done separately by batching all BLAS together into 1 build command

	VkDeviceAddress getBLASDeviceAddress() const;

private:
	VkAccelerationStructureKHR m_vkBlas = VK_NULL_HANDLE;
	VkAccelerationStructureGeometryKHR m_blasGeometry{};
	VkAccelerationStructureBuildGeometryInfoKHR m_blasGeometryInfo{};
	VkAccelerationStructureBuildSizesInfoKHR m_blasBuildSizesInfo{};
	VkAccelerationStructureBuildRangeInfoKHR m_blasBuildRangeInfo{};
	VkDeviceSize m_bufferOffset = -1;//offset = previous blas offset + previous blas size
	VkDeviceSize m_scratchBufferOffset = -1;//same formula as m_bufferOffset
	VulkanBuffer* m_blasBuffer = nullptr;//pointer to buffer that will hold all blas
	size_t m_modelID = SIZE_MAX;

	friend RayTracedRendererSystem;
};

class VulkanTLAS 
{
public:
	VulkanTLAS(const VulkanTLAS&) = delete;
	VulkanTLAS& operator=(const VulkanTLAS&) = delete;

	VulkanTLAS(VulkanDevice& vulkanDevice, uint32_t& maxTLASInstances, size_t swapchainFramesInFlight,
		VkBuildAccelerationStructureFlagsKHR buildGeometryInfo = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
	~VulkanTLAS();

private:
	std::vector<std::vector<VkAccelerationStructureInstanceKHR>> m_tlasInstances;
	std::vector<VkAccelerationStructureKHR> m_vkTlas;//hold the top level acceleration structures
	std::vector<VulkanBuffer> m_vulkanTLASBuffers;//having 1 buffer per frame in flight as data will change per frame
	std::vector<VulkanBuffer> m_TLASInstanceBuffers;//having 1 buffer per frame in flight as data will change per frame
	std::optional<VulkanBuffer> m_vulkanTLASScratchBuildBuffer;

	friend RayTracedRendererSystem;
};

