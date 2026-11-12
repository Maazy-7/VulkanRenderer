#include "RayTracedRendererSystem.h"

RayTracedRendererSystem::RayTracedRendererSystem(VulkanDevice& vulkanDevice, VulkanRenderer& vulkanRenderer)
	:m_vulkanDevice(vulkanDevice),
	m_vulkanRenderer(vulkanRenderer),
	m_swapchainFramesInFlight(vulkanRenderer.getSwapChain().MAX_FRAMES_IN_FLIGHT),
	m_vkPhysicalDeviceRTPipelineProperties(vulkanDevice.getPhysicalDeviceRTPipelineProperties()),
	m_vkPhysicalDeviceAccelerationProperties(vulkanDevice.getPhysicalDeviceAccelerationStructureProperties())
{
	m_vulkanTLAS.emplace(m_vulkanDevice, m_maxTLASInstances, m_swapchainFramesInFlight);

	//double the descriptor sets than bindings due to 2 descriptors sets for each frame in flight
	std::unordered_map<VkDescriptorType, uint32_t> descriptorTypes = { {VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 2}, 
		{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 4}, {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 16}, {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2} };
	m_raytracingDescriptorPool = std::make_unique<VulkanDescriptorPool>(m_vulkanDevice, descriptorTypes);

	m_raytracingOutputImage.reserve(m_swapchainFramesInFlight);
	m_rasterAndRTBlendOutputImage.reserve(m_swapchainFramesInFlight);
	VkExtent2D swapChainExtent = m_vulkanRenderer.getSwapChain().getSwapChainExtent();
	for (size_t i = 0; i < m_swapchainFramesInFlight; i++) 
	{
		m_raytracingOutputImage.emplace_back(m_vulkanDevice, swapChainExtent.width, swapChainExtent.height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
			/*remove transfer src usage bit*/VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		m_rasterAndRTBlendOutputImage.emplace_back(m_vulkanDevice, swapChainExtent.width, swapChainExtent.height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	}
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

void RayTracedRendererSystem::createPipeline(VulkanDescriptorSetLayout& frameUBOLayout, VulkanDescriptorSetLayout& globalTextureDescriptorLayout,
	VulkanImage& gDepth, VulkanImage& gNormalMetallic, VulkanImage& gAlbedoRoughness, VulkanImage& deferredLightingOutput)
{
	std::vector<std::pair<VkShaderStageFlagBits, const std::string>> rayTracerShaderStages = {
		{ VK_SHADER_STAGE_RAYGEN_BIT_KHR, "Shaders/rayTracer.rgen.spv" },
		{ VK_SHADER_STAGE_MISS_BIT_KHR, "Shaders/rayTracerReflectionMiss.rmiss.spv" },
		{ VK_SHADER_STAGE_MISS_BIT_KHR, "Shaders/rayTracerShadowMiss.rmiss.spv" },
		{ VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, "Shaders/rayTracerReflectionClosestHit.rchit.spv" }
	};
	
	//setting up ray tracer's push constants
	VkPushConstantRange rayGenPushConstant{};
	rayGenPushConstant.offset = 0;
	rayGenPushConstant.size = 80;//64 byte inv proj view matrix, 16 byte background color
	rayGenPushConstant.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR;

	VkPushConstantRange rayTracingPushConstants[1] = { rayGenPushConstant };
	
	//setting up ray tracer's descriptor set layout
	std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;
	VulkanDescriptorSetLayout::addBinding(bindings, 0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 1);
	VulkanDescriptorSetLayout::addBinding(bindings, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 1);
	VulkanDescriptorSetLayout::addBinding(bindings, 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_RAYGEN_BIT_KHR, 1);
	VulkanDescriptorSetLayout::addBinding(bindings, 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_RAYGEN_BIT_KHR, 1);
	VulkanDescriptorSetLayout::addBinding(bindings, 5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_RAYGEN_BIT_KHR, 1);
	VulkanDescriptorSetLayout::addBinding(bindings, 6, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 1);
	
	m_raytracingDescriptorSetLayout = std::make_unique<VulkanDescriptorSetLayout>(m_vulkanDevice, bindings);
	m_raytracingDescriptorSet.resize(m_swapchainFramesInFlight);
	//creating descriptor sets
	for (size_t i = 0; i < m_raytracingDescriptorSet.size(); i++) 
	{
		VulkanDescriptorWriter descriptorWriter(*m_raytracingDescriptorPool, *m_raytracingDescriptorSetLayout);

		//tlas descriptor used to access tlas instances within shader
		VkWriteDescriptorSetAccelerationStructureKHR accelerationStructureInfo{};
		accelerationStructureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
		accelerationStructureInfo.accelerationStructureCount = 1;
		accelerationStructureInfo.pAccelerationStructures = &m_vulkanTLAS->m_vkTlas[i];

		//output image shader will write to
		VkDescriptorImageInfo outputImageInfo{};
		outputImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		outputImageInfo.imageView = m_raytracingOutputImage[i].getVkImageView();

		//gbuffer attachments, depth, normal & roughness, albedo & metallic
		VkDescriptorImageInfo depthImageInfo{};
		depthImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		depthImageInfo.imageView = gDepth.getVkImageView();
		depthImageInfo.sampler = gDepth.getVkSampler();

		VkDescriptorImageInfo normalMetallicImageInfo{};
		normalMetallicImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		normalMetallicImageInfo.imageView = gNormalMetallic.getVkImageView();
		normalMetallicImageInfo.sampler = gNormalMetallic.getVkSampler();

		VkDescriptorImageInfo albedoRoughnessImageInfo{};
		albedoRoughnessImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		albedoRoughnessImageInfo.imageView = gAlbedoRoughness.getVkImageView();
		albedoRoughnessImageInfo.sampler = gAlbedoRoughness.getVkSampler();

		VkDescriptorBufferInfo meshLayoutsShaderStorageBufferInfo{};
		meshLayoutsShaderStorageBufferInfo.buffer = m_vulkanTLAS->m_meshAddressesBuffers[i].getBuffer();
		meshLayoutsShaderStorageBufferInfo.offset = 0;
		meshLayoutsShaderStorageBufferInfo.range = VK_WHOLE_SIZE;//all of mesh address buffer is used for mesh address and data

		descriptorWriter.writeAccelerationStructure(&accelerationStructureInfo, 0).writeImage(&outputImageInfo, 1).
			writeImage(&depthImageInfo, 3).writeImage(&normalMetallicImageInfo, 4).writeImage(&albedoRoughnessImageInfo, 5).
			writeBuffer(&meshLayoutsShaderStorageBufferInfo, 6).build(m_raytracingDescriptorSet[i]);
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

	VkRayTracingShaderGroupCreateInfoKHR rtReflectionMissShaderGroupCreateInfo{};
	rtReflectionMissShaderGroupCreateInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
	rtReflectionMissShaderGroupCreateInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
	rtReflectionMissShaderGroupCreateInfo.generalShader = 1;
	rtReflectionMissShaderGroupCreateInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
	rtReflectionMissShaderGroupCreateInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
	rtReflectionMissShaderGroupCreateInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
	rtShaderGroupCreateInfos.push_back(rtReflectionMissShaderGroupCreateInfo);
	
	VkRayTracingShaderGroupCreateInfoKHR rtShadowMissShaderGroupCreateInfo{};
	rtShadowMissShaderGroupCreateInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
	rtShadowMissShaderGroupCreateInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
	rtShadowMissShaderGroupCreateInfo.generalShader = 2;
	rtShadowMissShaderGroupCreateInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
	rtShadowMissShaderGroupCreateInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
	rtShadowMissShaderGroupCreateInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
	rtShaderGroupCreateInfos.push_back(rtShadowMissShaderGroupCreateInfo);
	
	VkRayTracingShaderGroupCreateInfoKHR rtHitShaderGroupCreateInfo{};
	rtHitShaderGroupCreateInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
	rtHitShaderGroupCreateInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
	rtHitShaderGroupCreateInfo.generalShader = VK_SHADER_UNUSED_KHR;
	rtHitShaderGroupCreateInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
	rtHitShaderGroupCreateInfo.closestHitShader = 3;
	rtHitShaderGroupCreateInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
	rtShaderGroupCreateInfos.push_back(rtHitShaderGroupCreateInfo);

	constexpr uint32_t setLayoutCount = 3;
	VkDescriptorSetLayout raytracingPipelineSetLayouts[setLayoutCount] = { 
		frameUBOLayout.getDescriptorSetLayout(), globalTextureDescriptorLayout.getDescriptorSetLayout(), m_raytracingDescriptorSetLayout->getDescriptorSetLayout()};

	m_raytracingPipeline = std::make_unique<VulkanPipeline>(m_vulkanDevice, rayTracerShaderStages, rtShaderGroupCreateInfos, setLayoutCount, 
		raytracingPipelineSetLayouts, 1, rayTracingPushConstants);

	setupShaderBindingTable();


	//setting up compute pipeline
	std::pair<VkShaderStageFlagBits, const std::string> computeShaderStage { VK_SHADER_STAGE_COMPUTE_BIT, "Shaders/rasterAndRTBlendShader.comp.spv" };

	//push constants for compute pipeline
	VkPushConstantRange computePushConstant{};
	computePushConstant.offset = 0;
	computePushConstant.size = 80;//64 byte inv view proj mat, 16 byte camera view pos
	computePushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	
	//descriptors for compute pipeline, 1 for each frame in flight
	bindings.clear();
	VulkanDescriptorSetLayout::addBinding(bindings, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT, 1);
	VulkanDescriptorSetLayout::addBinding(bindings, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT, 1);
	VulkanDescriptorSetLayout::addBinding(bindings, 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT, 1);
	VulkanDescriptorSetLayout::addBinding(bindings, 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT, 1);
	VulkanDescriptorSetLayout::addBinding(bindings, 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT, 1);
	VulkanDescriptorSetLayout::addBinding(bindings, 5, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 1);

	m_rasterAndRTBlendDescriptorSetLayout = std::make_unique<VulkanDescriptorSetLayout>(m_vulkanDevice, bindings);
	m_rasterAndRTBlendDescriptorSet.resize(m_swapchainFramesInFlight);

	for (size_t i = 0; i < m_rasterAndRTBlendDescriptorSet.size(); i++) 
	{
		VulkanDescriptorWriter descriptorWriter(*m_raytracingDescriptorPool, *m_rasterAndRTBlendDescriptorSetLayout);

		//rasterized lighting pass output from deferred renderer
		VkDescriptorImageInfo deferredLightingOutputImageInfo{};
		deferredLightingOutputImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		deferredLightingOutputImageInfo.imageView = deferredLightingOutput.getVkImageView();
		deferredLightingOutputImageInfo.sampler = deferredLightingOutput.getVkSampler();

		//ray traced output from ray tracer
		VkDescriptorImageInfo rtOutputImageInfo{};
		rtOutputImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		rtOutputImageInfo.imageView = m_raytracingOutputImage[i].getVkImageView();
		rtOutputImageInfo.sampler = m_raytracingOutputImage[i].getVkSampler();

		//gbuffer depth buffer
		VkDescriptorImageInfo depthImageInfo{};
		depthImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		depthImageInfo.imageView = gDepth.getVkImageView();
		depthImageInfo.sampler = gDepth.getVkSampler();

		//gbuffer normal metallic buffer
		VkDescriptorImageInfo normalMetallicImageInfo{};
		normalMetallicImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		normalMetallicImageInfo.imageView = gNormalMetallic.getVkImageView();
		normalMetallicImageInfo.sampler = gNormalMetallic.getVkSampler();

		//gbuffer albedo roughness buffer
		VkDescriptorImageInfo albedoRoughnessImageInfo{};
		albedoRoughnessImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		albedoRoughnessImageInfo.imageView = gAlbedoRoughness.getVkImageView();
		albedoRoughnessImageInfo.sampler = gAlbedoRoughness.getVkSampler();

		//output image shader will write to
		VkDescriptorImageInfo outputImageInfo{};
		outputImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		outputImageInfo.imageView = m_rasterAndRTBlendOutputImage[i].getVkImageView();
		
		descriptorWriter.writeImage(&deferredLightingOutputImageInfo, 0).writeImage(&rtOutputImageInfo, 1).writeImage(&depthImageInfo, 2)
			.writeImage(&normalMetallicImageInfo, 3).writeImage(&albedoRoughnessImageInfo, 4)
			.writeImage(&outputImageInfo, 5).build(m_rasterAndRTBlendDescriptorSet[i]);
	}

	VkDescriptorSetLayout computePipelineSetLayout[] = { m_rasterAndRTBlendDescriptorSetLayout->getDescriptorSetLayout() };
	m_rasterAndRTBlendComputePipeline = std::make_unique<VulkanPipeline>(m_vulkanDevice, computeShaderStage, 1, computePipelineSetLayout, 
		1, &computePushConstant);
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

	uint32_t groupCount = 4;//1 ray gen, 2 miss - 1 shadow miss 1 reflection miss, 1 closest hit - 1 reflection closest hit, shadow hit is omitted
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
	uint32_t closestHitRegionSize = align(sbtEntrySize * 1, baseAlignment);// 1 ray closest hit

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
	m_shaderBindingTableBuffer->unMap();
	
	VkDeviceAddress sbtBufferAddress = m_shaderBindingTableBuffer->getBufferDeviceAddress();
	
	m_stridedDeviceAddressRegions.reserve(4);

	VkStridedDeviceAddressRegionKHR rayGenRegion{};
	rayGenRegion.deviceAddress = sbtBufferAddress;
	rayGenRegion.stride = rayGenRegionSize;
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
	VulkanBuffer& meshAddressBuffer = m_vulkanTLAS->m_meshAddressesBuffers[swapchainFrameIndex];
	//reseting tlas instances list as it will be built all over again
	tlasInstances.clear();
	tlasInstances.reserve(m_gameObjectList->size());
	std::vector<MeshAddress> meshAddresses; meshAddresses.reserve(m_gameObjectList->size());
	//gathering and uploading game object data
	for (size_t i = 0; i < m_gameObjectList->size(); i++) 
	{
		const gameObject& obj = (*m_gameObjectList)[i];
		VkAccelerationStructureInstanceKHR tlasInstance{};
		VulkanBLAS& blas = m_vulkanBLASlist[obj.getModelID()];//for now blas and model indices match

		glm::mat4 objModelMat = obj.getModelMat();
		tlasInstance.transform = {
			objModelMat[0][0], objModelMat[1][0], objModelMat[2][0], objModelMat[3][0],
			objModelMat[0][1], objModelMat[1][1], objModelMat[2][1], objModelMat[3][1],
			objModelMat[0][2], objModelMat[1][2], objModelMat[2][2], objModelMat[3][2]
		};

		tlasInstance.instanceCustomIndex = i;//this index will be used to index into the mesh addresses ssbo to access mesh data (used within the reflection closest hit shader)
		tlasInstance.mask = 0xFF;
		tlasInstance.instanceShaderBindingTableRecordOffset = 0;
		tlasInstance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
		tlasInstance.accelerationStructureReference = blas.getBLASDeviceAddress();

		tlasInstances.push_back(tlasInstance);

		//creating mesh address struct and populating with the gameObject's mesh and texture data
		metallicRoughnessMaterial objMaterial = obj.getMaterial();
		MeshAddress meshAddress{
			blas.m_blasGeometry.geometry.triangles.vertexData.deviceAddress,
			blas.m_blasGeometry.geometry.triangles.indexData.deviceAddress,
			static_cast<uint32_t>(objMaterial.m_diffuseTextureIndex),
			static_cast<uint32_t>(objMaterial.m_NormalMapTextureIndex),
			static_cast<uint32_t>(objMaterial.m_roughnessTextureIndex),
			static_cast<uint32_t>(objMaterial.m_metallicTextureIndex)
		};
		meshAddresses.push_back(meshAddress);
	}
	//loading tlas instance data to gpu buffer
	tlasInstanceBuffer.map(0, sizeof(VkAccelerationStructureInstanceKHR) * tlasInstances.size());
	tlasInstanceBuffer.writeToBuffer(0, sizeof(VkAccelerationStructureInstanceKHR) * tlasInstances.size(), tlasInstances.data());
	tlasInstanceBuffer.unMap();

	//loading mesh address data to gpu buffer
	meshAddressBuffer.map(0, sizeof(MeshAddress) * meshAddresses.size());
	meshAddressBuffer.writeToBuffer(0, sizeof(MeshAddress) * meshAddresses.size(), meshAddresses.data());
	meshAddressBuffer.unMap();

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

void RayTracedRendererSystem::executeRayTracingPass(VkCommandBuffer commandBuffer, size_t swapchainFrameIndex, VkDescriptorSet* descriptorSets)
{
	//transitioning ray tracer's output image layout for writing to
	m_raytracingOutputImage[swapchainFrameIndex].transitionImageLayout(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_2_SHADER_WRITE_BIT, 
		VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR, 1, commandBuffer);
	
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_raytracingPipeline->getPipeline());

	VkDescriptorSet rayTracerDescriptorSets[3] = { descriptorSets[0], descriptorSets[1], m_raytracingDescriptorSet[swapchainFrameIndex] };

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_raytracingPipeline->getPipelineLayout(), 0, 3, 
		rayTracerDescriptorSets, 0, nullptr);

	glm::mat4 invViewProjMat = m_camera->getProjection(); invViewProjMat[1][1] *= -1;
	invViewProjMat *= m_camera->getView();
	invViewProjMat = glm::inverse(invViewProjMat);
	vkCmdPushConstants(commandBuffer, m_raytracingPipeline->getPipelineLayout(), VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR, 0, sizeof(glm::mat4), &invViewProjMat);

	glm::vec4 skyColor = glm::vec4(0.0f, 0.61f, 1.f, 0.f);
	vkCmdPushConstants(commandBuffer, m_raytracingPipeline->getPipelineLayout(), VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR, 64, sizeof(glm::vec4), &skyColor);

	VkExtent2D swapchainExtent = m_vulkanRenderer.getSwapChain().getSwapChainExtent();
	
	RayTracingFunctions::m_vkCmdTraceRaysKHR(commandBuffer, &m_stridedDeviceAddressRegions[0], &m_stridedDeviceAddressRegions[1], 
		&m_stridedDeviceAddressRegions[2], &m_stridedDeviceAddressRegions[3], swapchainExtent.width, swapchainExtent.height, 1);

	m_raytracingOutputImage[swapchainFrameIndex].transitionImageLayout(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 
		VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, 1, commandBuffer);
}

void RayTracedRendererSystem::executeRasterAndRTBlendPass(VkCommandBuffer commandBuffer, size_t swapchainFrameIndex)
{
	//transitioning blend output to be written to
	m_rasterAndRTBlendOutputImage[swapchainFrameIndex].transitionImageLayout(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_2_SHADER_WRITE_BIT, 
		VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, 1, commandBuffer);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_rasterAndRTBlendComputePipeline->getPipeline());
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_rasterAndRTBlendComputePipeline->getPipelineLayout(), 0, 1, 
		&m_rasterAndRTBlendDescriptorSet[swapchainFrameIndex], 0, nullptr);

	glm::mat4 invViewProjMat = m_camera->getProjection(); invViewProjMat[1][1] *= -1;
	invViewProjMat *= m_camera->getView();
	invViewProjMat = glm::inverse(invViewProjMat);
	glm::vec4 viewPos = glm::vec4(m_camera->getPos(), 1.f);
	vkCmdPushConstants(commandBuffer, m_rasterAndRTBlendComputePipeline->getPipelineLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(glm::mat4), &invViewProjMat);
	vkCmdPushConstants(commandBuffer, m_rasterAndRTBlendComputePipeline->getPipelineLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(glm::vec4), &viewPos);

	VkExtent2D swapchainExtent = m_vulkanRenderer.getSwapChain().getSwapChainExtent();
	uint32_t groupCountX = (swapchainExtent.width + 15) / 16;
	uint32_t groupCountY = (swapchainExtent.height + 15) / 16;

	vkCmdDispatch(commandBuffer, groupCountX, groupCountY, 1);

	m_rasterAndRTBlendOutputImage[swapchainFrameIndex].transitionImageLayout(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
		VK_ACCESS_2_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT, 1, commandBuffer);
}

void RayTracedRendererSystem::recreateRayTracerSystemResources()
{
	m_raytracingOutputImage.reserve(m_swapchainFramesInFlight);
	VkExtent2D swapChainExtent = m_vulkanRenderer.getSwapChain().getSwapChainExtent();
	for (size_t i = 0; i < m_raytracingOutputImage.size(); i++)
	{
		m_raytracingOutputImage.emplace_back(m_vulkanDevice, swapChainExtent.width, swapChainExtent.height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		m_rasterAndRTBlendOutputImage.emplace_back(m_vulkanDevice, swapChainExtent.width, swapChainExtent.height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	}
}

VulkanImage& RayTracedRendererSystem::getRayTracerOutputImage(size_t frameIndex)
{
	return m_raytracingOutputImage[frameIndex];
}

VulkanImage& RayTracedRendererSystem::getRasterAndRTBlendOutputImage(size_t frameIndex)
{
	return m_rasterAndRTBlendOutputImage[frameIndex];
}
