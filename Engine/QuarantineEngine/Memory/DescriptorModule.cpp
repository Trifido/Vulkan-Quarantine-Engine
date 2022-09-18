#include "DescriptorModule.h"


DeviceModule* DescriptorModule::deviceModule;
uint32_t DescriptorModule::NumSwapchainImages;

DescriptorModule::DescriptorModule()
{
}

void DescriptorModule::createDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr; // Optional


    // ----------------- INICIO BUCLE CON TODAS LAS TEXTURAS
    std::vector<VkDescriptorSetLayoutBinding> samplerLayoutBinding;
    samplerLayoutBinding.resize(textures.size());

    for (size_t id = 0; id < textures.size(); id++)
    {
        samplerLayoutBinding[id].binding = id + 1;
        samplerLayoutBinding[id].descriptorCount = 1;
        samplerLayoutBinding[id].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding[id].pImmutableSamplers = nullptr;
        samplerLayoutBinding[id].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    
    // ----------------- FINAL BUCLE CON TODAS LAS TEXTURAS

    std::vector<VkDescriptorSetLayoutBinding> bindings = { uboLayoutBinding };
    bindings.insert(bindings.end(), samplerLayoutBinding.begin(), samplerLayoutBinding.end());

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(deviceModule->device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

void DescriptorModule::createDescriptorPool()
{
    VkDescriptorPoolSize textureSize = {};
    textureSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    textureSize.descriptorCount = static_cast<uint32_t>(this->NumSwapchainImages);

    std::vector<VkDescriptorPoolSize> poolSizes;
    poolSizes.resize(1 + textures.size(), textureSize);

    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(this->NumSwapchainImages);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(this->NumSwapchainImages);

    if (vkCreateDescriptorPool(deviceModule->device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void DescriptorModule::createDescriptorSets()
{
    std::vector<VkDescriptorSetLayout> layouts(this->NumSwapchainImages, descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(this->NumSwapchainImages);
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(this->NumSwapchainImages);
    if (vkAllocateDescriptorSets(deviceModule->device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    size_t numDescriptors = textures.size() + 1;

    //bufferModule->updateDescriptors(descriptorSets);
    for (size_t i = 0; i < this->NumSwapchainImages; i++)
    {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        std::vector<VkWriteDescriptorSet> descriptorWrites{};
        descriptorWrites.resize(numDescriptors);

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pImageInfo = VK_NULL_HANDLE;
        descriptorWrites[0].pBufferInfo = &bufferInfo;


        // ----------------- INICIO BUCLE CON TODAS LAS TEXTURAS
        for (size_t id = 1; id < numDescriptors; id++)
        {
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = textures[id - 1]->imageView;
            imageInfo.sampler = textures[id - 1]->textureSampler;

            descriptorWrites[id].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[id].dstSet = descriptorSets[i];
            descriptorWrites[id].dstBinding = id;
            descriptorWrites[id].dstArrayElement = 0;
            descriptorWrites[id].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[id].descriptorCount = 1;
            descriptorWrites[id].pBufferInfo = VK_NULL_HANDLE;
            descriptorWrites[id].pImageInfo = &imageInfo;
        }
        // ----------------- FINAL BUCLE CON TODAS LAS TEXTURAS


        vkUpdateDescriptorSets(deviceModule->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}

void DescriptorModule::cleanup()
{
    vkDestroyDescriptorSetLayout(deviceModule->device, descriptorSetLayout, nullptr);
}

void DescriptorModule::cleanupDescriptorPool()
{
    vkDestroyDescriptorPool(deviceModule->device, descriptorPool, nullptr);
}

void DescriptorModule::createUniformBuffers()
{
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    uniformBuffers.resize(this->NumSwapchainImages);
    uniformBuffersMemory.resize(this->NumSwapchainImages);

    for (size_t i = 0; i < this->NumSwapchainImages; i++) {
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

void DescriptorModule::Initialize(std::shared_ptr<std::map<TEXTURE_TYPE, std::shared_ptr<Texture>>> textures)
{
    this->InitializeTextureOrder(textures);

    this->createUniformBuffers();
    this->createDescriptorSetLayout();
    this->createDescriptorPool();
    this->createDescriptorSets();
}

void DescriptorModule::recreateUniformBuffer()
{
    this->createUniformBuffers();
    this->createDescriptorPool();
    this->createDescriptorSets();
}


void DescriptorModule::cleanupDescriptorBuffer()
{
    for (size_t i = 0; i < this->NumSwapchainImages; i++)
    {
        vkDestroyBuffer(deviceModule->device, uniformBuffers[i], nullptr);
        vkFreeMemory(deviceModule->device, uniformBuffersMemory[i], nullptr);
    }
}

void DescriptorModule::InitializeTextureOrder(std::shared_ptr<std::map<TEXTURE_TYPE, std::shared_ptr<Texture>>> textureMap)
{
    CheckTextures(textureMap, TEXTURE_TYPE::DIFFUSE_TYPE);
    CheckTextures(textureMap, TEXTURE_TYPE::NORMAL_TYPE);
    CheckTextures(textureMap, TEXTURE_TYPE::SPECULAR_TYPE);
    CheckTextures(textureMap, TEXTURE_TYPE::EMISSIVE_TYPE);
    CheckTextures(textureMap, TEXTURE_TYPE::BUMP_TYPE);
}

void DescriptorModule::CheckTextures(std::shared_ptr<std::map<TEXTURE_TYPE, std::shared_ptr<Texture>>> textureMap, TEXTURE_TYPE type)
{
    std::map<TEXTURE_TYPE, std::shared_ptr<Texture>>::iterator it = textureMap->find(type);
    if (it != textureMap->end())
        this->textures.push_back(it->second);
}
