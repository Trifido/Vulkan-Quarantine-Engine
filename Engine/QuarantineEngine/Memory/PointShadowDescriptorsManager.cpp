#include "PointShadowDescriptorsManager.h"
#include "SynchronizationModule.h"

PointShadowDescriptorsManager::PointShadowDescriptorsManager()
{
    this->deviceModule = DeviceModule::getInstance();

    this->CreateOffscreenDescriptorPool();
    this->CreateRenderDescriptorPool();
}

void PointShadowDescriptorsManager::AddPointLightResources(std::shared_ptr<UniformBufferObject> shadowMapUBO)
{
    this->shadowMapUBOs.push_back(shadowMapUBO);
    this->_numPointLights++;
}

void PointShadowDescriptorsManager::InitializeDescriptorSetLayouts(std::shared_ptr<ShaderModule> offscreen_shader_ptr)
{
    this->offscreenDescriptorSetLayout = offscreen_shader_ptr->descriptorSetLayouts.at(0);
    //this->renderDescriptorSetLayout = render_shader_ptr->descriptorSetLayouts.at(1);

    this->CreateOffscreenDescriptorSet();
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
    renderPoolSizes.resize(2);

    // Point Light Uniform
    renderPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    renderPoolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * MAX_NUM_POINT_LIGHTS;

    // Shadow Texture Samplers
    renderPoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    renderPoolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * MAX_NUM_POINT_LIGHTS;

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
    VkDescriptorImageInfo imageInfos[MAX_NUM_POINT_LIGHTS];
    for (int i = 0; i < 10; ++i)
    {
        imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfos[i].imageView = *this->_imageViews[i]; // ImageView de cada cubemap
        imageInfos[i].sampler = *this->_samplers[i]; // Sampler para cada cubemap
    }

    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = binding;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = descriptorType;
    descriptorWrite.descriptorCount = 10; // Número de cubemaps en el arreglo
    descriptorWrite.pImageInfo = imageInfos;
}

void PointShadowDescriptorsManager::CreateRenderDescriptorSet()
{
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, renderDescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = this->renderDescriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    if (vkAllocateDescriptorSets(deviceModule->device, &allocInfo, this->renderDescriptorSets) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        std::vector<VkWriteDescriptorSet> descriptorWrites{};
        descriptorWrites.resize(2);

        this->SetRenderDescriptorWrite(descriptorWrites[0], this->renderDescriptorSets[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, pointlightIdBuffer->uniformBuffers[i], sizePointlightIdBuffer);
        this->SetCubeMapDescriptorWrite(descriptorWrites[1], this->renderDescriptorSets[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1);

        vkUpdateDescriptorSets(deviceModule->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}

void PointShadowDescriptorsManager::CreateOffscreenDescriptorSet()
{
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = this->offscreenDescriptorPool;
    allocInfo.descriptorSetCount = 1;// static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
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

            this->SetOffscreenDescriptorWrite(descriptorWrites[npl], this->offscreenDescriptorSets[i][npl], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, npl, this->shadowMapUBOs[npl]->uniformBuffers[i], sizeof(OmniShadowUniform));
            vkUpdateDescriptorSets(deviceModule->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
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

}
