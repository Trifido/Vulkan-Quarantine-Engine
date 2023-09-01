#include "ComputeDescriptorBuffer.h"
#include "SynchronizationModule.h"

ComputeDescriptorBuffer::ComputeDescriptorBuffer()
{
	this->deviceModule = DeviceModule::getInstance();
    this->ssbo = std::make_shared<UniformBufferObject>();
}

ComputeDescriptorBuffer::ComputeDescriptorBuffer(std::shared_ptr<ShaderModule> shader_ptr) : ComputeDescriptorBuffer()
{
    this->numBinding = shader_ptr->reflectShader.bindings.size();
    this->StartResources(shader_ptr);
}

void ComputeDescriptorBuffer::StartResources(std::shared_ptr<ShaderModule> shader_ptr)
{
    std::vector<VkDescriptorPoolSize> poolSizes;
    poolSizes.resize(this->numBinding);
    size_t idx = 0;

    for each (auto binding in shader_ptr->reflectShader.bindings)
    {
        if (binding.first == "InputSSBO")
        {
            poolSizes[idx].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            poolSizes[idx].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
            idx++;
        }
        else if (binding.first == "OutputSSBO")
        {
            poolSizes[idx].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            poolSizes[idx].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
            idx++;
        }
        else if (binding.first == "InputImage")
        {
            poolSizes[idx].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            poolSizes[idx].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
            idx++;
        }
        else if (binding.first == "OutputImage")
        {
            poolSizes[idx].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
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

VkDescriptorBufferInfo ComputeDescriptorBuffer::GetBufferInfo(VkBuffer buffer, VkDeviceSize bufferSize)
{
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = bufferSize;
    return bufferInfo;
}

void ComputeDescriptorBuffer::SetDescriptorWrite(VkWriteDescriptorSet& descriptorWrite, uint32_t binding, VkBuffer buffer, VkDeviceSize bufferSize, uint32_t frameIdx)
{
    this->buffersInfo[binding] = this->GetBufferInfo(buffer, bufferSize);

    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSets[frameIdx];
    descriptorWrite.dstBinding = binding;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = VK_NULL_HANDLE;
    descriptorWrite.pBufferInfo = &this->buffersInfo[binding];
}

std::vector<VkWriteDescriptorSet> ComputeDescriptorBuffer::GetDescriptorWrites(std::shared_ptr<ShaderModule> shader_ptr, uint32_t frameIdx)
{
    std::vector<VkWriteDescriptorSet> descriptorWrites{};
    descriptorWrites.resize(this->numBinding);
    this->buffersInfo.resize(this->numBinding);
    uint32_t idx = 0;

    for each (auto binding in shader_ptr->reflectShader.bindings)
    {
        // Alteramos los ssbo para calcula en progresión las coordenadas de las partículas del ejemplo
        if (binding.first == "InputSSBO")
        {
            this->SetDescriptorWrite(descriptorWrites[idx], binding.second.binding, this->ssbo->uniformBuffers[(frameIdx - 1) % MAX_FRAMES_IN_FLIGHT], this->ssboSize, frameIdx);
            idx++;
        }
        else if (binding.first == "OutputSSBO")
        {
            this->SetDescriptorWrite(descriptorWrites[idx], binding.second.binding, this->ssbo->uniformBuffers[frameIdx], this->ssboSize, frameIdx);
            idx++;
        }

         if (binding.first == "InputImage")
         {
             this->inputImageInfo = {};

             this->inputImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
             this->inputImageInfo.imageView = inputTexture->imageView;
             this->inputImageInfo.sampler = inputTexture->textureSampler;

             descriptorWrites[idx] = {};
             descriptorWrites[idx].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
             descriptorWrites[idx].dstBinding = binding.second.binding;
             descriptorWrites[idx].dstArrayElement = 0;
             descriptorWrites[idx].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
             descriptorWrites[idx].descriptorCount = 1;
             descriptorWrites[idx].pBufferInfo = VK_NULL_HANDLE;
             descriptorWrites[idx].dstSet = descriptorSets[frameIdx];
             descriptorWrites[idx].pImageInfo = &inputImageInfo;

             idx++;
            }
         else if (binding.first == "OutputImage")
         {
             this->outputImageInfo = {};

             this->outputImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
             this->outputImageInfo.imageView = outputTexture->imageView;
             this->outputImageInfo.sampler = outputTexture->textureSampler;

             descriptorWrites[idx] = {};
             descriptorWrites[idx].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
             descriptorWrites[idx].dstBinding = binding.second.binding;
             descriptorWrites[idx].dstArrayElement = 0;
             descriptorWrites[idx].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
             descriptorWrites[idx].descriptorCount = 1;
             descriptorWrites[idx].pBufferInfo = VK_NULL_HANDLE;
             descriptorWrites[idx].dstSet = descriptorSets[frameIdx];
             descriptorWrites[idx].pImageInfo = &outputImageInfo;

             idx++;
            }
    }

    return descriptorWrites;
}

void ComputeDescriptorBuffer::InitializeDescriptorSets(std::shared_ptr<ShaderModule> shader_ptr)
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
