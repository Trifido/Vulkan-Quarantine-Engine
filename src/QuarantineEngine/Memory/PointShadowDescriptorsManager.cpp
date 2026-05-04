#include "PointShadowDescriptorsManager.h"
#include "SynchronizationModule.h"
#include <OmniShadowResources.h>
#include <Helpers/QEMemoryTrack.h>
#include <stdexcept>
#include <QueueModule.h>

PointShadowDescriptorsManager::PointShadowDescriptorsManager()
{
    this->deviceModule = DeviceModule::getInstance();

    this->CreateOffscreenDescriptorPool();
    this->CreateRenderDescriptorPool();
}

void PointShadowDescriptorsManager::AddPointLightResources(
    std::shared_ptr<UniformBufferObject> shadowMapUBO,
    VkImageView imageView,
    VkSampler sampler)
{
    if (!shadowMapUBO || imageView == VK_NULL_HANDLE || sampler == VK_NULL_HANDLE)
        return;

    if (this->_numPointLights >= MAX_NUM_POINT_LIGHTS)
        return;

    const uint32_t newLightIndex = this->_numPointLights;

    this->shadowMapUBOs.push_back(shadowMapUBO);
    this->_imageViews.push_back(imageView);
    this->_samplers.push_back(sampler);
    this->_numPointLights++;

    if (this->offscreenDescriptorSetLayout == VK_NULL_HANDLE)
        return;

    WaitForGpuIdle();

    this->AllocateOffscreenDescriptorSetForLight(newLightIndex);

    if (this->renderDescriptorSets[0] != VK_NULL_HANDLE)
    {
        this->UpdateRenderDescriptorSets();
    }
}

void PointShadowDescriptorsManager::DeletePointLightResources(int idPos)
{
    if (idPos < 0 || static_cast<uint32_t>(idPos) >= this->_numPointLights)
        return;

    WaitForGpuIdle();

    this->shadowMapUBOs.erase(this->shadowMapUBOs.begin() + idPos);
    this->_imageViews.erase(this->_imageViews.begin() + idPos);
    this->_samplers.erase(this->_samplers.begin() + idPos);
    this->_numPointLights--;

    for (size_t frame = 0; frame < NUM_POINT_SHADOW_SETS; ++frame)
    {
        for (uint32_t i = static_cast<uint32_t>(idPos); i < MAX_NUM_POINT_LIGHTS - 1; ++i)
        {
            offscreenDescriptorSets[frame][i] = offscreenDescriptorSets[frame][i + 1];
        }

        offscreenDescriptorSets[frame][MAX_NUM_POINT_LIGHTS - 1] = VK_NULL_HANDLE;
    }

    if (this->renderDescriptorSets[0] != VK_NULL_HANDLE)
    {
        this->UpdateRenderDescriptorSets();
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
    VkDescriptorPoolSize offscreenPoolSize{};
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
    VkDescriptorPoolSize renderPoolSize{};
    renderPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    renderPoolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * MAX_NUM_POINT_LIGHTS;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &renderPoolSize;
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    if (vkCreateDescriptorPool(this->deviceModule->device, &poolInfo, nullptr, &this->renderDescriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create render point shadow descriptor pool!");
    }
}

VkDescriptorBufferInfo PointShadowDescriptorsManager::GetBufferInfo(VkBuffer buffer, VkDeviceSize bufferSize)
{
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = bufferSize;
    return bufferInfo;
}

void PointShadowDescriptorsManager::CreateCubemapPlaceHolder()
{
    const VkFormat shadowColorFormat = OmniShadowResources::GetSupportedColorFormat(deviceModule);

    this->placeholderImage = OmniShadowResources::AllocateCubemapImage(
        deviceModule->device,
        deviceModule->physicalDevice,
        this->placeholderMemory,
        1,
        shadowColorFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        1);

    VkImageSubresourceRange subresourceRange{};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.layerCount = 6;

    OmniShadowResources::TransitionMultiImagesLayout(
        this->deviceModule->device,
        this->placeholderImage,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        subresourceRange);

    this->placeholderSampler = OmniShadowResources::CreateCubemapSampler(this->deviceModule->device);
    this->placeholderImageView = OmniShadowResources::CreateCubemapImageView(
        this->deviceModule->device,
        this->placeholderImage,
        shadowColorFormat,
        VK_IMAGE_ASPECT_COLOR_BIT);
}

void PointShadowDescriptorsManager::SetOffscreenDescriptorWrite(
    VkWriteDescriptorSet& descriptorWrite,
    VkDescriptorSet descriptorSet,
    VkDescriptorType descriptorType,
    uint32_t binding,
    VkBuffer buffer,
    VkDeviceSize bufferSize)
{
    this->offscreenBufferInfo = GetBufferInfo(buffer, bufferSize);

    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = binding;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = descriptorType;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = nullptr;
    descriptorWrite.pBufferInfo = &this->offscreenBufferInfo;
    descriptorWrite.pTexelBufferView = nullptr;
}

void PointShadowDescriptorsManager::SetCubeMapDescriptorWrite(
    VkWriteDescriptorSet& descriptorWrite,
    VkDescriptorSet descriptorSet,
    VkDescriptorType descriptorType,
    uint32_t binding)
{
    for (uint32_t i = 0; i < MAX_NUM_POINT_LIGHTS; ++i)
    {
        this->renderDescriptorImageInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        if (i < _numPointLights)
        {
            this->renderDescriptorImageInfo[i].imageView = _imageViews[i];
            this->renderDescriptorImageInfo[i].sampler = _samplers[i];
        }
        else
        {
            this->renderDescriptorImageInfo[i].imageView = placeholderImageView;
            this->renderDescriptorImageInfo[i].sampler = placeholderSampler;
        }
    }

    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = binding;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = descriptorType;
    descriptorWrite.descriptorCount = MAX_NUM_POINT_LIGHTS;
    descriptorWrite.pImageInfo = this->renderDescriptorImageInfo.data();
    descriptorWrite.pBufferInfo = nullptr;
    descriptorWrite.pTexelBufferView = nullptr;
}

void PointShadowDescriptorsManager::CreateRenderDescriptorSet()
{
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, renderDescriptorSetLayout);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = this->renderDescriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    this->renderDescriptorImageInfo.resize(MAX_NUM_POINT_LIGHTS);

    if (vkAllocateDescriptorSets(deviceModule->device, &allocInfo, this->renderDescriptorSets) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate render descriptor sets!");
    }

    this->UpdateRenderDescriptorSets();
}

VkDescriptorSetLayout PointShadowDescriptorsManager::CreateRenderDescriptorSetLayout()
{
    VkDescriptorSetLayout resultLayout = VK_NULL_HANDLE;

    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.descriptorCount = MAX_NUM_POINT_LIGHTS;
    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    binding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &binding;

    if (vkCreateDescriptorSetLayout(deviceModule->device, &layoutInfo, nullptr, &resultLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptor set layout!");
    }

    return resultLayout;
}

void PointShadowDescriptorsManager::AllocateOffscreenDescriptorSetForLight(uint32_t lightIndex)
{
    if (lightIndex >= _numPointLights)
        return;

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = this->offscreenDescriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &this->offscreenDescriptorSetLayout;

    for (size_t frame = 0; frame < MAX_FRAMES_IN_FLIGHT; ++frame)
    {
        if (offscreenDescriptorSets[frame][lightIndex] == VK_NULL_HANDLE)
        {
            if (vkAllocateDescriptorSets(deviceModule->device, &allocInfo, &offscreenDescriptorSets[frame][lightIndex]) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to allocate offscreen descriptor set!");
            }
        }

        VkWriteDescriptorSet write{};
        this->SetOffscreenDescriptorWrite(
            write,
            offscreenDescriptorSets[frame][lightIndex],
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            0,
            this->shadowMapUBOs[lightIndex]->uniformBuffers[frame],
            sizeof(OmniShadowUniform));

        vkUpdateDescriptorSets(deviceModule->device, 1, &write, 0, nullptr);
    }
}

void PointShadowDescriptorsManager::CreateOffscreenDescriptorSet()
{
    for (uint32_t lightIndex = 0; lightIndex < this->_numPointLights; ++lightIndex)
    {
        this->AllocateOffscreenDescriptorSetForLight(lightIndex);
    }
}

void PointShadowDescriptorsManager::UpdateRenderDescriptorSets()
{
    for (size_t frame = 0; frame < MAX_FRAMES_IN_FLIGHT; ++frame)
    {
        if (this->renderDescriptorSets[frame] == VK_NULL_HANDLE)
            continue;

        VkWriteDescriptorSet write{};
        this->SetCubeMapDescriptorWrite(
            write,
            this->renderDescriptorSets[frame],
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            0);

        vkUpdateDescriptorSets(deviceModule->device, 1, &write, 0, nullptr);
    }
}

void PointShadowDescriptorsManager::ResetSceneState()
{
    WaitForGpuIdle();

    if (this->offscreenDescriptorPool != VK_NULL_HANDLE)
    {
        vkResetDescriptorPool(deviceModule->device, this->offscreenDescriptorPool, 0);
    }

    if (this->renderDescriptorPool != VK_NULL_HANDLE)
    {
        vkResetDescriptorPool(deviceModule->device, this->renderDescriptorPool, 0);
    }

    _numPointLights = 0;

    shadowMapUBOs.clear();
    _imageViews.clear();
    _samplers.clear();
    renderDescriptorImageInfo.clear();
    offscreenBufferInfo = {};

    for (size_t i = 0; i < NUM_POINT_SHADOW_SETS; i++)
    {
        renderDescriptorSets[i] = VK_NULL_HANDLE;

        for (size_t j = 0; j < MAX_NUM_POINT_LIGHTS; j++)
        {
            offscreenDescriptorSets[i][j] = VK_NULL_HANDLE;
        }
    }
}

void PointShadowDescriptorsManager::Clean()
{
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
        this->placeholderSampler = VK_NULL_HANDLE;
    }

    if (this->placeholderImageView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(deviceModule->device, this->placeholderImageView, nullptr);
        this->placeholderImageView = VK_NULL_HANDLE;
    }

    if (this->placeholderImage != VK_NULL_HANDLE)
    {
        vkDestroyImage(deviceModule->device, this->placeholderImage, nullptr);
        this->placeholderImage = VK_NULL_HANDLE;
    }

    if (this->placeholderMemory != VK_NULL_HANDLE)
    {
        QE_FREE_MEMORY(deviceModule->device, this->placeholderMemory, "PointShadowDescriptorsManager::Clean");
        this->placeholderMemory = VK_NULL_HANDLE;
    }

    this->ResetSceneState();
}

void PointShadowDescriptorsManager::WaitForGpuIdle() const
{
    if (!deviceModule)
        return;

    vkDeviceWaitIdle(deviceModule->device);
}
