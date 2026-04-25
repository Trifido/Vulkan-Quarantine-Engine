#include "SpotShadowDescriptorsManager.h"
#include "SynchronizationModule.h"
#include <ImageMemoryTools.h>
#include <stdexcept>
#include <Helpers/QEMemoryTrack.h>

SpotShadowDescriptorsManager::SpotShadowDescriptorsManager()
{
    this->deviceModule = DeviceModule::getInstance();

    this->CreateOffscreenDescriptorPool();
    this->CreateRenderDescriptorPool();

    this->spotViewProjDataBufferSize = sizeof(glm::mat4) * MAX_NUM_SPOT_LIGHTS;
    this->spotRenderViewProjBuffer.CreateSSBO(this->spotViewProjDataBufferSize, MAX_FRAMES_IN_FLIGHT, *deviceModule);
}

void SpotShadowDescriptorsManager::AddSpotLightResources(
    std::shared_ptr<UniformBufferObject> offscreenShadowMapUBO,
    VkImageView imageView,
    VkSampler sampler)
{
    if (!offscreenShadowMapUBO || imageView == VK_NULL_HANDLE || sampler == VK_NULL_HANDLE)
        return;

    if (this->_numSpotLights >= MAX_NUM_SPOT_LIGHTS)
        return;

    const uint32_t newLightIndex = this->_numSpotLights;

    this->offscreenShadowMapUBOs.push_back(offscreenShadowMapUBO);
    this->_imageViews.push_back(imageView);
    this->_samplers.push_back(sampler);
    this->_numSpotLights++;

    if (this->offscreenDescriptorSetLayout == VK_NULL_HANDLE)
        return;

    WaitForGpuIdle();
    this->AllocateOffscreenDescriptorSetForLight(newLightIndex);

    if (this->renderDescriptorSets[0] != VK_NULL_HANDLE)
    {
        this->UpdateRenderDescriptorSets();
    }
}

void SpotShadowDescriptorsManager::DeleteSpotLightResources(int idPos)
{
    if (idPos < 0 || static_cast<uint32_t>(idPos) >= this->_numSpotLights)
        return;

    WaitForGpuIdle();

    this->offscreenShadowMapUBOs.erase(this->offscreenShadowMapUBOs.begin() + idPos);
    this->_imageViews.erase(this->_imageViews.begin() + idPos);
    this->_samplers.erase(this->_samplers.begin() + idPos);

    if (idPos < static_cast<int>(this->shadowResources.size()))
    {
        this->shadowResources.erase(this->shadowResources.begin() + idPos);
    }

    this->_numSpotLights--;

    for (size_t frame = 0; frame < NUM_SPOT_SHADOW_SETS; ++frame)
    {
        for (uint32_t i = static_cast<uint32_t>(idPos); i < MAX_NUM_SPOT_LIGHTS - 1; ++i)
        {
            offscreenDescriptorSets[frame][i] = offscreenDescriptorSets[frame][i + 1];
        }

        offscreenDescriptorSets[frame][MAX_NUM_SPOT_LIGHTS - 1] = VK_NULL_HANDLE;
    }

    if (this->renderDescriptorSets[0] != VK_NULL_HANDLE)
    {
        this->UpdateRenderDescriptorSets();
    }
}

void SpotShadowDescriptorsManager::BindResources(const std::shared_ptr<SpotShadowResources>& resources)
{
    this->shadowResources.push_back(resources);
}

void SpotShadowDescriptorsManager::UpdateResources(int currentFrame)
{
    this->spotViewProjDataResources.clear();

    for (int i = 0; i < MAX_NUM_SPOT_LIGHTS; i++)
    {
        if (i < static_cast<int>(this->shadowResources.size()) && this->shadowResources[i])
        {
            this->spotViewProjDataResources.push_back(this->shadowResources[i]->ViewProjMatrix);
        }
        else
        {
            this->spotViewProjDataResources.push_back(glm::mat4(1.0f));
        }
    }

    void* dataViewProj = nullptr;
    vkMapMemory(
        this->deviceModule->device,
        this->spotRenderViewProjBuffer.uniformBuffersMemory[currentFrame],
        0,
        this->spotViewProjDataBufferSize,
        0,
        &dataViewProj);
    memcpy(dataViewProj, this->spotViewProjDataResources.data(), this->spotViewProjDataBufferSize);
    vkUnmapMemory(this->deviceModule->device, this->spotRenderViewProjBuffer.uniformBuffersMemory[currentFrame]);
}

void SpotShadowDescriptorsManager::InitializeDescriptorSetLayouts(std::shared_ptr<ShaderModule> offscreen_shader_ptr)
{
    this->CreateShadowPlaceholder();

    this->offscreenDescriptorSetLayout = offscreen_shader_ptr->descriptorSetLayouts.at(0);
    this->renderDescriptorSetLayout = this->CreateRenderDescriptorSetLayout();

    this->CreateOffscreenDescriptorSet();
    this->CreateRenderDescriptorSet();
}

void SpotShadowDescriptorsManager::CreateOffscreenDescriptorPool()
{
    VkDescriptorPoolSize offscreenPoolSize{};
    offscreenPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    offscreenPoolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * MAX_NUM_SPOT_LIGHTS;

    VkDescriptorPoolCreateInfo offscreenPoolInfo{};
    offscreenPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    offscreenPoolInfo.poolSizeCount = 1;
    offscreenPoolInfo.pPoolSizes = &offscreenPoolSize;
    offscreenPoolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * MAX_NUM_SPOT_LIGHTS;

    if (vkCreateDescriptorPool(this->deviceModule->device, &offscreenPoolInfo, nullptr, &this->offscreenDescriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create spot offscreen descriptor pool!");
    }
}

void SpotShadowDescriptorsManager::CreateRenderDescriptorPool()
{
    std::array<VkDescriptorPoolSize, 2> poolSizes{};

    poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * MAX_NUM_SPOT_LIGHTS;

    poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    if (vkCreateDescriptorPool(this->deviceModule->device, &poolInfo, nullptr, &this->renderDescriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create render spot shadow descriptor pool!");
    }
}

VkDescriptorBufferInfo SpotShadowDescriptorsManager::GetBufferInfo(VkBuffer buffer, VkDeviceSize bufferSize)
{
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = bufferSize;
    return bufferInfo;
}

void SpotShadowDescriptorsManager::CreateShadowPlaceholder()
{
    const VkFormat shadowFormat = CSMResources::GetSupportedShadowFormat(deviceModule);

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = { 1, 1, 1 };
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = shadowFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (vkCreateImage(deviceModule->device, &imageInfo, nullptr, &this->placeholderImage) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create spot placeholder image!");
    }

    VkMemoryRequirements memRequirements{};
    vkGetImageMemoryRequirements(deviceModule->device, this->placeholderImage, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = IMT::findMemoryType(
        memRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        deviceModule->physicalDevice);

    if (vkAllocateMemory(deviceModule->device, &allocInfo, nullptr, &this->placeholderMemory) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate spot placeholder image memory!");
    }

    vkBindImageMemory(deviceModule->device, this->placeholderImage, this->placeholderMemory, 0);

    CSMResources::TransitionImageLayout(
        deviceModule->device,
        this->placeholderImage,
        shadowFormat,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
        1);

    this->placeholderImageView = IMT::createImageView(
        deviceModule->device,
        this->placeholderImage,
        VK_IMAGE_VIEW_TYPE_2D,
        shadowFormat,
        VK_IMAGE_ASPECT_DEPTH_BIT);

    this->placeholderSampler = CSMResources::CreateCSMSampler(this->deviceModule->device);
}

void SpotShadowDescriptorsManager::SetOffscreenDescriptorWrite(
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

void SpotShadowDescriptorsManager::SetSpotDescriptorWrite(
    VkWriteDescriptorSet& descriptorWrite,
    VkDescriptorSet descriptorSet,
    VkDescriptorType descriptorType,
    uint32_t binding)
{
    for (uint32_t i = 0; i < MAX_NUM_SPOT_LIGHTS; ++i)
    {
        this->renderDescriptorImageInfo[i].imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

        if (i < _numSpotLights)
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
    descriptorWrite.descriptorCount = MAX_NUM_SPOT_LIGHTS;
    descriptorWrite.pImageInfo = this->renderDescriptorImageInfo.data();
    descriptorWrite.pBufferInfo = nullptr;
    descriptorWrite.pTexelBufferView = nullptr;
}

void SpotShadowDescriptorsManager::SetRenderDescriptorWrite(
    VkWriteDescriptorSet& descriptorWrite,
    VkDescriptorSet descriptorSet,
    VkDescriptorType descriptorType,
    uint32_t binding,
    VkBuffer buffer,
    VkDeviceSize bufferSize)
{
    this->renderBuffersInfo[binding - 1] = GetBufferInfo(buffer, bufferSize);

    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = binding;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = descriptorType;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = nullptr;
    descriptorWrite.pBufferInfo = &this->renderBuffersInfo[binding - 1];
}

void SpotShadowDescriptorsManager::CreateRenderDescriptorSet()
{
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, renderDescriptorSetLayout);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = this->renderDescriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    this->renderDescriptorImageInfo.resize(MAX_NUM_SPOT_LIGHTS);
    this->renderBuffersInfo.resize(1);

    if (vkAllocateDescriptorSets(deviceModule->device, &allocInfo, this->renderDescriptorSets) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate spot render descriptor sets!");
    }

    this->UpdateRenderDescriptorSets();
}

VkDescriptorSetLayout SpotShadowDescriptorsManager::CreateRenderDescriptorSetLayout()
{
    VkDescriptorSetLayout resultLayout = VK_NULL_HANDLE;

    std::array<VkDescriptorSetLayoutBinding, 2> layoutBindings{};

    layoutBindings[0].binding = 0;
    layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layoutBindings[0].descriptorCount = MAX_NUM_SPOT_LIGHTS;
    layoutBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    layoutBindings[0].pImmutableSamplers = nullptr;

    layoutBindings[1].binding = 1;
    layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    layoutBindings[1].descriptorCount = 1;
    layoutBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    layoutBindings[1].pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
    layoutInfo.pBindings = layoutBindings.data();

    if (vkCreateDescriptorSetLayout(deviceModule->device, &layoutInfo, nullptr, &resultLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create spot shadow descriptor set layout!");
    }

    return resultLayout;
}

void SpotShadowDescriptorsManager::AllocateOffscreenDescriptorSetForLight(uint32_t lightIndex)
{
    if (lightIndex >= _numSpotLights)
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
                throw std::runtime_error("failed to allocate spot offscreen descriptor set!");
            }
        }

        VkWriteDescriptorSet write{};
        this->SetOffscreenDescriptorWrite(
            write,
            offscreenDescriptorSets[frame][lightIndex],
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            0,
            this->offscreenShadowMapUBOs[lightIndex]->uniformBuffers[frame],
            sizeof(CSMUniform));

        vkUpdateDescriptorSets(deviceModule->device, 1, &write, 0, nullptr);
    }
}

void SpotShadowDescriptorsManager::CreateOffscreenDescriptorSet()
{
    for (uint32_t lightIndex = 0; lightIndex < this->_numSpotLights; ++lightIndex)
    {
        this->AllocateOffscreenDescriptorSetForLight(lightIndex);
    }
}

void SpotShadowDescriptorsManager::UpdateRenderDescriptorSets()
{
    for (size_t frame = 0; frame < MAX_FRAMES_IN_FLIGHT; ++frame)
    {
        if (this->renderDescriptorSets[frame] == VK_NULL_HANDLE)
            continue;

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

        this->SetSpotDescriptorWrite(
            descriptorWrites[0],
            this->renderDescriptorSets[frame],
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            0);

        this->SetRenderDescriptorWrite(
            descriptorWrites[1],
            this->renderDescriptorSets[frame],
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            1,
            this->spotRenderViewProjBuffer.uniformBuffers[frame],
            this->spotViewProjDataBufferSize);

        vkUpdateDescriptorSets(deviceModule->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}

void SpotShadowDescriptorsManager::ResetSceneState()
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

    _numSpotLights = 0;
    offscreenShadowMapUBOs.clear();
    shadowResources.clear();
    _imageViews.clear();
    _samplers.clear();
    spotViewProjDataResources.clear();
    renderBuffersInfo.clear();
    renderDescriptorImageInfo.clear();
    offscreenBufferInfo = {};

    for (size_t i = 0; i < NUM_SPOT_SHADOW_SETS; i++)
    {
        renderDescriptorSets[i] = VK_NULL_HANDLE;

        for (size_t j = 0; j < MAX_NUM_SPOT_LIGHTS; j++)
        {
            offscreenDescriptorSets[i][j] = VK_NULL_HANDLE;
        }
    }
}

void SpotShadowDescriptorsManager::Clean()
{
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (this->spotRenderViewProjBuffer.uniformBuffers[i] != VK_NULL_HANDLE)
        {
            QE_DESTROY_BUFFER(deviceModule->device, this->spotRenderViewProjBuffer.uniformBuffers[i], "SpotShadowDescriptorsManager::Clean");
            this->spotRenderViewProjBuffer.uniformBuffers[i] = VK_NULL_HANDLE;
        }

        if (this->spotRenderViewProjBuffer.uniformBuffersMemory[i] != VK_NULL_HANDLE)
        {
            QE_FREE_MEMORY(deviceModule->device, this->spotRenderViewProjBuffer.uniformBuffersMemory[i], "SpotShadowDescriptorsManager::Clean");
            this->spotRenderViewProjBuffer.uniformBuffersMemory[i] = VK_NULL_HANDLE;
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
        QE_FREE_MEMORY(deviceModule->device, this->placeholderMemory, "SpotShadowDescriptorsManager::Clean");
        this->placeholderMemory = VK_NULL_HANDLE;
    }
}

void SpotShadowDescriptorsManager::WaitForGpuIdle() const
{
    if (!deviceModule)
        return;

    vkDeviceWaitIdle(deviceModule->device);
}
