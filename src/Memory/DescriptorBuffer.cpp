#include "DescriptorBuffer.h"
#include "SynchronizationModule.h"
#include <QESessionManager.h>

DescriptorBuffer::DescriptorBuffer()
{
    this->deviceModule = DeviceModule::getInstance();
}

DescriptorBuffer::DescriptorBuffer(std::shared_ptr<ShaderModule> shader_ptr) : DescriptorBuffer()
{
    for (int idSet = 0; idSet < shader_ptr->reflectShader.bindings.size(); idSet++)
    {
        for (auto binding : shader_ptr->reflectShader.bindings[idSet])
        {
            if (binding.second.set != 0)
                continue;
            this->numBinding++;
        }
    }

    //this->numBinding = size;

    this->ubos["materialUBO"] = std::make_shared<UniformBufferObject>();
    this->StartResources(shader_ptr);
}

void DescriptorBuffer::SetMeshletBuffers(std::shared_ptr<Meshlet> meshlets_ptr)
{
    this->meshlets_ptr = meshlets_ptr;

    this->ssboData["Meshlets"]->CreateSSBO(sizeof(MeshletDescriptor) * meshlets_ptr->gpuMeshlets.size(), MAX_FRAMES_IN_FLIGHT, *deviceModule);
    this->ssboSize["Meshlets"] = sizeof(MeshletDescriptor) * meshlets_ptr->gpuMeshlets.size();

    this->ssboData["MeshletVertices"]->CreateSSBO(sizeof(Vertex) * meshlets_ptr->verticesData.size(), MAX_FRAMES_IN_FLIGHT, *deviceModule);
    this->ssboSize["MeshletVertices"] = sizeof(Vertex) * meshlets_ptr->verticesData.size();

    this->ssboData["IndexBuffer"]->CreateSSBO(sizeof(uint32_t) * meshlets_ptr->indexData.size(), MAX_FRAMES_IN_FLIGHT, *deviceModule);
    this->ssboSize["IndexBuffer"] = sizeof(uint32_t) * meshlets_ptr->indexData.size();

    for (int currentFrame = 0; currentFrame < MAX_FRAMES_IN_FLIGHT; currentFrame++)
    {
        void* data;
        vkMapMemory(deviceModule->device, this->ssboData["Meshlets"]->uniformBuffersMemory[currentFrame], 0, this->ssboSize["Meshlets"], 0, &data);
        memcpy(data, this->meshlets_ptr->gpuMeshlets.data(), this->ssboSize["Meshlets"]);
        vkUnmapMemory(deviceModule->device, this->ssboData["Meshlets"]->uniformBuffersMemory[currentFrame]);

        vkMapMemory(deviceModule->device, this->ssboData["MeshletVertices"]->uniformBuffersMemory[currentFrame], 0, this->ssboSize["MeshletVertices"], 0, &data);
        memcpy(data, this->meshlets_ptr->verticesData.data(), this->ssboSize["MeshletVertices"]);
        vkUnmapMemory(deviceModule->device, this->ssboData["MeshletVertices"]->uniformBuffersMemory[currentFrame]);

        vkMapMemory(deviceModule->device, this->ssboData["IndexBuffer"]->uniformBuffersMemory[currentFrame], 0, this->ssboSize["IndexBuffer"], 0, &data);
        memcpy(data, this->meshlets_ptr->indexData.data(), this->ssboSize["IndexBuffer"]);
        vkUnmapMemory(deviceModule->device, this->ssboData["IndexBuffer"]->uniformBuffersMemory[currentFrame]);
    }
}

void DescriptorBuffer::CleanLastResources()
{
    this->deviceModule = nullptr;
    this->lightManager = nullptr;
    this->textures.reset();
    this->textures = nullptr;

    auto it = this->ubos.begin();
    while(it != this->ubos.end())
    {
        it->second.reset();
        it->second = nullptr;

        it++;
    }

    it = this->ssboData.begin();
    while (it != this->ssboData.end())
    {
        it->second.reset();
        it->second = nullptr;

        it++;
    }

    this->ubos.clear();
    this->uboSizes.clear();
}

void DescriptorBuffer::StartResources(std::shared_ptr<ShaderModule> shader_ptr)
{
    std::vector<VkDescriptorPoolSize> poolSizes;
    poolSizes.resize(this->numBinding);
    size_t idx = 0;

    for (int idSet = 0; idSet < shader_ptr->reflectShader.bindings.size(); idSet++)
    {
        for (auto binding : shader_ptr->reflectShader.bindings[idSet])
        {
            if (binding.second.set != 0)
                continue;

            if (binding.first == "CameraUniform")
            {
                poolSizes[idx].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                poolSizes[idx].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
                idx++;
            }
            else if (binding.first == "SunUniform")
            {
                poolSizes[idx].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                poolSizes[idx].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
                idx++;
            }
            else if (binding.first == "LightCameraUniform")
            {
                poolSizes[idx].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                poolSizes[idx].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
                idx++;
            }
            else if (binding.first == "PointLightCameraUniform")
            {
                poolSizes[idx].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                poolSizes[idx].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
                idx++;
            }
            else if (binding.first == "ScreenData")
            {
                this->swapChainModule = SwapChainModule::getInstance();
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
            else if (binding.first == "UniformParticleTexture")
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
            else if (binding.first == "Texture1")
            {
                poolSizes[idx].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                poolSizes[idx].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
                idx++;
            }
            else if (binding.first == "ParticleSSBO")
            {
                poolSizes[idx].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                poolSizes[idx].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

                this->ssboData["ParticleSSBO"] = std::make_shared<UniformBufferObject>();
                this->ssboSize["ParticleSSBO"] = VkDeviceSize();

                this->numSSBOs++;
                idx++;
            }
            else if (binding.first == "LightSSBO")
            {
                poolSizes[idx].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                poolSizes[idx].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

                this->ssboData["LightSSBO"] = std::make_shared<UniformBufferObject>();
                this->ssboSize["LightSSBO"] = VkDeviceSize();

                this->numSSBOs++;
                idx++;
            }
            else if (binding.first == "LightIndices")
            {
                poolSizes[idx].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                poolSizes[idx].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

                this->ssboData["LightIndices"] = std::make_shared<UniformBufferObject>();
                this->ssboSize["LightIndices"] = VkDeviceSize();

                this->numSSBOs++;
                idx++;
            }
            else if (binding.first == "ZBins")
            {
                poolSizes[idx].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                poolSizes[idx].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

                this->ssboData["ZBins"] = std::make_shared<UniformBufferObject>();
                this->ssboSize["ZBins"] = VkDeviceSize();

                this->numSSBOs++;
                idx++;
            }
            else if (binding.first == "Tiles")
            {
                poolSizes[idx].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                poolSizes[idx].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

                this->ssboData["Tiles"] = std::make_shared<UniformBufferObject>();
                this->ssboSize["Tiles"] = VkDeviceSize();

                this->numSSBOs++;
                idx++;
            }
            else if (binding.first == "Meshlets")
            {
                poolSizes[idx].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                poolSizes[idx].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

                this->ssboData["Meshlets"] = std::make_shared<UniformBufferObject>();
                this->ssboSize["Meshlets"] = VkDeviceSize();

                this->numSSBOs++;
                idx++;
            }
            else if (binding.first == "MeshletVertices")
            {
                poolSizes[idx].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                poolSizes[idx].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

                this->ssboData["MeshletVertices"] = std::make_shared<UniformBufferObject>();
                this->ssboSize["MeshletVertices"] = VkDeviceSize();

                this->numSSBOs++;
                idx++;
            }
            else if (binding.first == "IndexBuffer")
            {
                poolSizes[idx].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                poolSizes[idx].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

                this->ssboData["IndexBuffer"] = std::make_shared<UniformBufferObject>();
                this->ssboSize["IndexBuffer"] = VkDeviceSize();

                this->numSSBOs++;
                idx++;
            }
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

void DescriptorBuffer::SetDescriptorWrite(VkWriteDescriptorSet& descriptorWrite, VkDescriptorType descriptorType, uint32_t binding, VkBuffer buffer, VkDeviceSize bufferSize, uint32_t frameIdx)
{
    this->buffersInfo[binding] = GetBufferInfo(buffer, bufferSize);

    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSets[frameIdx];
    descriptorWrite.dstBinding = binding;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = descriptorType;
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

    if (!shader_ptr->reflectShader.bindings[0].empty())
    {
        for (auto binding : shader_ptr->reflectShader.bindings[0])
        {
            if (binding.first == "CameraUniform")
            {
                auto cameraUBO = QESessionManager::getInstance()->GetCameraUBO();
                this->SetDescriptorWrite(descriptorWrites[idx], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, binding.second.binding, cameraUBO->uniformBuffers[frameIdx], sizeof(CameraUniform), frameIdx);
                idx++;
            }
            else if (binding.first == "ScreenData")
            {
                this->SetDescriptorWrite(descriptorWrites[idx], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, binding.second.binding, this->swapChainModule->screenData->uniformBuffers[frameIdx], sizeof(glm::vec2), frameIdx);
                idx++;
            }
            else if (binding.first == "UniformManagerLight")
            {
                this->SetDescriptorWrite(descriptorWrites[idx], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, binding.second.binding, this->lightManager->lightUBO->uniformBuffers[frameIdx], sizeof(LightManagerUniform), frameIdx);
                idx++;
            }
            else if (binding.first == "UniformMaterial")
            {
                this->SetDescriptorWrite(descriptorWrites[idx], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, binding.second.binding, this->ubos["materialUBO"]->uniformBuffers[frameIdx], this->uboSizes["materialUBO"], frameIdx);
                idx++;
            }
            else if (binding.first == "UniformAnimation")
            {
                this->SetDescriptorWrite(descriptorWrites[idx], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, binding.second.binding, this->ubos["animationUBO"]->uniformBuffers[frameIdx], this->uboSizes["animationUBO"], frameIdx);
                idx++;
            }
            else if (binding.first == "UniformParticleTexture")
            {
                this->SetDescriptorWrite(descriptorWrites[idx], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, binding.second.binding, this->ubos["particleSystemUBO"]->uniformBuffers[frameIdx], this->uboSizes["particleSystemUBO"], frameIdx);
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
                descriptorWrites[idx].descriptorCount = (uint32_t)textures->size();
                descriptorWrites[idx].pBufferInfo = VK_NULL_HANDLE;
                descriptorWrites[idx].dstSet = descriptorSets[frameIdx];
                descriptorWrites[idx].pImageInfo = this->imageInfo.data();

                idx++;
            }
            else if (binding.first == "Texture1")
            {
                this->imageInfo.resize(1);
                this->imageInfo[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                this->imageInfo[0].imageView = textures->at(0)->imageView;
                this->imageInfo[0].sampler = textures->at(0)->textureSampler;

                descriptorWrites[idx] = {};
                descriptorWrites[idx].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrites[idx].dstBinding = binding.second.binding;
                descriptorWrites[idx].dstArrayElement = 0;
                descriptorWrites[idx].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descriptorWrites[idx].descriptorCount = 1;
                descriptorWrites[idx].pBufferInfo = VK_NULL_HANDLE;
                descriptorWrites[idx].dstSet = descriptorSets[frameIdx];
                descriptorWrites[idx].pImageInfo = this->imageInfo.data();

                idx++;
            }
            else if (binding.first == "ParticleSSBO")
            {
                this->SetDescriptorWrite(descriptorWrites[idx], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, binding.second.binding, this->ssboData["ParticleSSBO"]->uniformBuffers[frameIdx], this->ssboSize["ParticleSSBO"], frameIdx);
                idx++;
            }
            else if (binding.first == "LightSSBO")
            {
                this->SetDescriptorWrite(descriptorWrites[idx], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, binding.second.binding, this->lightManager->lightSSBO->uniformBuffers[frameIdx], this->lightManager->lightSSBOSize, frameIdx);
                idx++;
            }
            else if (binding.first == "LightIndices")
            {
                this->SetDescriptorWrite(descriptorWrites[idx], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, binding.second.binding, this->lightManager->lightIndexSSBO->uniformBuffers[frameIdx], this->lightManager->lightIndexSSBOSize, frameIdx);
                idx++;
            }
            else if (binding.first == "ZBins")
            {
                this->SetDescriptorWrite(descriptorWrites[idx], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, binding.second.binding, this->lightManager->lightBinSSBO->uniformBuffers[frameIdx], this->lightManager->lightBinSSBOSize, frameIdx);
                idx++;
            }
            else if (binding.first == "Tiles")
            {
                this->SetDescriptorWrite(descriptorWrites[idx], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, binding.second.binding, this->lightManager->lightTilesSSBO->uniformBuffers[frameIdx], this->lightManager->lightTilesSSBOSize, frameIdx);
                idx++;
            }
            else if (binding.first == "Meshlets")
            {
                this->SetDescriptorWrite(descriptorWrites[idx], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, binding.second.binding, this->ssboData["Meshlets"]->uniformBuffers[frameIdx], this->ssboSize["Meshlets"], frameIdx);
                idx++;
            }
            else if (binding.first == "MeshletVertices")
            {
                this->SetDescriptorWrite(descriptorWrites[idx], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, binding.second.binding, this->ssboData["MeshletVertices"]->uniformBuffers[frameIdx], this->ssboSize["MeshletVertices"], frameIdx);
                idx++;
            }
            else if (binding.first == "IndexBuffer")
            {
                this->SetDescriptorWrite(descriptorWrites[idx], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, binding.second.binding, this->ssboData["IndexBuffer"]->uniformBuffers[frameIdx], this->ssboSize["IndexBuffer"], frameIdx);
                idx++;
            }
        }
    }

    return descriptorWrites;
}

void DescriptorBuffer::InitializeDescriptorSets(std::shared_ptr<ShaderModule> shader_ptr)
{
    std::vector<VkDescriptorSetLayout> layouts;
    for (uint32_t i = 0; i < shader_ptr->descriptorSetLayouts.size(); i++)
    {
        for (uint32_t f = 0; f < MAX_FRAMES_IN_FLIGHT; f++)
        {
            layouts.push_back(shader_ptr->descriptorSetLayouts.at(i));
        }
    }

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = this->descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(deviceModule->device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        std::vector<VkWriteDescriptorSet> descriptorWrites{};
        descriptorWrites = this->GetDescriptorWrites(shader_ptr, (uint32_t)i);
        vkUpdateDescriptorSets(deviceModule->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}

void DescriptorBuffer::CleanDescriptorSetPool()
{
    vkDestroyDescriptorPool(deviceModule->device, this->descriptorPool, nullptr);
    this->descriptorSets.clear();
}

void DescriptorBuffer::Cleanup()
{
    this->CleanDescriptorSetPool();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (!this->ssboData.empty())
        {
            auto it = this->ssboData.begin();
            while(it != this->ssboData.end())
            {
                if (!it->second->uniformBuffers.empty())
                {
                    vkDestroyBuffer(deviceModule->device, it->second->uniformBuffers[i], nullptr);
                    vkFreeMemory(deviceModule->device, it->second->uniformBuffersMemory[i], nullptr);
                    it->second->uniformBuffers[i] = VK_NULL_HANDLE;
                }

                it++;
            }
        }

        if (this->ubos["particleSystemUBO"] != nullptr)
        {
            vkDestroyBuffer(deviceModule->device, this->ubos["particleSystemUBO"]->uniformBuffers[i], nullptr);
            vkFreeMemory(deviceModule->device, this->ubos["particleSystemUBO"]->uniformBuffersMemory[i], nullptr);
        }
    }

    if (this->ubos["particleSystemUBO"] != nullptr)
        this->ubos["particleSystemUBO"] = nullptr;

    this->ssboData.clear();
    this->buffersInfo.clear();
}
