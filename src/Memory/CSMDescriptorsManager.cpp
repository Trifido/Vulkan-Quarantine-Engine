#include "CSMDescriptorsManager.h"
#include "SynchronizationModule.h"

CSMDescriptorsManager::CSMDescriptorsManager()
{
    this->deviceModule = DeviceModule::getInstance();

    this->CreateOffscreenDescriptorPool();
    this->CreateRenderDescriptorPool();

    this->csmRenderSplitBuffer = UniformBufferObject();
    this->csmSplitDataBufferSize = sizeof(float) * CSM_NUM * MAX_NUM_DIR_LIGHTS;
    this->csmRenderSplitBuffer.CreateUniformBuffer(this->csmSplitDataBufferSize, MAX_FRAMES_IN_FLIGHT, *deviceModule);

    this->csmRenderViewProjBuffer = UniformBufferObject();
    this->csmViewProjDataBufferSize = sizeof(glm::mat4) * CSM_NUM * MAX_NUM_DIR_LIGHTS;
    this->csmRenderViewProjBuffer.CreateSSBO(this->csmViewProjDataBufferSize, MAX_FRAMES_IN_FLIGHT, *deviceModule);
}

void CSMDescriptorsManager::AddDirLightResources(std::shared_ptr<UniformBufferObject> offscreenShadowMapUBO, VkImageView imageView, VkSampler sampler)
{
    if (this->_numDirLights < MAX_NUM_DIR_LIGHTS)
    {
        this->csmOffscreenUBOs.push_back(offscreenShadowMapUBO);
        this->_imageViews.push_back(imageView);
        this->_samplers.push_back(sampler);
        this->_numDirLights++;
    }
}

void CSMDescriptorsManager::DeleteDirLightResources(int idPos)
{
    if (idPos >= 0 && idPos < this->_numDirLights)
    {
        this->csmOffscreenUBOs.erase(this->csmOffscreenUBOs.begin() + idPos);
        this->_imageViews.erase(this->_imageViews.begin() + idPos);
        this->_samplers.erase(this->_samplers.begin() + idPos);
        this->_numDirLights--;
    }
}

void CSMDescriptorsManager::BindResources(std::shared_ptr<std::array<CascadeResource, SHADOW_MAP_CASCADE_COUNT>> resources)
{
    this->csmResources.push_back(resources);
}

void CSMDescriptorsManager::UpdateResources(int currentFrame)
{
    this->csmSplitDataResources.clear();
    this->csmViewProjDataResources.clear();

    for (int i = 0; i < MAX_NUM_DIR_LIGHTS; i++)
    {
        if (i < this->csmResources.size())
        {
            for (int j = 0; j < CSM_NUM; j++)
            {
                this->csmSplitDataResources.push_back(this->csmResources[i]->at(j).splitDepth);
                this->csmViewProjDataResources.push_back(this->csmResources[i]->at(j).viewProjMatrix);
            }
        }
        else
        {
            for (int j = 0; j < CSM_NUM; j++)
            {
                this->csmSplitDataResources.push_back(0);
                this->csmViewProjDataResources.push_back(glm::mat4(1.0f));
            }
        }
    }

    void* data;
    vkMapMemory(this->deviceModule->device, this->csmRenderSplitBuffer.uniformBuffersMemory[currentFrame], 0, this->csmSplitDataBufferSize, 0, &data);
    memcpy(data, this->csmSplitDataResources.data(), this->csmSplitDataBufferSize);
    vkUnmapMemory(this->deviceModule->device, this->csmRenderSplitBuffer.uniformBuffersMemory[currentFrame]);

    void* dataViewProj;
    vkMapMemory(this->deviceModule->device, this->csmRenderViewProjBuffer.uniformBuffersMemory[currentFrame], 0, this->csmViewProjDataBufferSize, 0, &dataViewProj);
    memcpy(dataViewProj, this->csmViewProjDataResources.data(), this->csmViewProjDataBufferSize);
    vkUnmapMemory(this->deviceModule->device, this->csmRenderViewProjBuffer.uniformBuffersMemory[currentFrame]);
}

void CSMDescriptorsManager::InitializeDescriptorSetLayouts(std::shared_ptr<ShaderModule> offscreen_shader_ptr)
{
    this->CreateCSMPlaceHolder();

    this->offscreenDescriptorSetLayout = offscreen_shader_ptr->descriptorSetLayouts.at(0);
    this->renderDescriptorSetLayout = this->CreateRenderDescriptorSetLayout();

    this->CreateOffscreenDescriptorSet();
    this->CreateRenderDescriptorSet();
}

void CSMDescriptorsManager::CreateOffscreenDescriptorPool()
{
    // Point Camera Uniform
    VkDescriptorPoolSize offscreenPoolSize;
    offscreenPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    offscreenPoolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * MAX_NUM_DIR_LIGHTS;

    VkDescriptorPoolCreateInfo offscreenPoolInfo{};
    offscreenPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    offscreenPoolInfo.poolSizeCount = 1;
    offscreenPoolInfo.pPoolSizes = &offscreenPoolSize;
    offscreenPoolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * MAX_NUM_DIR_LIGHTS;

    if (vkCreateDescriptorPool(this->deviceModule->device, &offscreenPoolInfo, nullptr, &this->offscreenDescriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create offscreen descriptor pool!");
    }
}

void CSMDescriptorsManager::CreateRenderDescriptorPool()
{
    std::vector<VkDescriptorPoolSize> renderPoolSizes;
    renderPoolSizes.resize(3);

    // Shadow Texture Samplers
    renderPoolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    renderPoolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * MAX_NUM_DIR_LIGHTS;

    // CascadeSplits buffer
    renderPoolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    renderPoolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    // VIEWPROJ buffer
    renderPoolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    renderPoolSizes[2].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(renderPoolSizes.size());
    poolInfo.pPoolSizes = renderPoolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * MAX_NUM_DIR_LIGHTS;

    if (vkCreateDescriptorPool(this->deviceModule->device, &poolInfo, nullptr, &this->renderDescriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create render point shadow descriptor pool!");
    }
}

VkDescriptorBufferInfo CSMDescriptorsManager::GetBufferInfo(VkBuffer buffer, VkDeviceSize bufferSize)
{
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = bufferSize;
    return bufferInfo;
}

void CSMDescriptorsManager::CreateCSMPlaceHolder()
{
    this->placeholderImage = CSMResources::AllocateImage(
        deviceModule->device,
        deviceModule->physicalDevice,
        this->placeholderMemory,
        1,
        VK_FORMAT_D32_SFLOAT,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        1);

    CSMResources::TransitionImageLayout(deviceModule->device, this->placeholderImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);

    this->placeholderImageView = CSMResources::CreateImageView(this->deviceModule->device, this->placeholderImage, VK_FORMAT_D32_SFLOAT, VK_IMAGE_ASPECT_DEPTH_BIT, 0, CSM_NUM);
    this->placeholderSampler = CSMResources::CreateCSMSampler(this->deviceModule->device);
}

void CSMDescriptorsManager::SetOffscreenDescriptorWrite(VkWriteDescriptorSet& descriptorWrite, VkDescriptorSet descriptorSet, VkDescriptorType descriptorType, uint32_t binding, VkBuffer buffer, VkDeviceSize bufferSize)
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

void CSMDescriptorsManager::SetRenderDescriptorWrite(VkWriteDescriptorSet& descriptorWrite, VkDescriptorSet descriptorSet, VkDescriptorType descriptorType, uint32_t binding, VkBuffer buffer, VkDeviceSize bufferSize)
{
    this->renderBuffersInfo[binding - 1] = GetBufferInfo(buffer, bufferSize);

    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = binding;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = descriptorType;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = VK_NULL_HANDLE;
    descriptorWrite.pBufferInfo = &this->renderBuffersInfo[binding - 1];
}

void CSMDescriptorsManager::SetCSMDescriptorWrite(VkWriteDescriptorSet& descriptorWrite, VkDescriptorSet descriptorSet, VkDescriptorType descriptorType, uint32_t binding)
{
    for (uint32_t i = 0; i < MAX_NUM_DIR_LIGHTS; ++i)
    {
        this->renderDescriptorImageInfo[i].imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        if (i < _numDirLights)
        {
            this->renderDescriptorImageInfo[i].imageView = _imageViews[i]; // ImageView de cada csm
            this->renderDescriptorImageInfo[i].sampler = _samplers[i]; // Sampler para cada csm
        }
        else
        {
            this->renderDescriptorImageInfo[i].imageView = placeholderImageView; // ImageView placeholder para cada csm
            this->renderDescriptorImageInfo[i].sampler = placeholderSampler; // Sampler placeholder para cada csm
        }
    }

    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = binding;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = descriptorType;
    descriptorWrite.descriptorCount = MAX_NUM_DIR_LIGHTS; // Número de descriptores en el array
    descriptorWrite.pImageInfo = this->renderDescriptorImageInfo.data();
}

void CSMDescriptorsManager::CreateRenderDescriptorSet()
{
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, renderDescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = this->renderDescriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    this->renderDescriptorImageInfo.resize(MAX_NUM_DIR_LIGHTS);
    this->renderBuffersInfo.resize(2);

    if (vkAllocateDescriptorSets(deviceModule->device, &allocInfo, this->renderDescriptorSets) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        std::vector<VkWriteDescriptorSet> descriptorWrites{};
        descriptorWrites.resize(3);

        this->SetCSMDescriptorWrite(descriptorWrites[0], this->renderDescriptorSets[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0);
        this->SetRenderDescriptorWrite(descriptorWrites[1], this->renderDescriptorSets[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, this->csmRenderSplitBuffer.uniformBuffers[i], this->csmSplitDataBufferSize);
        this->SetRenderDescriptorWrite(descriptorWrites[2], this->renderDescriptorSets[i], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2, this->csmRenderViewProjBuffer.uniformBuffers[i], this->csmViewProjDataBufferSize);

        vkUpdateDescriptorSets(deviceModule->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}

VkDescriptorSetLayout CSMDescriptorsManager::CreateRenderDescriptorSetLayout()
{
    VkDescriptorSetLayout resultLayout = {};

    std::array<VkDescriptorSetLayoutBinding, 3> layoutBindings{};

    layoutBindings[0].binding = 0;
    layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layoutBindings[0].descriptorCount = MAX_NUM_DIR_LIGHTS;
    layoutBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    layoutBindings[0].pImmutableSamplers = nullptr;

    layoutBindings[1].binding = 1;
    layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBindings[1].descriptorCount = 1;
    layoutBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    layoutBindings[1].pImmutableSamplers = nullptr;

    layoutBindings[2].binding = 2;
    layoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    layoutBindings[2].descriptorCount = 1;
    layoutBindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    layoutBindings[2].pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = (uint32_t)layoutBindings.size();
    layoutInfo.pBindings = layoutBindings.data();

    if (vkCreateDescriptorSetLayout(deviceModule->device, &layoutInfo, nullptr, &resultLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptor set layout!");
    }

    return resultLayout;
}

void CSMDescriptorsManager::CreateOffscreenDescriptorSet()
{
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = this->offscreenDescriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &this->offscreenDescriptorSetLayout;

    std::vector<VkWriteDescriptorSet> descriptorWrites{};
    descriptorWrites.resize(this->_numDirLights);
    this->offscreenBuffersInfo.resize(this->_numDirLights);

    for (uint32_t ndl = 0; ndl < this->_numDirLights; ndl++)
    {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            if (vkAllocateDescriptorSets(deviceModule->device, &allocInfo, &this->offscreenDescriptorSets[i][ndl]) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to allocate descriptor sets!");
            }

            this->SetOffscreenDescriptorWrite(descriptorWrites[ndl], this->offscreenDescriptorSets[i][ndl],
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, this->csmOffscreenUBOs[ndl]->uniformBuffers[i], sizeof(CSMUniform));
            vkUpdateDescriptorSets(deviceModule->device, 1, &descriptorWrites[ndl], 0, nullptr);
        }
    }
}

void CSMDescriptorsManager::Clean()
{
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (this->csmRenderSplitBuffer.uniformBuffers[i] != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(deviceModule->device, this->csmRenderSplitBuffer.uniformBuffers[i], nullptr);
            vkFreeMemory(deviceModule->device, this->csmRenderSplitBuffer.uniformBuffersMemory[i], nullptr);
        }

        if (this->csmRenderViewProjBuffer.uniformBuffers[i] != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(deviceModule->device, this->csmRenderViewProjBuffer.uniformBuffers[i], nullptr);
            vkFreeMemory(deviceModule->device, this->csmRenderViewProjBuffer.uniformBuffersMemory[i], nullptr);
        }
    }

    for (uint32_t npl = 0; npl < this->_numDirLights; npl++)
    {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            if (this->offscreenDescriptorSets[i][npl] != VK_NULL_HANDLE)
            {
                this->offscreenDescriptorSets[i][npl] = VK_NULL_HANDLE;
            }

            if (this->renderDescriptorSets[i] != VK_NULL_HANDLE)
            {
                this->renderDescriptorSets[i] = VK_NULL_HANDLE;
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
        vkDestroyImageView(deviceModule->device, this->placeholderImageView, nullptr);
    }

    if (this->placeholderImage != VK_NULL_HANDLE)
    {
        vkDestroyImage(deviceModule->device, this->placeholderImage, nullptr);
    }

    if (this->placeholderMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(deviceModule->device, this->placeholderMemory, nullptr);
    }
}
