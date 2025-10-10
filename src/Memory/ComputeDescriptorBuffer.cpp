#include "ComputeDescriptorBuffer.h"
#include "SynchronizationModule.h"
#include "Timer.h"
#include <QESessionManager.h>

ComputeDescriptorBuffer::ComputeDescriptorBuffer()
{
	this->deviceModule = DeviceModule::getInstance();
}

ComputeDescriptorBuffer::ComputeDescriptorBuffer(std::shared_ptr<ShaderModule> shader_ptr) : ComputeDescriptorBuffer()
{
    this->numBinding = (uint32_t)shader_ptr->reflectShader.bindings.at(0).size();
    this->StartResources(shader_ptr);
}

void ComputeDescriptorBuffer::StartResources(std::shared_ptr<ShaderModule> shader_ptr)
{
    std::vector<VkDescriptorPoolSize> poolSizes;
    poolSizes.resize(this->numBinding);
    size_t idx = 0;

    for (int idSet = 0; idSet < shader_ptr->reflectShader.bindings.size(); idSet++)
    {
        for (auto binding : shader_ptr->reflectShader.bindings[idSet])
        {
            if (binding.first == "InputSSBO")
            {
                poolSizes[idx].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                poolSizes[idx].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
                this->_numSSBOs++;
                idx++;
            }
            else if (binding.first == "CameraUniform")
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
            else if (binding.first == "InputBoneSSBO")
            {
                poolSizes[idx].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                poolSizes[idx].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
                this->_numSSBOs++;
                idx++;
            }
            else if (binding.first == "OutputSSBO")
            {
                poolSizes[idx].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                poolSizes[idx].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
                this->_numSSBOs++;
                idx++;
            }
            else if (binding.first == "UniformDeltaTime")
            {
                poolSizes[idx].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                poolSizes[idx].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
                idx++;

                this->ubos["UniformDeltaTime"] = std::make_shared<UniformBufferObject>();
                this->uboSizes["UniformDeltaTime"] = sizeof(DeltaTimeUniform);
                this->ubos["UniformDeltaTime"]->CreateUniformBuffer(this->uboSizes["UniformDeltaTime"], MAX_FRAMES_IN_FLIGHT, *deviceModule);
                this->deltaTimeUniform = std::make_shared<DeltaTimeUniform>();
            }
            else if (binding.first == "InputImage")
            {
                poolSizes[idx].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                poolSizes[idx].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
                idx++;
            }
            else if (binding.first == "InputImage_2")
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
            else if (binding.first == "UniformAnimation")
            {
                poolSizes[idx].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                poolSizes[idx].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
                idx++;

                this->ubos["UniformAnimation"] = std::make_shared<UniformBufferObject>();
                this->uboSizes["UniformAnimation"] = sizeof(glm::mat4) * 200;
                this->ubos["UniformAnimation"]->CreateUniformBuffer(this->uboSizes["UniformAnimation"], MAX_FRAMES_IN_FLIGHT, *deviceModule);
            }
            else if (binding.first == "UniformVertexParam")
            {
                poolSizes[idx].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                poolSizes[idx].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
                idx++;

                this->ubos["UniformVertexParam"] = std::make_shared<UniformBufferObject>();
                this->uboSizes["UniformVertexParam"] = sizeof(int);
                this->ubos["UniformVertexParam"]->CreateUniformBuffer(this->uboSizes["UniformVertexParam"], MAX_FRAMES_IN_FLIGHT, *deviceModule);
            }
            else if (binding.first == "DeadParticlesSSBO")
            {
                poolSizes[idx].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                poolSizes[idx].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

                this->_numSSBOs++;
                idx++;
            }
            else if (binding.first == "UniformParticleSystem")
            {
                poolSizes[idx].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                poolSizes[idx].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
                idx++;

                this->ubos["UniformParticleSystem"] = std::make_shared<UniformBufferObject>();
                this->uboSizes["UniformParticleSystem"] = sizeof(ParticleSystemUniform);
                this->ubos["UniformParticleSystem"]->CreateUniformBuffer(this->uboSizes["UniformParticleSystem"], MAX_FRAMES_IN_FLIGHT, *deviceModule);
            }
            else if (binding.first == "UniformParticleTexture")
            {
                poolSizes[idx].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                poolSizes[idx].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
                idx++;

                this->ubos["UniformParticleTexture"] = std::make_shared<UniformBufferObject>();
                this->uboSizes["UniformParticleTexture"] = sizeof(ParticleTextureParamsUniform);
                this->ubos["UniformParticleTexture"]->CreateUniformBuffer(this->uboSizes["UniformParticleTexture"], MAX_FRAMES_IN_FLIGHT, *deviceModule);
            }
            else if (binding.first == "UniformNewParticles")
            {
                poolSizes[idx].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                poolSizes[idx].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
                idx++;

                this->ubos["UniformNewParticles"] = std::make_shared<UniformBufferObject>();
                this->uboSizes["UniformNewParticles"] = sizeof(NewParticleUniform);
                this->ubos["UniformNewParticles"]->CreateUniformBuffer(this->uboSizes["UniformNewParticles"], MAX_FRAMES_IN_FLIGHT, *deviceModule);
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

void ComputeDescriptorBuffer::InitializeSSBOData()
{
    if (this->IsProgressiveComputation)
    {
        this->ssboData.push_back(std::make_shared<UniformBufferObject>());
        this->ssboSize.push_back(VkDeviceSize());
    }
    else
    {
        for (uint32_t i = 0; i < this->_numSSBOs; i++)
        {
            this->ssboData.push_back(std::make_shared<UniformBufferObject>());
            this->ssboSize.push_back(VkDeviceSize());
        }
    }
}

void ComputeDescriptorBuffer::AssignSSBO(std::shared_ptr<UniformBufferObject> ssbo, VkDeviceSize size)
{
    this->ssboData.push_back(ssbo);
    this->ssboSize.push_back(size);
}

VkDescriptorBufferInfo ComputeDescriptorBuffer::GetBufferInfo(VkBuffer buffer, VkDeviceSize bufferSize)
{
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = bufferSize;
    return bufferInfo;
}

void ComputeDescriptorBuffer::SetDescriptorWrite(VkWriteDescriptorSet& descriptorWrite, VkDescriptorType descriptorType, uint32_t binding, VkBuffer buffer, VkDeviceSize bufferSize, uint32_t frameIdx)
{
    this->buffersInfo[binding] = this->GetBufferInfo(buffer, bufferSize);

    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSets[frameIdx];
    descriptorWrite.dstBinding = binding;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = descriptorType;
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

    for (int idSet = 0; idSet < shader_ptr->reflectShader.bindings.size(); idSet++)
    {
        for (auto binding : shader_ptr->reflectShader.bindings[idSet])
        {
            if (this->IsProgressiveComputation)
            {
                if (binding.first == "InputSSBO")
                {
                    // Alteramos los ssbo para calcular en progresión las coordenadas de las partículas
                    this->SetDescriptorWrite(descriptorWrites[idx], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, binding.second.binding, this->ssboData[0]->uniformBuffers[(frameIdx - 1) % MAX_FRAMES_IN_FLIGHT], this->ssboSize[0], frameIdx);
                    idx++;
                }
                else if (binding.first == "InputBoneSSBO")
                {
                    // Alteramos los ssbo para calcular en progresión las coordenadas de las partículas
                    this->SetDescriptorWrite(descriptorWrites[idx], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, binding.second.binding, this->ssboData[1]->uniformBuffers[(frameIdx - 1) % MAX_FRAMES_IN_FLIGHT], this->ssboSize[1], frameIdx);
                    idx++;
                }
                else if (binding.first == "OutputSSBO")
                {
                    this->SetDescriptorWrite(descriptorWrites[idx], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, binding.second.binding, this->ssboData[2]->uniformBuffers[frameIdx], this->ssboSize[2], frameIdx);
                    idx++;
                }
            }
            else
            {
                if (binding.first == "InputSSBO")
                {
                    this->SetDescriptorWrite(descriptorWrites[idx], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, binding.second.binding, this->ssboData[0]->uniformBuffers[frameIdx], this->ssboSize[0], frameIdx);
                    idx++;
                }
                else if (binding.first == "InputBoneSSBO")
                {
                    this->SetDescriptorWrite(descriptorWrites[idx], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, binding.second.binding, this->ssboData[1]->uniformBuffers[frameIdx], this->ssboSize[1], frameIdx);
                    idx++;
                }
                else if (binding.first == "OutputSSBO")
                {
                    this->SetDescriptorWrite(descriptorWrites[idx], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, binding.second.binding, this->ssboData[2]->uniformBuffers[frameIdx], this->ssboSize[2], frameIdx);
                    idx++;
                }
            }

            if (binding.first == "CameraUniform")
            {
                auto cameraUBO = QESessionManager::getInstance()->GetCameraUBO();
                this->SetDescriptorWrite(descriptorWrites[idx], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, binding.second.binding, cameraUBO->uniformBuffers[frameIdx], sizeof(CameraUniform), frameIdx);
                idx++;
            }

            if (binding.first == "SunUniform")
            {
                this->SetDescriptorWrite(descriptorWrites[idx], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, binding.second.binding, this->ubos["SunUniform"]->uniformBuffers[frameIdx], sizeof(SunUniform), frameIdx);
                idx++;
            }

            if (binding.first == "UniformDeltaTime")
            {
                this->SetDescriptorWrite(descriptorWrites[idx], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, binding.second.binding, this->ubos["UniformDeltaTime"]->uniformBuffers[frameIdx], this->uboSizes["UniformDeltaTime"], frameIdx);
                idx++;
            }
            if (binding.first == "UniformAnimation")
            {
                this->SetDescriptorWrite(descriptorWrites[idx], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, binding.second.binding, this->ubos["UniformAnimation"]->uniformBuffers[frameIdx], this->uboSizes["UniformAnimation"], frameIdx);
                idx++;
            }
            if (binding.first == "UniformVertexParam")
            {
                this->SetDescriptorWrite(descriptorWrites[idx], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, binding.second.binding, this->ubos["UniformVertexParam"]->uniformBuffers[frameIdx], this->uboSizes["UniformVertexParam"], frameIdx);
                idx++;
            }
            else if (binding.first == "DeadParticlesSSBO")
            {
                this->SetDescriptorWrite(descriptorWrites[idx], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, binding.second.binding, this->ssboData[1]->uniformBuffers[0], this->ssboSize[1], frameIdx);
                idx++;
            }
            else if (binding.first == "UniformParticleSystem")
            {
                this->SetDescriptorWrite(descriptorWrites[idx], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, binding.second.binding, this->ubos["UniformParticleSystem"]->uniformBuffers[frameIdx], this->uboSizes["UniformParticleSystem"], frameIdx);
                idx++;
            }
            else if (binding.first == "UniformParticleTexture")
            {
                this->SetDescriptorWrite(descriptorWrites[idx], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, binding.second.binding, this->ubos["UniformParticleTexture"]->uniformBuffers[frameIdx], this->uboSizes["UniformParticleTexture"], frameIdx);
                idx++;
            }
            else if (binding.first == "UniformNewParticles")
            {
                this->SetDescriptorWrite(descriptorWrites[idx], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, binding.second.binding, this->ubos["UniformNewParticles"]->uniformBuffers[frameIdx], this->uboSizes["UniformNewParticles"], frameIdx);
                idx++;
            }
            else if (binding.first == "InputImage")
            {
                this->inputImageInfo = {};

                this->inputImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                this->inputImageInfo.imageView = inputTextures.at(0)->imageView;
                this->inputImageInfo.sampler = inputTextures.at(0)->textureSampler;

                descriptorWrites[idx] = {};
                descriptorWrites[idx].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrites[idx].dstBinding = binding.second.binding;
                descriptorWrites[idx].dstArrayElement = 0;
                descriptorWrites[idx].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                descriptorWrites[idx].descriptorCount = 1;
                descriptorWrites[idx].pBufferInfo = VK_NULL_HANDLE;
                descriptorWrites[idx].dstSet = descriptorSets[frameIdx];
                descriptorWrites[idx].pImageInfo = &inputImageInfo;

                idx++;
            }
            else if (binding.first == "InputImage_2")
            {
                this->inputImageInfo_2 = {};

                this->inputImageInfo_2.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                this->inputImageInfo_2.imageView = inputTextures.at(1)->imageView;
                this->inputImageInfo_2.sampler = inputTextures.at(1)->textureSampler;

                descriptorWrites[idx] = {};
                descriptorWrites[idx].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrites[idx].dstBinding = binding.second.binding;
                descriptorWrites[idx].dstArrayElement = 0;
                descriptorWrites[idx].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                descriptorWrites[idx].descriptorCount = 1;
                descriptorWrites[idx].pBufferInfo = VK_NULL_HANDLE;
                descriptorWrites[idx].dstSet = descriptorSets[frameIdx];
                descriptorWrites[idx].pImageInfo = &inputImageInfo_2;

                idx++;
            }
            else if (binding.first == "OutputImage")
            {
                this->outputImageInfo = {};

                this->outputImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                this->outputImageInfo.imageView = outputTexture->imageView;
                this->outputImageInfo.sampler = outputTexture->textureSampler;

                descriptorWrites[idx] = {};
                descriptorWrites[idx].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrites[idx].dstBinding = binding.second.binding;
                descriptorWrites[idx].dstArrayElement = 0;
                descriptorWrites[idx].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                descriptorWrites[idx].descriptorCount = 1;
                descriptorWrites[idx].pBufferInfo = VK_NULL_HANDLE;
                descriptorWrites[idx].dstSet = descriptorSets[frameIdx];
                descriptorWrites[idx].pImageInfo = &outputImageInfo;

                idx++;
            }
        }
    }

    return descriptorWrites;
}

void ComputeDescriptorBuffer::InitializeDescriptorSets(std::shared_ptr<ShaderModule> shader_ptr)
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

void ComputeDescriptorBuffer::UpdateUBODeltaTime()
{
    if (this->deltaTimeUniform != nullptr)
    {
        auto currentFrame = SynchronizationModule::GetCurrentFrame();
        this->deltaTimeUniform->deltaTime = Timer::DeltaTime * 2000.0f;
        void* data;
        vkMapMemory(deviceModule->device, this->ubos["UniformDeltaTime"]->uniformBuffersMemory[currentFrame], 0, sizeof(DeltaTimeUniform), 0, &data);
        memcpy(data, static_cast<const void*>(this->deltaTimeUniform.get()), sizeof(DeltaTimeUniform));
        vkUnmapMemory(deviceModule->device, this->ubos["UniformDeltaTime"]->uniformBuffersMemory[currentFrame]);
    }
}

void ComputeDescriptorBuffer::Cleanup()
{
    if (!this->descriptorSets.empty())
    {
        this->descriptorSets.clear();
    }

    if (this->descriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(deviceModule->device, this->descriptorPool, nullptr);
        this->descriptorPool = VK_NULL_HANDLE;
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (!this->ssboData.empty())
        {
            for (uint32_t j = 0; j < this->ssboData.size(); j++)
            {
                if (this->ssboData[j]->uniformBuffers[i] != VK_NULL_HANDLE)
                {
                    vkDestroyBuffer(deviceModule->device, this->ssboData[j]->uniformBuffers[i], nullptr);
                    vkFreeMemory(deviceModule->device, this->ssboData[j]->uniformBuffersMemory[i], nullptr);
                    this->ssboData[j]->uniformBuffers[i] = VK_NULL_HANDLE;
                }
            }
        }

        for (auto ubo : ubos)
        {
            if (ubo.second->uniformBuffers[i] != VK_NULL_HANDLE)
            {
                vkDestroyBuffer(deviceModule->device, ubo.second->uniformBuffers[i], nullptr);
                vkFreeMemory(deviceModule->device, ubo.second->uniformBuffersMemory[i], nullptr);
                ubo.second->uniformBuffers[i] = VK_NULL_HANDLE;
            }
        }
    }

    this->ssboData.clear();

    if (!this->inputTextures.empty())
    {
        for (int i = 0; i < this->inputTextures.size(); i++)
        {
            this->inputTextures[i]->cleanup();
            this->inputTextures[i].reset();
            this->inputTextures[i] = nullptr;
        }
        inputTextures.clear();
    }

    if (this->outputTexture != nullptr)
    {
        this->outputTexture->cleanup();
        this->outputTexture.reset();
        this->outputTexture = nullptr;
    }

    this->deltaTimeUniform.reset();
    this->deltaTimeUniform = nullptr;

    this->buffersInfo.clear();
}
