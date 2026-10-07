#include "VulkanAccelerationStructure.h"

PFN_vkCreateAccelerationStructureKHR VulkanBLAS::m_vkCreateAccelerationStructureKHR = nullptr;
PFN_vkGetAccelerationStructureBuildSizesKHR VulkanBLAS::m_vkGetAccelerationStructureBuildSizesKHR = nullptr;
PFN_vkGetAccelerationStructureDeviceAddressKHR VulkanBLAS::m_vkGetAccelerationStructureDeviceAddressKHR = nullptr;
PFN_vkDestroyAccelerationStructureKHR VulkanBLAS::m_vkDestroyAccelerationStructureKHR = nullptr;

PFN_vkCreateAccelerationStructureKHR VulkanTLAS::m_vkCreateAccelerationStructureKHR = nullptr;
PFN_vkGetAccelerationStructureBuildSizesKHR VulkanTLAS::m_vkGetAccelerationStructureBuildSizesKHR = nullptr;
PFN_vkDestroyAccelerationStructureKHR VulkanTLAS::m_vkDestroyAccelerationStructureKHR = nullptr;

VulkanBLAS::VulkanBLAS(const Model& model, VkGeometryFlagsKHR geometryFlags, VkBuildAccelerationStructureFlagsKHR buildGeometryInfo)
{
	VulkanDevice& device = model.getMesh().getMeshVertexBuffer().getVulkanDevice();

	m_blasGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	m_blasGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
	m_blasGeometry.flags = geometryFlags;

	m_blasGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
	m_blasGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
	m_blasGeometry.geometry.triangles.vertexData.deviceAddress = model.getMesh().getMeshVertexBuffer().getBufferDeviceAddress();
	m_blasGeometry.geometry.triangles.vertexStride = sizeof(Vertex);
	m_blasGeometry.geometry.triangles.maxVertex = model.getMesh().getMeshVertexBuffer().getBufferElementCount() - 1;
	m_blasGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
	m_blasGeometry.geometry.triangles.indexData.deviceAddress = model.getMesh().getMeshIndexBuffer().getBufferDeviceAddress();

	m_blasGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	m_blasGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	m_blasGeometryInfo.flags = buildGeometryInfo;
	m_blasGeometryInfo.geometryCount = 1;//ideally the mesh count but for now we only have one
	m_blasGeometryInfo.pGeometries = &m_blasGeometry;

	uint32_t maxPrimitiveCount = model.getMesh().getMeshIndexBuffer().getBufferElementCount() / 3;//<- assuming triangle list is used, if strips then this becomes indices - 2

	m_blasBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	m_vkGetAccelerationStructureBuildSizesKHR(device.getDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &m_blasGeometryInfo,
		&maxPrimitiveCount, &m_blasBuildSizesInfo);

	m_blasBuildRangeInfo.primitiveCount = maxPrimitiveCount;
	m_blasBuildRangeInfo.primitiveOffset = 0;
	m_blasBuildRangeInfo.firstVertex = 0;
	m_blasBuildRangeInfo.transformOffset = 0;
}

VulkanBLAS::~VulkanBLAS()
{
	m_vkDestroyAccelerationStructureKHR(m_blasBuffer->getVulkanDevice().getDevice(), m_vkBlas, nullptr);
}

void VulkanBLAS::createBLAS(VulkanBuffer& blasBuffer, VulkanBuffer& blasScratchBuildBuffer)
{
	m_blasBuffer = &blasBuffer;
	
	VkAccelerationStructureCreateInfoKHR accelStructCreateInfo{};
	accelStructCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	accelStructCreateInfo.buffer = m_blasBuffer->getBuffer();
	accelStructCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	accelStructCreateInfo.size = m_blasBuildSizesInfo.accelerationStructureSize;
	accelStructCreateInfo.offset = m_bufferOffset;

	m_vkCreateAccelerationStructureKHR(m_blasBuffer->getVulkanDevice().getDevice(), &accelStructCreateInfo, nullptr, &m_vkBlas);

	m_blasGeometryInfo.pGeometries = &m_blasGeometry;//reseting pointer as blas stored in vector will get moved
	m_blasGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	m_blasGeometryInfo.dstAccelerationStructure = m_vkBlas;
	m_blasGeometryInfo.scratchData.deviceAddress = blasScratchBuildBuffer.getBufferDeviceAddress() + m_scratchBufferOffset;
}

VkDeviceAddress VulkanBLAS::getBLASDeviceAddress() const
{
	VkAccelerationStructureDeviceAddressInfoKHR blasDeviceAddressInfo{};
	blasDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
	blasDeviceAddressInfo.accelerationStructure = m_vkBlas;
	return m_vkGetAccelerationStructureDeviceAddressKHR(m_blasBuffer->getVulkanDevice().getDevice(), &blasDeviceAddressInfo);
}

VulkanTLAS::VulkanTLAS(VulkanDevice& vulkanDevice, uint32_t& maxTLASInstances, size_t swapchainFramesInFlight,
	VkBuildAccelerationStructureFlagsKHR buildGeometryInfo)
{
	VkAccelerationStructureGeometryKHR tlasGeometry{};
	tlasGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	tlasGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	tlasGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	tlasGeometry.geometry.instances.arrayOfPointers = VK_FALSE;

	VkAccelerationStructureBuildGeometryInfoKHR tlasGeometryBuildInfo{};
	tlasGeometryBuildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	tlasGeometryBuildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	tlasGeometryBuildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	tlasGeometryBuildInfo.geometryCount = 1;
	tlasGeometryBuildInfo.pGeometries = &tlasGeometry;

	VkAccelerationStructureBuildSizesInfoKHR tlasBuildSizesInfo{};
	tlasBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	m_vkGetAccelerationStructureBuildSizesKHR(vulkanDevice.getDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &tlasGeometryBuildInfo, 
		&maxTLASInstances, &tlasBuildSizesInfo);

	m_vkTlas.resize(swapchainFramesInFlight);
	m_vulkanTLASBuffers.reserve(swapchainFramesInFlight);
	m_TLASInstanceBuffers.reserve(swapchainFramesInFlight);
	m_tlasInstances.resize(swapchainFramesInFlight);
	
	//only 1 scratch buffer required as it'll only be used during tlas build and will be free to use the next frame
	m_vulkanTLASScratchBuildBuffer.emplace(vulkanDevice, static_cast<VkDeviceSize>(maxTLASInstances), tlasBuildSizesInfo.buildScratchSize,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	for (size_t i = 0; i < swapchainFramesInFlight; i++) 
	{
		m_tlasInstances[i].reserve(maxTLASInstances);

		//stores the TLAS handle(s) 
		m_vulkanTLASBuffers.emplace_back(vulkanDevice, static_cast<VkDeviceSize>(maxTLASInstances), tlasBuildSizesInfo.accelerationStructureSize, 
			VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		
		//stores the TLAS instances
		m_TLASInstanceBuffers.emplace_back(vulkanDevice, static_cast<VkDeviceSize>(maxTLASInstances), maxTLASInstances * sizeof(VkAccelerationStructureInstanceKHR),
			VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, 
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		//creating the tlas handles
		VkAccelerationStructureCreateInfoKHR tlasCreateInfo{};
		tlasCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		tlasCreateInfo.buffer = m_vulkanTLASBuffers[i].getBuffer();
		tlasCreateInfo.size = tlasBuildSizesInfo.accelerationStructureSize;
		tlasCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

		m_vkCreateAccelerationStructureKHR(vulkanDevice.getDevice(), &tlasCreateInfo, nullptr, &m_vkTlas[i]);
	}
}

VulkanTLAS::~VulkanTLAS()
{
	for (VkAccelerationStructureKHR& tlas : m_vkTlas) 
	{
		m_vkDestroyAccelerationStructureKHR(m_vulkanTLASScratchBuildBuffer->getVulkanDevice().getDevice(), tlas, nullptr);
	}
}
