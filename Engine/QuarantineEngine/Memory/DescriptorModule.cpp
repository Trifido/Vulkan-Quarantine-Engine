#include "DescriptorModule.h"

DescriptorModule::DescriptorModule(DeviceModule& deviceModule)
{
    this->deviceModule = &deviceModule;
}

void DescriptorModule::createDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(deviceModule->device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

void DescriptorModule::createDescriptorPool(size_t numSwapchainImgs)
{
    descriptorCount = numSwapchainImgs;
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(descriptorCount);
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(descriptorCount);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(descriptorCount);

    if (vkCreateDescriptorPool(deviceModule->device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void DescriptorModule::createDescriptorSets()
{
    std::vector<VkDescriptorSetLayout> layouts(descriptorCount, descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(descriptorCount);
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(descriptorCount);
    if (vkAllocateDescriptorSets(deviceModule->device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    //bufferModule->updateDescriptors(descriptorSets);
    for (size_t i = 0; i < descriptorCount; i++)
    {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = ptrTexture->imageView;
        imageInfo.sampler = ptrTexture->textureSampler;

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pImageInfo = VK_NULL_HANDLE;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pBufferInfo = VK_NULL_HANDLE;
        descriptorWrites[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(deviceModule->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}

void DescriptorModule::addPtrData(Texture& texModule)
{
    ptrTexture = std::make_shared<Texture>(texModule);
}

void DescriptorModule::cleanup()
{
    vkDestroyDescriptorSetLayout(deviceModule->device, descriptorSetLayout, nullptr);
}

void DescriptorModule::cleanupDescriptorPool()
{
    vkDestroyDescriptorPool(deviceModule->device, descriptorPool, nullptr);
}

void DescriptorModule::createUniformBuffers(size_t numImagesSwapChain)
{
    numSwapchainImages = numImagesSwapChain;
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    uniformBuffers.resize(numSwapchainImages);
    uniformBuffersMemory.resize(numSwapchainImages);

    for (size_t i = 0; i < numSwapchainImages; i++) {
        BufferManageModule::createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i], *deviceModule);
    }
}

void DescriptorModule::updateUniformBuffer(/*uint32_t currentImage, */VkExtent2D extent, glm::mat4& VPMainCamera, std::shared_ptr<Transform> transform)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    //transform->updateMVP(time, VPMainCamera, extent.width / (float)extent.height);

    //void* data;
    //vkMapMemory(deviceModule->device, uniformBuffersMemory[currentImage], 0, sizeof(transform->getMVP()), 0, &data);
    //memcpy(data, &transform->getMVP(), sizeof(transform->getMVP()));
    //vkUnmapMemory(deviceModule->device, uniformBuffersMemory[currentImage]);
}

void DescriptorModule::init(uint32_t numSwapChain, Texture& texModule)
{
    createUniformBuffers(numSwapChain);
    addPtrData(texModule);
    createDescriptorSetLayout();
    createDescriptorPool(numSwapChain);
    createDescriptorSets();
}

void DescriptorModule::recreateUniformBuffer(uint32_t numSwapChain)
{
    createUniformBuffers(numSwapChain);
    createDescriptorPool(numSwapChain);
    createDescriptorSets();
}

void DescriptorModule::cleanupDescriptorBuffer()
{
    for (size_t i = 0; i < numSwapchainImages; i++)
    {
        vkDestroyBuffer(deviceModule->device, uniformBuffers[i], nullptr);
        vkFreeMemory(deviceModule->device, uniformBuffersMemory[i], nullptr);
    }
}
