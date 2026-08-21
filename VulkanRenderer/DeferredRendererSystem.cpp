#include "DeferredRendererSystem.h"

DeferredRendererSystem::DeferredRendererSystem(VulkanDevice& vulkanDevice, const VulkanRenderer& vulkanRenderer, Camera& camera)
	:m_vulkanDevice(vulkanDevice), m_vulkanRenderer(vulkanRenderer), m_camera(camera)
{
	uint32_t width = m_vulkanRenderer.getSwapChain().getSwapChainWidth();
	uint32_t height = m_vulkanRenderer.getSwapChain().getSwapChainHeight();
	VkSampler gbufferTexturesSampler;

	VkSamplerCreateInfo samplerCreateInfo{};
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.anisotropyEnable = VK_TRUE;
	samplerCreateInfo.maxAnisotropy = m_vulkanDevice.getPhysicalDeviceProperties().limits.maxSamplerAnisotropy;
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	if (vkCreateSampler(m_vulkanDevice.getDevice(), &samplerCreateInfo, nullptr, &gbufferTexturesSampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create vulkan Sampler");
	}

	//8MB depth buffer - vertex position will be derived from depth buffer
	m_gDepth = std::make_unique<VulkanImage>(m_vulkanDevice, width, height, VK_FORMAT_D32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, true, gbufferTexturesSampler);
	//31.6MB normal buffer
	m_gNormal = std::make_unique<VulkanImage>(m_vulkanDevice, width, height, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT, true, gbufferTexturesSampler);
	//8MB albedo and specular buffer, xyz stores diffuse color whereas w stores specualr intensity
	m_gAlbedoSpec = std::make_unique<VulkanImage>(m_vulkanDevice, width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, 
		VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT, true, gbufferTexturesSampler);
	//8MB output buffer for the lighting pass
	m_lightingPassOutput = std::make_unique<VulkanImage>(m_vulkanDevice, width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT, true, gbufferTexturesSampler);

	//creating descriptor pool used for both geometry and lighting pass
	//4 combined image samplers - depth buffer, normal buffer, albedo spec buffer and shadow map
	std::unordered_map<VkDescriptorType, uint32_t> descriptorTypes = { {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 6} };

	//temp
	specularTexture = std::make_unique<Texture>(m_vulkanDevice, "Assets/container2Specular.png", VK_FORMAT_R8_SRGB, VK_IMAGE_TILING_OPTIMAL, 
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	diffuseTexture = std::make_unique<Texture>(m_vulkanDevice, "Assets/container2.jpg", VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

DeferredRendererSystem::~DeferredRendererSystem()
{

}

void DeferredRendererSystem::createPipelines(VulkanDescriptorSetLayout& descriptorSetLayout)
{
	//variables and structs for the geometry pass pipeline
	std::vector<std::pair<VkShaderStageFlagBits, const std::string>> geometryShaderStages = {
		{VK_SHADER_STAGE_VERTEX_BIT, "Shaders/gBufferGeometry.vert.spv"}, {VK_SHADER_STAGE_FRAGMENT_BIT, "Shaders/gBufferGeometry.frag.spv"} };
	VkVertexInputBindingDescription bindingDescription = Vertex::getBindingDescription();
	std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = Vertex::getAttributeDescriptions();
	float blendConstants[4] = { 0.f, 0.f, 0.f, 0.f };
	VkPipelineColorBlendAttachmentState colorBlendAttachmentState{};
	colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachmentState.blendEnable = VK_FALSE; colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
	VkDynamicState dynamicStates[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkFormat colorAttachments[1] = { m_vulkanRenderer.getSwapChain().getSwapChainFormat() };
	m_geometryPassPipeline = std::make_unique<VulkanPipeline>(m_vulkanDevice, geometryShaderStages);
	
	VkPushConstantRange geometryPassPushConstants{};
	geometryPassPushConstants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	geometryPassPushConstants.offset = 0;
	geometryPassPushConstants.size = 64;//one mat4 being the model matrix

	std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;
	VulkanDescriptorSetLayout::addBinding(bindings, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1);
	VulkanDescriptorSetLayout::addBinding(bindings, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1);
	m_geometryPassDescriptorSets.resize(1);
	m_geometryPassSetLayout = std::make_unique<VulkanDescriptorSetLayout>(m_vulkanDevice, bindings);
	VulkanDescriptorWriter descriptorWriter(*m_vulkanDescriptorPool.get(), *m_geometryPassSetLayout.get());
	for (size_t i = 0; i < m_geometryPassDescriptorSets.size(); i++) 
	{
		VkDescriptorImageInfo descriptorImgInfo1{};
		descriptorImgInfo1.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		descriptorImgInfo1.imageView = diffuseTexture->getVkImageView();
		descriptorImgInfo1.sampler = diffuseTexture->getVkSampler();

		VkDescriptorImageInfo descriptorImgInfo2{};
		descriptorImgInfo1.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		descriptorImgInfo1.imageView = specularTexture->getVkImageView();
		descriptorImgInfo1.sampler = specularTexture->getVkSampler();

		descriptorWriter.writeImage(&descriptorImgInfo1, 0).writeImage(&descriptorImgInfo2, 1).build(m_geometryPassDescriptorSets[i]);
	}

	VkDescriptorSetLayout geometrySetLayouts[2] = { descriptorSetLayout.getDescriptorSetLayout(), m_geometryPassSetLayout->getDescriptorSetLayout() };
	m_geometryPassPipeline->createPipelineLayout(2, geometrySetLayouts, 1, &geometryPassPushConstants);

}

void DeferredRendererSystem::setLightLists(std::vector<Light>* m_lights)
{
	m_lightsList = m_lights;
}

VulkanImage& DeferredRendererSystem::getDepthBuffer()
{
	return *m_gDepth.get();
}

VulkanImage& DeferredRendererSystem::getLightingPassOutput()
{
	return *m_lightingPassOutput.get();
}
