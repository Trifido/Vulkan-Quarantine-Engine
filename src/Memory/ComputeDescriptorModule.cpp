#include "ComputeDescriptorModule.h"
#include "SynchronizationModule.h"
#include "BufferManageModule.h"

DeviceModule* ComputeDescriptorModule::deviceModule;
uint32_t ComputeDescriptorModule::NumSwapchainImages;

void ComputeDescriptorModule::initializeDescriptor(std::shared_ptr<std::vector<VkBuffer>> shaderStorageBuffers, long long unsigned storageSize)
{
    this->shaderStorageBuffers = shaderStorageBuffers;
    this->storageSize = storageSize;
    this->deviceModule = DeviceModule::getInstance();
    //this->createUniformBuffers();
    this->createComputeDescriptorSetLayout();
    this->createDescriptorPool();
    this->createComputeDescriptorSets();
}

void ComputeDescriptorModule::createComputeDescriptorSetLayout()
{
    std::array<VkDescriptorSetLayoutBinding, 2> layoutBindings{};
    layoutBindings[0].binding = 0;
    layoutBindings[0].descriptorCount = 1;
    layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    layoutBindings[0].pImmutableSamplers = nullptr;
    layoutBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    layoutBindings[1].binding = 1;
    layoutBindings[1].descriptorCount = 1;
    layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    layoutBindings[1].pImmutableSamplers = nullptr;
    layoutBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 2;
    layoutInfo.pBindings = layoutBindings.data();

    if (vkCreateDescriptorSetLayout(this->deviceModule->device, &layoutInfo, nullptr, &computeDescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create compute descriptor set layout!");
    }
}

void ComputeDescriptorModule::createDescriptorPool()
{
    std::array<VkDescriptorPoolSize, 1> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    if (vkCreateDescriptorPool(deviceModule->device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void ComputeDescriptorModule::createComputeDescriptorSets()
{
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, computeDescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    computeDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(deviceModule->device, &allocInfo, computeDescriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo storageBufferInfoLastFrame{};
        storageBufferInfoLastFrame.buffer = shaderStorageBuffers->at((i - 1) % MAX_FRAMES_IN_FLIGHT);
        storageBufferInfoLastFrame.offset = 0;
        storageBufferInfoLastFrame.range = this->storageSize;

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = computeDescriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &storageBufferInfoLastFrame;

        VkDescriptorBufferInfo storageBufferInfoCurrentFrame{};
        storageBufferInfoCurrentFrame.buffer = shaderStorageBuffers->at(i);
        storageBufferInfoCurrentFrame.offset = 0;
        storageBufferInfoCurrentFrame.range = this->storageSize;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = computeDescriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pBufferInfo = &storageBufferInfoCurrentFrame;

        vkUpdateDescriptorSets(deviceModule->device, 2, descriptorWrites.data(), 0, nullptr);
    }
}

//void ComputeDescriptorModule::createUniformBuffers()
//{
//    VkDeviceSize bufferSize = sizeof(UniformBufferComputeObject);
//
//    uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
//    uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
//    uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);
//
//    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
//    {
//        BufferManageModule::createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i], *deviceModule);
//
//        vkMapMemory(this->deviceModule->device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
//    }
//}

void ComputeDescriptorModule::cleanup()
{
    vkDestroyDescriptorSetLayout(deviceModule->device, computeDescriptorSetLayout, nullptr);
}
