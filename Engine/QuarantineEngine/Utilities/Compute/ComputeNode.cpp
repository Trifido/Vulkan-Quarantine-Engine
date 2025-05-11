#include "ComputeNode.h"
#include "SynchronizationModule.h"
#include "BufferManageModule.h"

ComputeNode::ComputeNode()
{
    this->deviceModule = DeviceModule::getInstance();
    this->widthImage = this->heightImage = 0;
}

ComputeNode::ComputeNode(std::string computeShaderPath) : ComputeNode(std::make_shared<ShaderModule>(computeShaderPath))
{
}

ComputeNode::ComputeNode(std::shared_ptr<ShaderModule> shader_ptr) : ComputeNode()
{
    this->computeShader = shader_ptr;
    this->computeDescriptor = std::make_shared<ComputeDescriptorBuffer>(this->computeShader);
}

void ComputeNode::FillComputeBuffer(uint32_t numElements, unsigned long long elementType, void* data)
{
    this->NElements = numElements;

    VkDeviceSize bufferSize = elementType * this->NElements;

    // Create a staging buffer used to upload data to the gpu
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    BufferManageModule::createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingBufferMemory, *deviceModule);

    void* auxData;
    vkMapMemory(deviceModule->device, stagingBufferMemory, 0, bufferSize, 0, &auxData);
    memcpy(auxData, data, (size_t)bufferSize);
    vkUnmapMemory(deviceModule->device, stagingBufferMemory);

    // Initialize ssbo
    this->InitializeComputeBuffer(0,(uint32_t)bufferSize);

    // Fill ssbo
    this->computeDescriptor->ssboData[0]->FillSSBO(stagingBuffer, bufferSize, MAX_FRAMES_IN_FLIGHT, *deviceModule);

    vkDestroyBuffer(deviceModule->device, stagingBuffer, nullptr);
    vkFreeMemory(deviceModule->device, stagingBufferMemory, nullptr);
}

void ComputeNode::FillComputeBuffer(uint32_t ssboIndex, VkBuffer buffer, uint32_t bufferSize)
{
    this->computeDescriptor->ssboData[ssboIndex]->FillSSBO(buffer, bufferSize, MAX_FRAMES_IN_FLIGHT, *deviceModule);
}

void ComputeNode::InitializeComputeNode()
{
    this->computeDescriptor->InitializeDescriptorSets(this->computeShader);
}

void ComputeNode::InitializeOutputTextureComputeNode(uint32_t width, uint32_t height, VkFormat format)
{
    this->widthImage = width;
    this->heightImage = height;

    this->computeDescriptor->outputTexture = std::make_shared<CustomTexture>(
        width, height, format,
        VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        TEXTURE_TYPE::COMPUTE_TYPE
    );

    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.layerCount = 1;
    this->computeDescriptor->outputTexture->transitionImageLayout(this->computeDescriptor->outputTexture->image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, subresourceRange);
}

void ComputeNode::cleanup()
{
    if (this->computeDescriptor != nullptr)
    {
        this->computeDescriptor->Cleanup();
        this->computeDescriptor.reset();
        this->computeDescriptor = nullptr;
    }

    if (this->computeShader != nullptr)
    {
        this->computeShader.reset();
        this->computeShader = nullptr;
    }

    this->NElements = 0;
}

void ComputeNode::InitializeComputeBuffer(uint32_t idBuffer, uint32_t bufferSize)
{
    this->computeDescriptor->ssboData[idBuffer]->CreateSSBO(bufferSize, MAX_FRAMES_IN_FLIGHT, *deviceModule);
    this->computeDescriptor->ssboSize[idBuffer] = bufferSize;
}

void ComputeNode::DispatchCommandBuffer(VkCommandBuffer commandBuffer, uint32_t currentFrame)
{
    if (this->OnDemandCompute && !this->Compute)
    {
        this->UpdateOutputTextureState();
        return;
    }

    this->Compute = false;

    if (this->UseDependencyBuffer)
    {
        VkBufferMemoryBarrier bufferBarrier = {};
        bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        bufferBarrier.buffer = computeDescriptor->ssboData.at(0)->uniformBuffers[currentFrame];
        bufferBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        bufferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        bufferBarrier.size = VK_WHOLE_SIZE;
        bufferBarrier.pNext = NULL;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
            0, nullptr, 1, &bufferBarrier, 0, nullptr);
    }

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, this->computeShader->ComputePipelineModule->pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, this->computeShader->ComputePipelineModule->pipelineLayout, 0, 1, &computeDescriptor->descriptorSets[currentFrame], 0, 0);

    if (this->widthImage == 0 || this->heightImage == 0)
    {
        uint32_t groupX = static_cast<uint32_t>((this->NElements < 256) ? this->NElements : CEIL_DIV(this->NElements, 256.0f));
        vkCmdDispatch(commandBuffer, groupX, 1, 1);
    }
    else
    {
        this->UpdateOutputTextureState();

        vkCmdDispatch(commandBuffer, CEIL_DIV(this->widthImage, NElements), CEIL_DIV(this->heightImage, NElements), 1);
    }
}

void ComputeNode::UpdateComputeDescriptor()
{
    this->computeDescriptor->UpdateUBODeltaTime();
}

void ComputeNode::UpdateOutputTextureState()
{
    auto outputTexture = this->computeDescriptor->outputTexture;

    if (outputTexture == nullptr)
        return;

    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.layerCount = 1;

    if (outputTexture->currentLayout != VK_IMAGE_LAYOUT_GENERAL)
    {
        if (outputTexture->currentLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            outputTexture->transitionImageLayout(outputTexture->image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, subresourceRange);
        else
            outputTexture->transitionImageLayout(outputTexture->image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, subresourceRange);
    }
}
