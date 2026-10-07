#include "RayTracedRendererSystem.h"

PFN_vkCreateAccelerationStructureKHR RayTracingFunctions::m_vkCreateAccelerationStructureKHR = nullptr;
PFN_vkDestroyAccelerationStructureKHR RayTracingFunctions::m_vkDestroyAccelerationStructureKHR = nullptr;
PFN_vkGetAccelerationStructureBuildSizesKHR RayTracingFunctions::m_vkGetAccelerationStructureBuildSizesKHR = nullptr;
PFN_vkGetAccelerationStructureDeviceAddressKHR RayTracingFunctions::m_vkGetAccelerationStructureDeviceAddressKHR = nullptr;
PFN_vkCmdBuildAccelerationStructuresKHR RayTracingFunctions::m_vkCmdBuildAccelerationStructuresKHR = nullptr;
PFN_vkCreateRayTracingPipelinesKHR RayTracingFunctions::m_vkCreateRayTracingPipelinesKHR = nullptr;
PFN_vkGetRayTracingShaderGroupHandlesKHR RayTracingFunctions::m_vkGetRayTracingShaderGroupHandlesKHR = nullptr;
PFN_vkCmdTraceRaysKHR RayTracingFunctions::m_vkCmdTraceRaysKHR = nullptr;

RayTracedRendererSystem::RayTracedRendererSystem(VulkanDevice& vulkanDevice, VulkanRenderer& vulkanRenderer)
	:m_vulkanDevice(vulkanDevice),
	m_vulkanRenderer(vulkanRenderer),
	m_swapchainFramesInFlight(vulkanRenderer.getSwapChain().MAX_FRAMES_IN_FLIGHT),
	m_vkPhysicalDeviceRTPipelineProperties(vulkanDevice.getPhysicalDeviceRTPipelineProperties()),
	m_vkPhysicalDeviceAccelerationProperties(vulkanDevice.getPhysicalDeviceAccelerationStructureProperties())
{
	loadRayTracingFunctions();
	m_vulkanTLAS.emplace(m_vulkanDevice, m_maxTLASInstances, m_swapchainFramesInFlight);
}

void RayTracedRendererSystem::setModelList(const std::vector<Model>* modelList)
{
	m_modelList = modelList;
}

void RayTracedRendererSystem::setGameObjectList(const std::vector<gameObject>* gameObjectList) 
{
	m_gameObjectList = gameObjectList;
}

void RayTracedRendererSystem::attachCamera(const Camera* camera)
{
	m_camera = camera;
}

void RayTracedRendererSystem::loadRayTracingFunctions()
{
	const VkDevice& device = m_vulkanDevice.getDevice();
	RayTracingFunctions::m_vkCreateAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR)vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR");
	RayTracingFunctions::m_vkDestroyAccelerationStructureKHR = (PFN_vkDestroyAccelerationStructureKHR)vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR");
	RayTracingFunctions::m_vkGetAccelerationStructureBuildSizesKHR = (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetDeviceProcAddr(device, "vkGetAccelerationStructureBuildSizesKHR");
	RayTracingFunctions::m_vkGetAccelerationStructureDeviceAddressKHR = (PFN_vkGetAccelerationStructureDeviceAddressKHR)vkGetDeviceProcAddr(device, "vkGetAccelerationStructureDeviceAddressKHR");
	RayTracingFunctions::m_vkCmdBuildAccelerationStructuresKHR = (PFN_vkCmdBuildAccelerationStructuresKHR)vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresKHR");
	RayTracingFunctions::m_vkCreateRayTracingPipelinesKHR = (PFN_vkCreateRayTracingPipelinesKHR)vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR");
	RayTracingFunctions::m_vkGetRayTracingShaderGroupHandlesKHR = (PFN_vkGetRayTracingShaderGroupHandlesKHR)vkGetDeviceProcAddr(device, "vkGetRayTracingShaderGroupHandlesKHR");
	RayTracingFunctions::m_vkCmdTraceRaysKHR = (PFN_vkCmdTraceRaysKHR)vkGetDeviceProcAddr(device, "vkCmdTraceRaysKHR");

	VulkanBLAS::m_vkCreateAccelerationStructureKHR = RayTracingFunctions::m_vkCreateAccelerationStructureKHR;
	VulkanBLAS::m_vkGetAccelerationStructureBuildSizesKHR = RayTracingFunctions::m_vkGetAccelerationStructureBuildSizesKHR;
	VulkanBLAS::m_vkGetAccelerationStructureDeviceAddressKHR = RayTracingFunctions::m_vkGetAccelerationStructureDeviceAddressKHR;
	VulkanBLAS::m_vkDestroyAccelerationStructureKHR = RayTracingFunctions::m_vkDestroyAccelerationStructureKHR;

	VulkanTLAS::m_vkCreateAccelerationStructureKHR = RayTracingFunctions::m_vkCreateAccelerationStructureKHR;
	VulkanTLAS::m_vkGetAccelerationStructureBuildSizesKHR = RayTracingFunctions::m_vkGetAccelerationStructureBuildSizesKHR;
	VulkanTLAS::m_vkDestroyAccelerationStructureKHR = RayTracingFunctions::m_vkDestroyAccelerationStructureKHR;
}

void RayTracedRendererSystem::buildBottomLevelAccelerationStructures(VkCommandBuffer commandBuffer)
{
	m_vulkanBLASScratchBuildBuffer.reset();
	m_vulkanBLASlist.clear();//for now rebuilding all blas but should add offset or indices for only building a range of blas
	m_vulkanBLASlist.reserve(m_modelList->size());
	m_totalBlasSize = 0;
	uint32_t totalBlasElementCount = 0;//used as the element count variable for the blas buffer, not sure if this is needed but just in case
	VkDeviceSize totalBlasScratchSize = 0;
	VkDeviceSize blasScratchAlignment = static_cast<VkDeviceSize>(m_vkPhysicalDeviceAccelerationProperties.minAccelerationStructureScratchOffsetAlignment);
	//getting blas sizes and setting other blas info
	for (size_t i = 0; i < m_modelList->size(); i++)
	{
		const Model& model = (*m_modelList)[i];

		VulkanBLAS& blas = m_vulkanBLASlist.emplace_back(model);
		blas.m_bufferOffset = m_totalBlasSize;
		blas.m_scratchBufferOffset = totalBlasScratchSize;
		m_totalBlasSize += (blas.m_blasBuildSizesInfo.accelerationStructureSize + 255) & ~255;
		totalBlasScratchSize += (blas.m_blasBuildSizesInfo.buildScratchSize + blasScratchAlignment - 1) & ~(blasScratchAlignment - 1);
		totalBlasElementCount += blas.m_blasBuildRangeInfo.primitiveCount;
		blas.m_modelID = i;
	}
	m_vulkanBLASbuffer.emplace(m_vulkanDevice, totalBlasElementCount, m_totalBlasSize, 
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	m_vulkanBLASScratchBuildBuffer.emplace(m_vulkanDevice, totalBlasElementCount, totalBlasScratchSize, 
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	//creating the acceleration structure handles and populating geometry and build range info list for build command later
	std::vector<VkAccelerationStructureBuildGeometryInfoKHR> blasGeometryBuildInfos; blasGeometryBuildInfos.reserve(m_vulkanBLASlist.size());
	std::vector<VkAccelerationStructureBuildRangeInfoKHR*> blasBuildRangeInfos; blasBuildRangeInfos.reserve(m_vulkanBLASlist.size());
	for (VulkanBLAS& blas : m_vulkanBLASlist)
	{
		blas.createBLAS(*m_vulkanBLASbuffer, *m_vulkanBLASScratchBuildBuffer);
		blasGeometryBuildInfos.push_back(blas.m_blasGeometryInfo);
		blasBuildRangeInfos.push_back(&blas.m_blasBuildRangeInfo);
	}

	//building the acceleration structures, all of them at once
	RayTracingFunctions::m_vkCmdBuildAccelerationStructuresKHR(commandBuffer, static_cast<uint32_t>(blasGeometryBuildInfos.size()), blasGeometryBuildInfos.data(), blasBuildRangeInfos.data());
	//might need to add memory barrier here to all blas built before tlas construction starts
}

void RayTracedRendererSystem::buildTopLevelAccelerationStructure(VkCommandBuffer commandBuffer, size_t swapchainFrameIndex)
{
	//can put this in constructor of class
	//m_vulkanTLAS.emplace(m_vulkanDevice, m_maxTLASInstances, m_swapchainFramesInFlight);
	
	//inserting memory barrier so that tlas only starts its building after the blas have been built
	VkMemoryBarrier2 tlasBuildMemoryBarrier{};
	tlasBuildMemoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
	tlasBuildMemoryBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
	tlasBuildMemoryBarrier.srcAccessMask = VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
	tlasBuildMemoryBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
	tlasBuildMemoryBarrier.dstAccessMask = VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR;

	VkDependencyInfo dependencyInfo{};
	dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	dependencyInfo.pMemoryBarriers = &tlasBuildMemoryBarrier;

	vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);

	//tlas buffers and lists for the current frame
	VulkanBuffer& tlasBuffer = m_vulkanTLAS->m_vulkanTLASBuffers[swapchainFrameIndex];
	VulkanBuffer& tlasInstanceBuffer = m_vulkanTLAS->m_TLASInstanceBuffers[swapchainFrameIndex];
	std::vector<VkAccelerationStructureInstanceKHR>& tlasInstances = m_vulkanTLAS->m_tlasInstances[swapchainFrameIndex];
	//reseting tlas instances list as it will be built all over again
	tlasInstances.clear();
	tlasInstances.reserve(m_gameObjectList->size());
	//gathering and uploading game object data
	for (const gameObject& obj : *m_gameObjectList) 
	{
		VkAccelerationStructureInstanceKHR tlasInstance{};
		VulkanBLAS& blas = m_vulkanBLASlist[obj.getModelID()];//for now blas and model indices match

		glm::mat4 objModelMat = obj.getModelMat();
		tlasInstance.transform = {
			objModelMat[0][0], objModelMat[1][0], objModelMat[2][0], objModelMat[3][0],
			objModelMat[0][1], objModelMat[1][1], objModelMat[2][1], objModelMat[3][1],
			objModelMat[0][2], objModelMat[1][2], objModelMat[2][2], objModelMat[3][2]
		};

		tlasInstance.instanceCustomIndex = 0;//should give game object a type of index
		tlasInstance.mask = 0xFF;
		tlasInstance.instanceShaderBindingTableRecordOffset = 0;
		tlasInstance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
		tlasInstance.accelerationStructureReference = blas.getBLASDeviceAddress();

		tlasInstances.push_back(tlasInstance);
	}
	tlasInstanceBuffer.map(0, sizeof(VkAccelerationStructureInstanceKHR) * tlasInstances.size());
	tlasInstanceBuffer.writeToBuffer(0, sizeof(VkAccelerationStructureInstanceKHR) * tlasInstances.size(), tlasInstances.data());
	tlasInstanceBuffer.unMap();

	//building tlas
	VkAccelerationStructureGeometryKHR tlasGeometry{};
	tlasGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	tlasGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	tlasGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	tlasGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
	tlasGeometry.geometry.instances.data.deviceAddress = tlasInstanceBuffer.getBufferDeviceAddress();
	
	VkAccelerationStructureBuildGeometryInfoKHR tlasGeometryBuildInfo{};
	tlasGeometryBuildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	tlasGeometryBuildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	tlasGeometryBuildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	tlasGeometryBuildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	tlasGeometryBuildInfo.dstAccelerationStructure = m_vulkanTLAS->m_vkTlas[swapchainFrameIndex];
	tlasGeometryBuildInfo.geometryCount = 1;
	tlasGeometryBuildInfo.pGeometries = &tlasGeometry;
	tlasGeometryBuildInfo.scratchData.deviceAddress = m_vulkanTLAS->m_vulkanTLASScratchBuildBuffer->getBufferDeviceAddress();


	VkAccelerationStructureBuildRangeInfoKHR tlasBuildRangeInfo{};
	tlasBuildRangeInfo.primitiveCount = static_cast<uint32_t>(tlasInstances.size());
	tlasBuildRangeInfo.primitiveOffset = 0;
	tlasBuildRangeInfo.firstVertex = 0;
	tlasBuildRangeInfo.transformOffset = 0;

	const VkAccelerationStructureBuildRangeInfoKHR* pTlasBuildRangeInfo = &tlasBuildRangeInfo;

	RayTracingFunctions::m_vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &tlasGeometryBuildInfo, &pTlasBuildRangeInfo);
}