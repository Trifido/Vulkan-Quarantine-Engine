#include "DescriptorBuffer.h"
#include "SynchronizationModule.h"
#include <CameraEditor.h>

DescriptorBuffer::DescriptorBuffer()
{
    this->deviceModule = DeviceModule::getInstance();
}

DescriptorBuffer::DescriptorBuffer(std::shared_ptr<ShaderModule> shader_ptr) : DescriptorBuffer()
{
    this->numBinding = shader_ptr->reflectShader.bindings.size();
    this->materialUBO = std::make_shared<UniformBufferObject>();
    this->StartResources(shader_ptr);
}

void DescriptorBuffer::CleanLastResources()
{
    this->deviceModule = nullptr;
    this->lightManager = nullptr;
    this->camera = nullptr;
    this->textures.reset();
    this->textures = nullptr;
    this->materialUBO.reset();
    this->materialUBO = nullptr;
    this->animationUBO.reset();
    this->animationUBO = nullptr;
}

void DescriptorBuffer::StartResources(std::shared_ptr<ShaderModule> shader_ptr)
{
    std::vector<VkDescriptorPoolSize> poolSizes;
    poolSizes.resize(this->numBinding);
    size_t idx = 0;

    for each (auto binding in shader_ptr->reflectShader.bindings)
    {
        if (binding.first == "CameraUniform")
        {
            this->camera = CameraEditor::getInstance();
            poolSizes[idx].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            poolSizes[idx].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
            idx++;
        }
        else if (binding.first == "UniformManagerLight")
        {
            this->lightManager = LightManager::getInstance();
            poolSizes[idx].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            poolSizes[idx].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
            idx++;
        }
        else if (binding.first == "UniformMaterial")
        {
            poolSizes[idx].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            poolSizes[idx].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
            idx++;
        }
        else if (binding.first == "UniformAnimation")
        {
            poolSizes[idx].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            poolSizes[idx].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
            idx++;
        }
        else if (binding.first == "Texture2DArray")
        {
            poolSizes[idx].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            poolSizes[idx].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
            idx++;
        }
    }

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    if (vkCreateDescriptorPool(deviceModule->device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

VkDescriptorBufferInfo DescriptorBuffer::GetBufferInfo(VkBuffer buffer, VkDeviceSize bufferSize)
{
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = bufferSize;
    return bufferInfo;
}

void DescriptorBuffer::SetDescriptorWrite(VkWriteDescriptorSet& descriptorWrite, uint32_t binding, VkBuffer buffer, VkDeviceSize bufferSize, uint32_t frameIdx)
{
    this->buffersInfo[binding] = GetBufferInfo(buffer, bufferSize);

    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSets[frameIdx];
    descriptorWrite.dstBinding = binding;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = VK_NULL_HANDLE;
    descriptorWrite.pBufferInfo = &this->buffersInfo[binding];
}

std::vector<VkWriteDescriptorSet> DescriptorBuffer::GetDescriptorWrites(std::shared_ptr<ShaderModule> shader_ptr, uint32_t frameIdx)
{
    std::vector<VkWriteDescriptorSet> descriptorWrites{};
    descriptorWrites.resize(this->numBinding);
    this->buffersInfo.resize(this->numBinding);
    uint32_t idx = 0;

    for each (auto binding in shader_ptr->reflectShader.bindings)
    {
        if (binding.first == "CameraUniform")
        {
            this->SetDescriptorWrite(descriptorWrites[idx], binding.second.binding, this->camera->cameraUBO->uniformBuffers[frameIdx], sizeof(CameraUniform), frameIdx);
            idx++;
        }
        else if (binding.first == "UniformManagerLight")
        {
            this->SetDescriptorWrite(descriptorWrites[idx], binding.second.binding, this->lightManager->lightUBO->uniformBuffers[frameIdx], sizeof(LightUniform), frameIdx);
            idx++;
        }
        else if (binding.first == "UniformMaterial")
        {
            this->SetDescriptorWrite(descriptorWrites[idx], binding.second.binding, this->materialUBO->uniformBuffers[frameIdx], this->materialUniformSize, frameIdx);
            idx++;
        }
        else if (binding.first == "UniformAnimation")
        {
            this->SetDescriptorWrite(descriptorWrites[idx], binding.second.binding, this->animationUBO->uniformBuffers[frameIdx], this->animationUniformSize, frameIdx);
            idx++;
        }
        else if (binding.first == "Texture2DArray")
        {
            this->imageInfo.resize(textures->size());
            for (uint32_t id = 0; id < textures->size(); ++id)
            {
                this->imageInfo[id].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                this->imageInfo[id].imageView = textures->at(id)->imageView;
                this->imageInfo[id].sampler = textures->at(id)->textureSampler;
            }

            descriptorWrites[idx] = {};
            descriptorWrites[idx].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[idx].dstBinding = binding.second.binding;
            descriptorWrites[idx].dstArrayElement = 0;
            descriptorWrites[idx].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[idx].descriptorCount = textures->size();
            descriptorWrites[idx].pBufferInfo = VK_NULL_HANDLE;
            descriptorWrites[idx].dstSet = descriptorSets[frameIdx];
            descriptorWrites[idx].pImageInfo = this->imageInfo.data();

            idx++;
        }
    }

    return descriptorWrites;
}

void DescriptorBuffer::InitializeDescriptorSets(std::shared_ptr<ShaderModule> shader_ptr)
{
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, shader_ptr->descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = this->descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(deviceModule->device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    size_t numDescriptors = this->numBinding;
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        std::vector<VkWriteDescriptorSet> descriptorWrites{};
        descriptorWrites = this->GetDescriptorWrites(shader_ptr, i);
        vkUpdateDescriptorSets(deviceModule->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}

void DescriptorBuffer::CleanDescriptorSetPool()
{
    vkDestroyDescriptorPool(deviceModule->device, this->descriptorPool, nullptr);
}
