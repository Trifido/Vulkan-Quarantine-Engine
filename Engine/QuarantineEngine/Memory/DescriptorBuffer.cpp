#include "DescriptorBuffer.h"
#include "SynchronizationModule.h"
#include <CameraEditor.h>

DescriptorBuffer::DescriptorBuffer()
{
    this->deviceModule = DeviceModule::getInstance();
}

DescriptorBuffer::DescriptorBuffer(std::shared_ptr<ShaderModule> shader_ptr) : DescriptorBuffer()
{
    this->shader = shader_ptr;

    //Check buffer layouts

    this->numBinding = this->shader->reflectShader.bindings.size();

    this->cameraUBO = std::make_shared<UniformBufferObject>();
    this->materialUBO = std::make_shared<UniformBufferObject>();
    this->lightUBO = std::make_shared<UniformBufferObject>();

    this->CheckResources();

    this->CreateDescriptorPool();
}

void DescriptorBuffer::CreateDescriptorPool()
{
    std::vector<VkDescriptorPoolSize> poolSizes;
    poolSizes.resize(this->numBinding);

    size_t idx = 0;
    while (idx < this->numUBOs)
    {
        poolSizes[idx].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[idx].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        idx++;
    }

    poolSizes[idx].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[idx].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

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

void DescriptorBuffer::CheckResources()
{
    for each (auto binding in this->shader->reflectShader.bindings)
    {
        if (binding.first == "CameraUniform")
        {
            this->camera = CameraEditor::getInstance();
        }
        else if (binding.first == "UniformManagerLight")
        {
            this->lightManager = LightManager::getInstance();
        }
        else if (binding.first == "UniformMaterial")
        {
            
        }
        else if (binding.first == "UniformAnimation")
        {

        }
        else if (binding.first == "Texture2DArray")
        {

        }
        
    }
}

void DescriptorBuffer::CreateDescriptorSets(const ShaderModule& shader)
{
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, shader.descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    //allocInfo.descriptorPool = descriptorLayout.descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(deviceModule->device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    size_t numDescriptors = this->numBinding;
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
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
