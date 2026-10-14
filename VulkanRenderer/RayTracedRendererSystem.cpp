#include "RayTracedRendererSystem.h"

RayTracedRendererSystem::RayTracedRendererSystem(VulkanDevice& vulkanDevice, VulkanRenderer& vulkanRenderer)
	:m_vulkanDevice(vulkanDevice),
	m_vulkanRenderer(vulkanRenderer),
	m_swapchainFramesInFlight(vulkanRenderer.getSwapChain().MAX_FRAMES_IN_FLIGHT),
	m_vkPhysicalDeviceRTPipelineProperties(vulkanDevice.getPhysicalDeviceRTPipelineProperties()),
	m_vkPhysicalDeviceAccelerationProperties(vulkanDevice.getPhysicalDeviceAccelerationStructureProperties())
{
	m_vulkanTLAS.emplace(m_vulkanDevice, m_maxTLASInstances, m_swapchainFramesInFlight);

	std::unordered_map<VkDescriptorType, uint32_t> descriptorTypes = { {VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1}, 
		{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1}, {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1} };
	m_raytracingDescriptorPool = std::make_unique<VulkanDescriptorPool>(m_vulkanDevice, descriptorTypes);

	m_raytracingOutputImage.reserve(m_swapchainFramesInFlight);
	VkExtent2D swapChainExtent = m_vulkanRenderer.getSwapChain().getSwapChainExtent();
	for (size_t i = 0; i < m_raytracingOutputImage.size(); i++) 
	{
		m_raytracingOutputImage.emplace_back(m_vulkanDevice, swapChainExtent.width, swapChainExtent.height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	}

	//setupShaderBindingTable();
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

void RayTracedRendererSystem::createPipeline()
{
	std::vector<std::pair<VkShaderStageFlagBits, const std::string>> rayTracerShaderStages = {};//TODO write ray tracing shaders
	
	//setting up ray tracer's push constants
	VkPushConstantRange pushConstant{};
	
	//setting up ray tracer's descriptor set layout
	std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;
	VulkanDescriptorSetLayout::addBinding(bindings, 0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 1);
	VulkanDescriptorSetLayout::addBinding(bindings, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_KHR, 1);
	//VulkanDescriptorSetLayout::addBinding(bindings, 2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR, 1);
	m_raytracingDescriptorSetLayout = std::make_unique<VulkanDescriptorSetLayout>(m_vulkanDevice, bindings);
	m_raytracingDescriptorSet.resize(m_swapchainFramesInFlight);
	//creating descriptor sets
	for (size_t i = 0; i < m_raytracingDescriptorSet.size(); i++) 
	{
		VulkanDescriptorWriter descriptorWriter(*m_raytracingDescriptorPool, *m_raytracingDescriptorSetLayout);

		VkDescriptorImageInfo outputImageInfo{};
		outputImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		outputImageInfo.imageView = m_raytracingOutputImage[i].getVkImageView();

		VkWriteDescriptorSetAccelerationStructureKHR accelerationStructureInfo{};
		accelerationStructureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
		accelerationStructureInfo.accelerationStructureCount = 1;
		accelerationStructureInfo.pAccelerationStructures = &m_vulkanTLAS->m_vkTlas[i];

		//for camera info might just use push constants if possible

		descriptorWriter.writeImage(&outputImageInfo, 0).writeAccelerationStructure(&accelerationStructureInfo, 1).build(m_raytracingDescriptorSet[i]);
	}

	std::vector<VkRayTracingShaderGroupCreateInfoKHR> rtShaderGroupCreateInfos;
	VkRayTracingShaderGroupCreateInfoKHR rtGenShaderGroupCreateInfo{};
	rtGenShaderGroupCreateInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
	rtGenShaderGroupCreateInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
	rtGenShaderGroupCreateInfo.generalShader = 0;
	rtGenShaderGroupCreateInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
	rtGenShaderGroupCreateInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
	rtGenShaderGroupCreateInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
	rtShaderGroupCreateInfos.push_back(rtGenShaderGroupCreateInfo);

	VkRayTracingShaderGroupCreateInfoKHR rtMissShaderGroupCreateInfo{};
	rtMissShaderGroupCreateInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
	rtMissShaderGroupCreateInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
	rtMissShaderGroupCreateInfo.generalShader = 1;
	rtMissShaderGroupCreateInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
	rtMissShaderGroupCreateInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
	rtMissShaderGroupCreateInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
	rtShaderGroupCreateInfos.push_back(rtMissShaderGroupCreateInfo);

	VkRayTracingShaderGroupCreateInfoKHR rtHitShaderGroupCreateInfo{};
	rtHitShaderGroupCreateInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
	rtHitShaderGroupCreateInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
	rtHitShaderGroupCreateInfo.generalShader = VK_SHADER_UNUSED_KHR;
	rtHitShaderGroupCreateInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
	rtHitShaderGroupCreateInfo.closestHitShader = 2;
	rtHitShaderGroupCreateInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
	rtShaderGroupCreateInfos.push_back(rtHitShaderGroupCreateInfo);

	VkDescriptorSetLayout raytracingPipelineSetLayouts[] = { m_raytracingDescriptorSetLayout->getDescriptorSetLayout() };

	m_raytracingPipeline = std::make_unique<VulkanPipeline>(m_vulkanDevice, rayTracerShaderStages, rtShaderGroupCreateInfos, 1, 
		raytracingPipelineSetLayouts, 1, &pushConstant);
}

void RayTracedRendererSystem::setupShaderBindingTable()
{
	//querying alignments and sizes
	uint32_t handleSize = m_vkPhysicalDeviceRTPipelineProperties.shaderGroupHandleSize;
	uint32_t handleAlignment = m_vkPhysicalDeviceRTPipelineProperties.shaderGroupHandleAlignment;
	uint32_t baseAlignment = m_vkPhysicalDeviceRTPipelineProperties.shaderGroupBaseAlignment;

	//getting sizes for the sbt handles and alignments
	auto align = [](uint32_t value, uint32_t alignment) {
		return (value + alignment - 1) & ~(alignment - 1);
		};

	uint32_t sbtEntrySize = align(handleSize, handleAlignment);

	uint32_t groupCount = 5;//1 ray gen, 2 miss - 1 shadow miss 1 reflection miss, 2 closest hit - 1 shadow closest hit 1 reflection closest hit
	uint32_t dataSize = groupCount * handleSize;
	std::vector<uint8_t> shaderHandleStorage(dataSize);
	if (RayTracingFunctions::m_vkGetRayTracingShaderGroupHandlesKHR(m_vulkanDevice.getDevice(), m_raytracingPipeline->getPipeline(),
		0, groupCount, dataSize, shaderHandleStorage.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to get Vulkan ray tracing shader group handles");
	}

	//setting sizes for each region within the sbt
	uint32_t rayGenRegionSize = align(sbtEntrySize * 1, baseAlignment);//1 ray gen 
	uint32_t missRegionSize = align(sbtEntrySize * 2, baseAlignment);//2 ray miss
	uint32_t closestHitRegionSize = align(sbtEntrySize * 2, baseAlignment);// 2 ray closest hit

	//creating the sbt buffer
	uint32_t sbtBufferSize = rayGenRegionSize + missRegionSize + closestHitRegionSize;
	m_shaderBindingTableBuffer.emplace(m_vulkanDevice, 3, sbtBufferSize, VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	m_shaderBindingTableBuffer->map(0, sbtBufferSize);
	uint8_t* pHandles = shaderHandleStorage.data();
	VkDeviceSize currentOffset = 0;//aligned shader group region sizes are added to the offset
	m_shaderBindingTableBuffer->writeToBuffer(currentOffset, handleSize, pHandles + (0 * handleSize));//grabbing data size of handle size and from pHandles with group number as offset
	currentOffset += rayGenRegionSize;
	//since there are 2 groups for miss and hit each is offset by the size of the sbt entry
	m_shaderBindingTableBuffer->writeToBuffer(currentOffset + (0 * sbtEntrySize), handleSize, pHandles + (1 * handleSize));
	m_shaderBindingTableBuffer->writeToBuffer(currentOffset + (1 * sbtEntrySize), handleSize, pHandles + (2 * handleSize));
	currentOffset += missRegionSize;
	m_shaderBindingTableBuffer->writeToBuffer(currentOffset + (0 * sbtEntrySize), handleSize, pHandles + (3 * handleSize));
	m_shaderBindingTableBuffer->writeToBuffer(currentOffset + (1 * sbtEntrySize), handleSize, pHandles + (4 * handleSize));
	m_shaderBindingTableBuffer->unMap();
	
	VkDeviceAddress sbtBufferAddress = m_shaderBindingTableBuffer->getBufferDeviceAddress();
	
	m_stridedDeviceAddressRegions.reserve(4);

	VkStridedDeviceAddressRegionKHR rayGenRegion{};
	rayGenRegion.deviceAddress = sbtBufferAddress;
	rayGenRegion.stride = sbtEntrySize;
	rayGenRegion.size = rayGenRegionSize;
	m_stridedDeviceAddressRegions.push_back(rayGenRegion);

	VkStridedDeviceAddressRegionKHR missRegion{};
	missRegion.deviceAddress = sbtBufferAddress + rayGenRegionSize;
	missRegion.stride = sbtEntrySize;
	missRegion.size = missRegionSize;
	m_stridedDeviceAddressRegions.push_back(missRegion);

	VkStridedDeviceAddressRegionKHR closestHitRegion{};
	closestHitRegion.deviceAddress = sbtBufferAddress + rayGenRegionSize + missRegionSize;
	closestHitRegion.stride = sbtEntrySize;
	closestHitRegion.size = closestHitRegionSize;
	m_stridedDeviceAddressRegions.push_back(closestHitRegion);

	VkStridedDeviceAddressRegionKHR callableRegion{};//currently unused
	m_stridedDeviceAddressRegions.push_back(callableRegion);

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

void RayTracedRendererSystem::executeRayTracingPass(VkCommandBuffer commandBuffer, size_t swapchainFrameIndex)
{
	//transitioning ray tracer's output image layout for writing to
	m_raytracingOutputImage[swapchainFrameIndex].transitionImageLayout(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_2_SHADER_WRITE_BIT, 
		VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR, 1, commandBuffer);
	
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_raytracingPipeline->getPipeline());

	//TODO properly implement descriptor sets ie gbuffer info from deferred renderer, global texture descriptor set
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_raytracingPipeline->getPipelineLayout(), 0, 1, 
		&m_raytracingDescriptorSet[swapchainFrameIndex], 0, nullptr);

	VkExtent2D swapchainExtent = m_vulkanRenderer.getSwapChain().getSwapChainExtent();
	
	//TODO proprerly implement a simple pbr system with mettalic and roughness material proeprties to allow for better lighting with ray tracing
	RayTracingFunctions::m_vkCmdTraceRaysKHR(commandBuffer, &m_stridedDeviceAddressRegions[0], &m_stridedDeviceAddressRegions[1], 
		&m_stridedDeviceAddressRegions[2], &m_stridedDeviceAddressRegions[3], swapchainExtent.width, swapchainExtent.height, 1);
}
