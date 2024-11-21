#include "PointShadowDescriptorsManager.h"
#include "SynchronizationModule.h"
#include <TextureManager.h>

PointShadowDescriptorsManager::PointShadowDescriptorsManager()
{
    this->deviceModule = DeviceModule::getInstance();

    this->CreateOffscreenDescriptorPool();
    this->CreateRenderDescriptorPool();
}

void PointShadowDescriptorsManager::AddPointLightResources(std::shared_ptr<UniformBufferObject> shadowMapUBO, VkImageView imageView, VkSampler sampler)
{
    if (this->_numPointLights < MAX_NUM_POINT_LIGHTS)
    {
        this->shadowMapUBOs.push_back(shadowMapUBO);
        this->_imageViews.push_back(imageView);
        this->_samplers.push_back(sampler);
        this->_numPointLights++;
    }
}

void PointShadowDescriptorsManager::InitializeDescriptorSetLayouts(std::shared_ptr<ShaderModule> offscreen_shader_ptr)
{
    this->CreateCubemapPlaceHolder();

    this->offscreenDescriptorSetLayout = offscreen_shader_ptr->descriptorSetLayouts.at(0);
    this->renderDescriptorSetLayout = this->CreateRenderDescriptorSetLayout();

    this->CreateOffscreenDescriptorSet();
    this->CreateRenderDescriptorSet();
}

void PointShadowDescriptorsManager::CreateOffscreenDescriptorPool()
{
    // Point Camera Uniform
    VkDescriptorPoolSize offscreenPoolSize;
    offscreenPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    offscreenPoolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * MAX_NUM_POINT_LIGHTS;

    VkDescriptorPoolCreateInfo offscreenPoolInfo{};
    offscreenPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    offscreenPoolInfo.poolSizeCount = 1;
    offscreenPoolInfo.pPoolSizes = &offscreenPoolSize;
    offscreenPoolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * MAX_NUM_POINT_LIGHTS;

    if (vkCreateDescriptorPool(this->deviceModule->device, &offscreenPoolInfo, nullptr, &this->offscreenDescriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create offscreen descriptor pool!");
    }
}

void PointShadowDescriptorsManager::CreateRenderDescriptorPool()
{
    std::vector<VkDescriptorPoolSize> renderPoolSizes;
    renderPoolSizes.resize(1);

    // Point Light Uniform
    //renderPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    //renderPoolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * MAX_NUM_POINT_LIGHTS;

    // Shadow Texture Samplers
    renderPoolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    renderPoolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * MAX_NUM_POINT_LIGHTS;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(renderPoolSizes.size());
    poolInfo.pPoolSizes = renderPoolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * MAX_NUM_POINT_LIGHTS;

    if (vkCreateDescriptorPool(this->deviceModule->device, &poolInfo, nullptr, &this->renderDescriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create render point shadow descriptor pool!");
    }
}

VkDescriptorBufferInfo PointShadowDescriptorsManager::GetBufferInfo(VkBuffer buffer, VkDeviceSize bufferSize)
{
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = bufferSize;
    return bufferInfo;
}

void PointShadowDescriptorsManager::CreateCubemapPlaceHolder()
{
    TextureManager* textureManager = TextureManager::getInstance();
    auto emptyTexture = textureManager->GetTexture("NULL_TEXTURE");

    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    vkCreateSampler(this->deviceModule->device, &samplerInfo, NULL, &placeholderSampler);

    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = emptyTexture->image; // Una textura 1x1 creada previamente
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 6;
    vkCreateImageView(this->deviceModule->device, &viewInfo, NULL, &placeholderImageView);
}

void PointShadowDescriptorsManager::SetOffscreenDescriptorWrite(VkWriteDescriptorSet& descriptorWrite, VkDescriptorSet descriptorSet, VkDescriptorType descriptorType, uint32_t binding, VkBuffer buffer, VkDeviceSize bufferSize)
{
    this->offscreenBuffersInfo[binding] = GetBufferInfo(buffer, bufferSize);

    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = binding;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = descriptorType;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = VK_NULL_HANDLE;
    descriptorWrite.pBufferInfo = &this->offscreenBuffersInfo[binding];
}

void PointShadowDescriptorsManager::SetRenderDescriptorWrite(VkWriteDescriptorSet& descriptorWrite, VkDescriptorSet descriptorSet, VkDescriptorType descriptorType, uint32_t binding, VkBuffer buffer, VkDeviceSize bufferSize)
{
    this->renderBuffersInfo[binding] = GetBufferInfo(buffer, bufferSize);

    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = binding;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = descriptorType;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = VK_NULL_HANDLE;
    descriptorWrite.pBufferInfo = &this->renderBuffersInfo[binding];
}

void PointShadowDescriptorsManager::SetCubeMapDescriptorWrite(VkWriteDescriptorSet& descriptorWrite, VkDescriptorSet descriptorSet, VkDescriptorType descriptorType, uint32_t binding)
{
    for (uint32_t i = 0; i < MAX_NUM_POINT_LIGHTS; ++i)
    {
        this->shadowPointsImageInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        if (i < _numPointLights)
        {
            this->shadowPointsImageInfo[i].imageView = _imageViews[i]; // ImageView de cada cubemap
            this->shadowPointsImageInfo[i].sampler = _samplers[i]; // Sampler para cada cubemap
        }
        else
        {
            this->shadowPointsImageInfo[i].imageView = placeholderImageView; // ImageView placeholder para cada cubemap
            this->shadowPointsImageInfo[i].sampler = placeholderSampler; // Sampler placeholder para cada cubemap
        }
    }

    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = binding;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = descriptorType;
    descriptorWrite.descriptorCount = MAX_NUM_POINT_LIGHTS; // Número de cubemaps en el arreglo
    descriptorWrite.pImageInfo = this->shadowPointsImageInfo.data();
}

void PointShadowDescriptorsManager::CreateRenderDescriptorSet()
{
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, renderDescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = this->renderDescriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    this->shadowPointsImageInfo.resize(MAX_NUM_POINT_LIGHTS);

    if (vkAllocateDescriptorSets(deviceModule->device, &allocInfo, this->renderDescriptorSets) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        std::vector<VkWriteDescriptorSet> descriptorWrites{};
        descriptorWrites.resize(1);

        this->SetCubeMapDescriptorWrite(descriptorWrites[0], this->renderDescriptorSets[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0);

        vkUpdateDescriptorSets(deviceModule->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}

VkDescriptorSetLayout PointShadowDescriptorsManager::CreateRenderDescriptorSetLayout()
{
    VkDescriptorSetLayout resultLayout = {};

    const uint32_t NUM_BINDINGS = 1;

    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.descriptorCount = MAX_NUM_POINT_LIGHTS;
    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    binding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = NUM_BINDINGS;
    layoutInfo.pBindings = &binding;

    if (vkCreateDescriptorSetLayout(deviceModule->device, &layoutInfo, nullptr, &resultLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptor set layout!");
    }

    return resultLayout;
}

void PointShadowDescriptorsManager::CreateOffscreenDescriptorSet()
{
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = this->offscreenDescriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &this->offscreenDescriptorSetLayout;

    std::vector<VkWriteDescriptorSet> descriptorWrites{};
    descriptorWrites.resize(this->_numPointLights);
    this->offscreenBuffersInfo.resize(this->_numPointLights);

    for (uint32_t npl = 0; npl < this->_numPointLights; npl++)
    {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            if (vkAllocateDescriptorSets(deviceModule->device, &allocInfo, &this->offscreenDescriptorSets[i][npl]) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to allocate descriptor sets!");
            }

            this->SetOffscreenDescriptorWrite(descriptorWrites[npl], this->offscreenDescriptorSets[i][npl], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, this->shadowMapUBOs[npl]->uniformBuffers[i], sizeof(OmniShadowUniform));
            vkUpdateDescriptorSets(deviceModule->device, 1, &descriptorWrites[npl], 0, nullptr);
        }
    }
}

void PointShadowDescriptorsManager::Clean()
{
    for (uint32_t npl = 0; npl < this->_numPointLights; npl++)
    {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            if (this->offscreenDescriptorSets[i][npl] != VK_NULL_HANDLE)
            {
                this->offscreenDescriptorSets[i][npl] = VK_NULL_HANDLE;
            }
        }
    }

    if (this->offscreenDescriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(deviceModule->device, this->offscreenDescriptorPool, nullptr);
        this->offscreenDescriptorPool = VK_NULL_HANDLE;
    }

    if (this->renderDescriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(deviceModule->device, this->renderDescriptorPool, nullptr);
        this->renderDescriptorPool = VK_NULL_HANDLE;
    }

    if (this->renderDescriptorSetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(deviceModule->device, this->renderDescriptorSetLayout, nullptr);
        this->renderDescriptorSetLayout = VK_NULL_HANDLE;
    }

    if (this->placeholderSampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(this->deviceModule->device, this->placeholderSampler, nullptr);
    }

    if (this->placeholderImageView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(deviceModule->device, placeholderImageView, nullptr);
    }
}
