#include "ShadowCaster.h"

ShadowCaster::ShadowCaster(VulkanDevice& vulkanDevice, Light& light, uint32_t shadowWidth, uint32_t shadowHeight, size_t shadowCasterIndex, 
	float nearPlane, float farPlane)
	:m_vulkanDevice(vulkanDevice), m_shadowWidth(shadowWidth), m_shadowHeight(shadowHeight), m_shadowCasterIndex(shadowCasterIndex), 
	m_lightIndex(light.getLightIndex()), m_nearPlane(nearPlane), m_farPlane(farPlane)
{
	//sampler for shadow
	VkSamplerCreateInfo shadowSamplerCreateInfo{};
	shadowSamplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	shadowSamplerCreateInfo.minFilter = VK_FILTER_LINEAR;
	shadowSamplerCreateInfo.magFilter = VK_FILTER_LINEAR;
	shadowSamplerCreateInfo.anisotropyEnable = VK_FALSE;
	shadowSamplerCreateInfo.maxAnisotropy = 1.f;
	shadowSamplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	shadowSamplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	shadowSamplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	shadowSamplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	shadowSamplerCreateInfo.compareEnable = VK_TRUE;
	shadowSamplerCreateInfo.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

	VkSampler sampler;
	if (vkCreateSampler(m_vulkanDevice.getDevice(), &shadowSamplerCreateInfo, nullptr, &sampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create vulkan Sampler");
	}

	if (light.getLightType() == LightType::Directional) //directional light -> 2d shadow map
	{
		m_shadowMap.emplace(vulkanDevice, shadowWidth, shadowHeight, VK_FORMAT_D32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
	VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, true, sampler);
		//1 shadow map 1 light space matrix
		glm::mat4 lightSpaceMat = glm::ortho(-15.f, 15.f, -15.f, 15.f, 0.1f, 25.f);
		lightSpaceMat[1][1] *= -1;
		lightSpaceMat *= glm::lookAt(glm::vec3(light.getPos()), glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f));
		m_lightSpaceMatrix.reserve(1);
		m_lightSpaceMatrix.emplace_back(lightSpaceMat);
	}
	else if (light.getLightType() == LightType::Point)//point light -> cubemap shadows
	{
		VkImageCreateInfo imageCreateInfo{};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.extent.width = shadowHeight;
		imageCreateInfo.extent.height = shadowHeight;
		imageCreateInfo.extent.depth = 1;
		imageCreateInfo.mipLevels = 1;
		imageCreateInfo.arrayLayers = 6;
		imageCreateInfo.format = VK_FORMAT_D32_SFLOAT;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		VkImage vkImage;
		if (vkCreateImage(m_vulkanDevice.getDevice(), &imageCreateInfo, nullptr, &vkImage) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create vulkan image");
		}

		VkImageViewCreateInfo imageViewCreateInfo{};
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.image = vkImage;
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
		imageViewCreateInfo.format = VK_FORMAT_D32_SFLOAT;
		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 6;

		m_shadowMap.emplace(vulkanDevice, vkImage, &imageViewCreateInfo, VK_FORMAT_D32_SFLOAT, sampler);

		//creating image view for rendering to cubemap
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;//second view type used for rendering to cubemap
		if (vkCreateImageView(m_vulkanDevice.getDevice(), &imageViewCreateInfo, nullptr, &m_cubemapLayerRenderImageView) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create vulkan image view");
		}

		//6 faces of cube 6 light space matrices
		glm::mat4 proj = glm::perspective(glm::radians(90.f), static_cast<float>(shadowWidth)/shadowHeight, m_nearPlane, m_farPlane);
		//proj[1][1] *= -1;
		m_lightSpaceMatrix.reserve(6);
		m_lightSpaceMatrix.emplace_back(proj * glm::lookAt(light.getPos(), light.getPos() + glm::vec3(1.f, 0.f, 0.f), glm::vec3(0.f, -1.f, 0.f)));//+X
		m_lightSpaceMatrix.emplace_back(proj * glm::lookAt(light.getPos(), light.getPos() + glm::vec3(-1.f, 0.f, 0.f), glm::vec3(0.f, -1.f, 0.f)));//-X
		m_lightSpaceMatrix.emplace_back(proj * glm::lookAt(light.getPos(), light.getPos() + glm::vec3(0.f, 1.f, 0.f), glm::vec3(0.f, 0.f, 1.f)));//+Y
		m_lightSpaceMatrix.emplace_back(proj * glm::lookAt(light.getPos(), light.getPos() + glm::vec3(0.f, -1.f, 0.f), glm::vec3(0.f, 0.f, -1.f)));//-Y
		m_lightSpaceMatrix.emplace_back(proj * glm::lookAt(light.getPos(), light.getPos() + glm::vec3(0.f, 0.f, 1.f), glm::vec3(0.f, -1.f, 0.f)));//+Z
		m_lightSpaceMatrix.emplace_back(proj * glm::lookAt(light.getPos(), light.getPos() + glm::vec3(0.f, 0.f, -1.f), glm::vec3(0.f, -1.f, 0.f)));//-Z
	}
}

ShadowCaster::~ShadowCaster()
{
	vkDestroyImageView(m_vulkanDevice.getDevice(), m_cubemapLayerRenderImageView, nullptr);
}

uint32_t ShadowCaster::getShadowWidth() const
{
	return m_shadowWidth;
}

uint32_t ShadowCaster::getShadowHeight() const
{
	return m_shadowHeight;
}

const VulkanImage& ShadowCaster::getShadowMap() const
{
	return *m_shadowMap;
}

void ShadowCaster::updateLightSpaceMatrix(Light& light) //called before getting light space matrix to update matrix to match light's position
{
	if (light.getLightType() == LightType::Directional) 
	{
		glm::mat4 lightSpaceMat = glm::ortho(-15.f, 15.f, -15.f, 15.f, 0.1f, 25.f);
		lightSpaceMat[1][1] *= -1;
		lightSpaceMat *= glm::lookAt(glm::vec3(light.getPos()), glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f));
		m_lightSpaceMatrix[0] = lightSpaceMat;
	}
	else if (light.getLightType() == LightType::Point) 
	{
		glm::mat4 proj = glm::perspective(glm::radians(90.f), static_cast<float>(m_shadowWidth) / m_shadowHeight, m_nearPlane, m_farPlane);
		//proj[1][1] *= -1;
		m_lightSpaceMatrix[0] = proj * glm::lookAt(light.getPos(), light.getPos() + glm::vec3(1.f, 0.f, 0.f), glm::vec3(0.f, -1.f, 0.f));//+X
		m_lightSpaceMatrix[1] = proj * glm::lookAt(light.getPos(), light.getPos() + glm::vec3(-1.f, 0.f, 0.f), glm::vec3(0.f, -1.f, 0.f));//-X
		m_lightSpaceMatrix[2] = proj * glm::lookAt(light.getPos(), light.getPos() + glm::vec3(0.f, 1.f, 0.f), glm::vec3(0.f, 0.f, 1.f));//+Y
		m_lightSpaceMatrix[3] = proj * glm::lookAt(light.getPos(), light.getPos() + glm::vec3(0.f, -1.f, 0.f), glm::vec3(0.f, 0.f, -1.f));//-Y
		m_lightSpaceMatrix[4] = proj * glm::lookAt(light.getPos(), light.getPos() + glm::vec3(0.f, 0.f, 1.f), glm::vec3(0.f, -1.f, 0.f));//+Z
		m_lightSpaceMatrix[5] = proj * glm::lookAt(light.getPos(), light.getPos() + glm::vec3(0.f, 0.f, -1.f), glm::vec3(0.f, -1.f, 0.f));//-Z
	}
}
