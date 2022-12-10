#include "DescriptorModule.h"

DeviceModule* DescriptorModule::deviceModule;
uint32_t DescriptorModule::NumSwapchainImages;

DescriptorModule::DescriptorModule()
{
    this->cameraUBO = std::make_shared<UniformBufferObject>();
    this->materialUBO = std::make_shared<UniformBufferObject>();
    this->lightUBO = std::make_shared<UniformBufferObject>();
}

void DescriptorModule::createDescriptorSetLayout()
{
    std::vector<VkDescriptorSetLayoutBinding> uboLayoutBinding{};
    uboLayoutBinding.resize(this->numUBOs);

    // Inicializamos los descriptor layouts de los UBO's
    for (size_t i = 0; i < this->numUBOs; i++)
    {
        uboLayoutBinding[i].binding = this->numBinding;
        uboLayoutBinding[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding[i].descriptorCount = 1;
        uboLayoutBinding[i].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        uboLayoutBinding[i].pImmutableSamplers = nullptr; // Optional
        this->numBinding++;
    }

    // Inicializamos el descriptor layouts de las texturas que será un array texture
    VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
    samplerLayoutBinding.binding = this->numBinding;
    samplerLayoutBinding.descriptorCount = textures->size();
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    this->numBinding++;
    
    // Formamos el layout total
    std::vector<VkDescriptorSetLayoutBinding> bindings = { uboLayoutBinding };
    bindings.push_back(samplerLayoutBinding);

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
    std::vector<VkDescriptorPoolSize> poolSizes;
    poolSizes.resize(this->numBinding);

    size_t idx = 0;
    while (idx < this->numUBOs)
    {
        poolSizes[idx].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[idx].descriptorCount = static_cast<uint32_t>(this->NumSwapchainImages);
        idx++;
    }

    poolSizes[idx].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[idx].descriptorCount = static_cast<uint32_t>(this->NumSwapchainImages);

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

    size_t numDescriptors = this->numBinding;
    for (size_t i = 0; i < this->NumSwapchainImages; i++)
    {
        uint32_t idx = 0;
        std::vector<VkWriteDescriptorSet> descriptorWrites{};
        descriptorWrites.resize(numDescriptors);

        if (this->cameraUniform != nullptr)
        {
            VkDescriptorBufferInfo bufferCameraInfo{};
            bufferCameraInfo.buffer = this->cameraUBO->uniformBuffers[i];
            bufferCameraInfo.offset = 0;
            bufferCameraInfo.range = sizeof(CameraUniform);

            descriptorWrites[idx].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[idx].dstSet = descriptorSets[i];
            descriptorWrites[idx].dstBinding = idx;
            descriptorWrites[idx].dstArrayElement = 0;
            descriptorWrites[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[idx].descriptorCount = 1;
            descriptorWrites[idx].pImageInfo = VK_NULL_HANDLE;
            descriptorWrites[idx].pBufferInfo = &bufferCameraInfo;

            idx++;
        }

        if (this->materialUniform != nullptr)
        {
            VkDescriptorBufferInfo bufferMaterialInfo{};
            bufferMaterialInfo.buffer = this->materialUBO->uniformBuffers[i];
            bufferMaterialInfo.offset = 0;
            bufferMaterialInfo.range = sizeof(MaterialUniform);

            descriptorWrites[idx].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[idx].dstSet = descriptorSets[i];
            descriptorWrites[idx].dstBinding = idx;
            descriptorWrites[idx].dstArrayElement = 0;
            descriptorWrites[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[idx].descriptorCount = 1;
            descriptorWrites[idx].pImageInfo = VK_NULL_HANDLE;
            descriptorWrites[idx].pBufferInfo = &bufferMaterialInfo;

            idx++;
        }

        if (this->lightUniform != nullptr)
        {
            VkDescriptorBufferInfo lightMaterialInfo{};
            lightMaterialInfo.buffer = this->lightUBO->uniformBuffers[i];
            lightMaterialInfo.offset = 0;
            lightMaterialInfo.range = sizeof(LightManagerUniform);

            descriptorWrites[idx].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[idx].dstSet = descriptorSets[i];
            descriptorWrites[idx].dstBinding = idx;
            descriptorWrites[idx].dstArrayElement = 0;
            descriptorWrites[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[idx].descriptorCount = 1;
            descriptorWrites[idx].pImageInfo = VK_NULL_HANDLE;
            descriptorWrites[idx].pBufferInfo = &lightMaterialInfo;

            idx++;
        }

        if (this->animationUniform != nullptr && this->hasAnimationProperties)
        {
            VkDescriptorBufferInfo bufferAnimationInfo{};
            bufferAnimationInfo.buffer = this->animationUBO->uniformBuffers[i];
            bufferAnimationInfo.offset = 0;
            bufferAnimationInfo.range = sizeof(AnimationUniform);

            descriptorWrites[idx].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[idx].dstSet = descriptorSets[i];
            descriptorWrites[idx].dstBinding = idx;
            descriptorWrites[idx].dstArrayElement = 0;
            descriptorWrites[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[idx].descriptorCount = 1;
            descriptorWrites[idx].pImageInfo = VK_NULL_HANDLE;
            descriptorWrites[idx].pBufferInfo = &bufferAnimationInfo;

            idx++;
        }


        // ----------------- INICIO BUCLE CON TODAS LAS TEXTURAS
        std::vector<VkDescriptorImageInfo> imageInfo;
        imageInfo.resize(textures->size());
        for (uint32_t id = 0; id < textures->size(); ++id)
        {
            imageInfo[id].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo[id].imageView = textures->at(id)->imageView;
            imageInfo[id].sampler = textures->at(id)->textureSampler;
        }

        for (size_t id = this->numUBOs; id < numDescriptors; id++)
        {
            descriptorWrites[id] = {};
            descriptorWrites[id].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[id].dstBinding = id;
            descriptorWrites[id].dstArrayElement = 0;
            descriptorWrites[id].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[id].descriptorCount = textures->size();
            descriptorWrites[id].pBufferInfo = VK_NULL_HANDLE;
            descriptorWrites[id].dstSet = descriptorSets[i];
            descriptorWrites[id].pImageInfo = imageInfo.data();
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
    // Camera UBO
    if (this->cameraUniform != nullptr)
    {
        this->cameraUBO->CreateUniformBuffer(sizeof(CameraUniform), this->NumSwapchainImages, *deviceModule);
        this->numUBOs++;
    }

    // Material UBO
    if (this->materialUniform != nullptr)
    {
        this->materialUBO->CreateUniformBuffer(sizeof(MaterialUniform), this->NumSwapchainImages, *deviceModule);
        this->numUBOs++;
    }

    // Light UBO
    if (this->lightUniform != nullptr)
    {
        this->lightUBO->CreateUniformBuffer(sizeof(LightManagerUniform), this->NumSwapchainImages, *deviceModule);
        this->numUBOs++;
    }

    // Animation UBO
    if (this->animationUBO != nullptr && this->hasAnimationProperties)
    {
        this->animationUBO->CreateUniformBuffer(sizeof(AnimationUniform), this->NumSwapchainImages, *deviceModule);
        this->numUBOs++;
    }
}

void DescriptorModule::updateUniforms(uint32_t currentImage)
{
    //static auto startTime = std::chrono::high_resolution_clock::now();

    //auto currentTime = std::chrono::high_resolution_clock::now();
    //float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    //transform->updateMVP(time, VPMainCamera, extent.width / (float)extent.height);

    void* data;
    vkMapMemory(deviceModule->device, this->cameraUBO->uniformBuffersMemory[currentImage], 0, sizeof(CameraUniform), 0, &data);
    memcpy(data, static_cast<const void*>(this->cameraUniform.get()), sizeof(CameraUniform));
    vkUnmapMemory(deviceModule->device, this->cameraUBO->uniformBuffersMemory[currentImage]);

    //Material
    vkMapMemory(deviceModule->device, this->materialUBO->uniformBuffersMemory[currentImage], 0, sizeof(MaterialUniform), 0, &data);
    memcpy(data, static_cast<const void*>(this->materialUniform.get()), sizeof(MaterialUniform));
    vkUnmapMemory(deviceModule->device, this->materialUBO->uniformBuffersMemory[currentImage]);

    //Light
    vkMapMemory(deviceModule->device, this->lightUBO->uniformBuffersMemory[currentImage], 0, sizeof(LightManagerUniform), 0, &data);
    memcpy(data, static_cast<const void*>(this->lightUniform.get()), sizeof(LightManagerUniform));
    vkUnmapMemory(deviceModule->device, this->lightUBO->uniformBuffersMemory[currentImage]);

    //Animation
    if (this->hasAnimationProperties)
    {
        vkMapMemory(deviceModule->device, this->animationUBO->uniformBuffersMemory[currentImage], 0, sizeof(AnimationUniform), 0, &data);
        memcpy(data, static_cast<const void*>(this->animationUniform.get()), sizeof(AnimationUniform));
        vkUnmapMemory(deviceModule->device, this->animationUBO->uniformBuffersMemory[currentImage]);
    }
}

void DescriptorModule::Initialize(std::shared_ptr <std::vector<std::shared_ptr<CustomTexture>>> textures, std::shared_ptr <MaterialUniform> uniformMaterial)
{
    this->textures = textures;
    this->materialUniform = uniformMaterial;

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

    for (size_t i = 0; i < this->NumSwapchainImages; i++)
    {
        void* data;

        //Camera
        vkMapMemory(deviceModule->device, this->cameraUBO->uniformBuffersMemory[i], 0, sizeof(CameraUniform), 0, &data);
        memcpy(data, static_cast<const void*>(this->cameraUniform.get()), sizeof(CameraUniform));
        vkUnmapMemory(deviceModule->device, this->cameraUBO->uniformBuffersMemory[i]);

        //Material
        vkMapMemory(deviceModule->device, this->materialUBO->uniformBuffersMemory[i], 0, sizeof(MaterialUniform), 0, &data);
        memcpy(data, static_cast<const void*>(this->materialUniform.get()), sizeof(MaterialUniform));
        vkUnmapMemory(deviceModule->device, this->materialUBO->uniformBuffersMemory[i]);

        //Light
        vkMapMemory(deviceModule->device, this->lightUBO->uniformBuffersMemory[i], 0, sizeof(LightManagerUniform), 0, &data);
        memcpy(data, static_cast<const void*>(this->lightUniform.get()), sizeof(LightManagerUniform));
        vkUnmapMemory(deviceModule->device, this->lightUBO->uniformBuffersMemory[i]);

        //Animation
        if (this->hasAnimationProperties)
        {
            vkMapMemory(deviceModule->device, this->animationUBO->uniformBuffersMemory[i], 0, sizeof(AnimationUniform), 0, &data);
            memcpy(data, static_cast<const void*>(this->animationUniform.get()), sizeof(AnimationUniform));
            vkUnmapMemory(deviceModule->device, this->animationUBO->uniformBuffersMemory[i]);
        }
    }
}

void DescriptorModule::cleanupDescriptorBuffer()
{
    for (size_t i = 0; i < this->NumSwapchainImages; i++)
    {
        // Camera UBO
        if (this->cameraUniform != nullptr)
        {
            vkDestroyBuffer(deviceModule->device, this->cameraUBO->uniformBuffers[i], nullptr);
            vkFreeMemory(deviceModule->device, this->cameraUBO->uniformBuffersMemory[i], nullptr);
        }

        // Material UBO
        if (this->materialUniform != nullptr)
        {
            vkDestroyBuffer(deviceModule->device, this->materialUBO->uniformBuffers[i], nullptr);
            vkFreeMemory(deviceModule->device, this->materialUBO->uniformBuffersMemory[i], nullptr);
        }

        // Light UBO
        if (this->lightUniform != nullptr)
        {
            vkDestroyBuffer(deviceModule->device, this->lightUBO->uniformBuffers[i], nullptr);
            vkFreeMemory(deviceModule->device, this->lightUBO->uniformBuffersMemory[i], nullptr);
        }

        // Animation UBO
        if (this->animationUBO != nullptr && this->hasAnimationProperties)
        {
            vkDestroyBuffer(deviceModule->device, this->animationUBO->uniformBuffers[i], nullptr);
            vkFreeMemory(deviceModule->device, this->animationUBO->uniformBuffersMemory[i], nullptr);
        }
    }
    this->numUBOs = 0;
}

void DescriptorModule::InitializeAnimationProperties()
{
    this->hasAnimationProperties = true;
    this->animationUBO = std::make_shared<UniformBufferObject>();
}

