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
    const uint32_t TARGET_SET = 0;

    auto itSet = shader_ptr->reflectShader.bindings.find(TARGET_SET);
    if (itSet == shader_ptr->reflectShader.bindings.end())
        throw std::runtime_error("ComputeDescriptorBuffer::StartResources: shader has no target descriptor set");

    const auto& setBindings = itSet->second; // map<bindingIndex, DescriptorBindingReflect>

    std::unordered_map<VkDescriptorType, uint32_t> typeCounts;
    typeCounts.reserve(16);

    this->_numSSBOs = 0;

    // Recorremos TODOS los bindings del set
    for (const auto& kv : setBindings)
    {
        const DescriptorBindingReflect& br = kv.second;
        if (br.set != TARGET_SET) continue;

        const uint32_t countPerSet = std::max(1u, br.arraySize);
        typeCounts[br.type] += countPerSet * uint32_t(MAX_FRAMES_IN_FLIGHT);

        if (br.name == "UniformDeltaTime")
        {
            if (this->ubos.find("UniformDeltaTime") == this->ubos.end())
                this->ubos["UniformDeltaTime"] = std::make_shared<UniformBufferObject>();

            this->uboSizes["UniformDeltaTime"] = sizeof(DeltaTimeUniform);
            this->ubos["UniformDeltaTime"]->CreateUniformBuffer(this->uboSizes["UniformDeltaTime"], MAX_FRAMES_IN_FLIGHT, *deviceModule);

            this->deltaTimeUniform = std::make_shared<DeltaTimeUniform>();
        }
        else if (br.name == "UniformVertexParam")
        {
            if (this->ubos.find("UniformVertexParam") == this->ubos.end())
                this->ubos["UniformVertexParam"] = std::make_shared<UniformBufferObject>();

            this->uboSizes["UniformVertexParam"] = sizeof(int);
            this->ubos["UniformVertexParam"]->CreateUniformBuffer(this->uboSizes["UniformVertexParam"], MAX_FRAMES_IN_FLIGHT, *deviceModule);
        }
        else if (br.name == "UniformParticleSystem")
        {
            if (this->ubos.find("UniformParticleSystem") == this->ubos.end())
                this->ubos["UniformParticleSystem"] = std::make_shared<UniformBufferObject>();

            this->uboSizes["UniformParticleSystem"] = sizeof(ParticleSystemUniform);
            this->ubos["UniformParticleSystem"]->CreateUniformBuffer(this->uboSizes["UniformParticleSystem"], MAX_FRAMES_IN_FLIGHT, *deviceModule);
        }
        else if (br.name == "UniformParticleTexture")
        {
            if (this->ubos.find("UniformParticleTexture") == this->ubos.end())
                this->ubos["UniformParticleTexture"] = std::make_shared<UniformBufferObject>();

            this->uboSizes["UniformParticleTexture"] = sizeof(ParticleTextureParamsUniform);
            this->ubos["UniformParticleTexture"]->CreateUniformBuffer(this->uboSizes["UniformParticleTexture"], MAX_FRAMES_IN_FLIGHT, *deviceModule);
        }
        else if (br.name == "UniformNewParticles")
        {
            if (this->ubos.find("UniformNewParticles") == this->ubos.end())
                this->ubos["UniformNewParticles"] = std::make_shared<UniformBufferObject>();

            this->uboSizes["UniformNewParticles"] = sizeof(NewParticleUniform);
            this->ubos["UniformNewParticles"]->CreateUniformBuffer(this->uboSizes["UniformNewParticles"], MAX_FRAMES_IN_FLIGHT, *deviceModule);
        }

        if (br.type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
            this->_numSSBOs++;
    }

    // Construir poolSizes
    std::vector<VkDescriptorPoolSize> poolSizes;
    poolSizes.reserve(typeCounts.size());
    for (auto& [type, count] : typeCounts)
    {
        if (count == 0) continue;
        poolSizes.push_back(VkDescriptorPoolSize{ type, count });
    }

    if (poolSizes.empty())
        throw std::runtime_error("ComputeDescriptorBuffer::StartResources: no descriptors found for pool");

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    if (vkCreateDescriptorPool(deviceModule->device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
        throw std::runtime_error("ComputeDescriptorBuffer::StartResources: failed to create descriptor pool");
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
    std::vector<VkWriteDescriptorSet> writes;
    writes.reserve(32);

    this->buffersInfo.clear();
    this->buffersInfo.reserve(32);

    const uint32_t TARGET_SET = 0;
    auto itSet = shader_ptr->reflectShader.bindings.find(TARGET_SET);
    if (itSet == shader_ptr->reflectShader.bindings.end())
        return writes;

    const auto& setBindings = itSet->second; // map<bindingIndex, DescriptorBindingReflect>

    auto pushBufferWrite = [&](VkDescriptorType type, uint32_t dstBinding, VkBuffer buffer, VkDeviceSize range)
        {
            VkDescriptorBufferInfo bi{};
            bi.buffer = buffer;
            bi.offset = 0;
            bi.range = range;
            this->buffersInfo.push_back(bi);

            VkWriteDescriptorSet w{};
            w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            w.dstSet = descriptorSets[frameIdx];
            w.dstBinding = dstBinding;
            w.dstArrayElement = 0;
            w.descriptorType = type;
            w.descriptorCount = 1;
            w.pBufferInfo = &this->buffersInfo.back();
            w.pImageInfo = nullptr;
            writes.push_back(w);
        };

    auto pushImageWrite = [&](uint32_t dstBinding, VkDescriptorImageInfo* ii)
        {
            VkWriteDescriptorSet w{};
            w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            w.dstSet = descriptorSets[frameIdx];
            w.dstBinding = dstBinding;
            w.dstArrayElement = 0;
            w.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            w.descriptorCount = 1;
            w.pBufferInfo = nullptr;
            w.pImageInfo = ii;
            writes.push_back(w);
        };

    const uint32_t prevFrame = (frameIdx + MAX_FRAMES_IN_FLIGHT - 1) % MAX_FRAMES_IN_FLIGHT;

    for (const auto& kv : setBindings)
    {
        const DescriptorBindingReflect& br = kv.second;
        const uint32_t b = br.binding;

        // SSBOs
        if (br.name == "InputSSBO")
        {
            VkBuffer buf = this->IsProgressiveComputation
                ? this->ssboData[0]->uniformBuffers[prevFrame]
                : this->ssboData[0]->uniformBuffers[frameIdx];

            pushBufferWrite(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, b, buf, this->ssboSize[0]);
            continue;
        }
        if (br.name == "InputBoneSSBO")
        {
            VkBuffer buf = this->IsProgressiveComputation
                ? this->ssboData[1]->uniformBuffers[prevFrame]
                : this->ssboData[1]->uniformBuffers[frameIdx];

            pushBufferWrite(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, b, buf, this->ssboSize[1]);
            continue;
        }
        if (br.name == "OutputSSBO")
        {
            pushBufferWrite(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, b,
                this->ssboData[2]->uniformBuffers[frameIdx], this->ssboSize[2]);
            continue;
        }

        // UBOs
        if (br.name == "UniformCamera")
        {
            auto cameraUBO = QESessionManager::getInstance()->GetCameraUBO();
            pushBufferWrite(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, b,
                cameraUBO->uniformBuffers[frameIdx], sizeof(UniformCamera));
            continue;
        }

        if (br.name == "SunUniform")
        {
            pushBufferWrite(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, b,
                this->ubos["SunUniform"]->uniformBuffers[frameIdx], sizeof(SunUniform));
            continue;
        }

        if (br.name == "UniformDeltaTime")
        {
            pushBufferWrite(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, b,
                this->ubos["UniformDeltaTime"]->uniformBuffers[frameIdx], this->uboSizes["UniformDeltaTime"]);
            continue;
        }

        if (br.name == "UniformAnimation")
        {
            pushBufferWrite(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, b,
                this->ssboData[3]->uniformBuffers[frameIdx], this->ssboSize[3]);
            continue;
        }

        if (br.name == "UniformVertexParam")
        {
            pushBufferWrite(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, b,
                this->ubos["UniformVertexParam"]->uniformBuffers[frameIdx], this->uboSizes["UniformVertexParam"]);
            continue;
        }

        if (br.name == "DeadParticlesSSBO")
        {
            pushBufferWrite(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, b,
                this->ssboData[1]->uniformBuffers[0], this->ssboSize[1]);
            continue;
        }

        if (br.name == "UniformParticleSystem")
        {
            pushBufferWrite(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, b,
                this->ubos["UniformParticleSystem"]->uniformBuffers[frameIdx], this->uboSizes["UniformParticleSystem"]);
            continue;
        }

        if (br.name == "UniformParticleTexture")
        {
            pushBufferWrite(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, b,
                this->ubos["UniformParticleTexture"]->uniformBuffers[frameIdx], this->uboSizes["UniformParticleTexture"]);
            continue;
        }

        if (br.name == "UniformNewParticles")
        {
            pushBufferWrite(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, b,
                this->ubos["UniformNewParticles"]->uniformBuffers[frameIdx], this->uboSizes["UniformNewParticles"]);
            continue;
        }

        // Storage Images
        if (br.name == "InputImage")
        {
            this->inputImageInfo = {};
            this->inputImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            this->inputImageInfo.imageView = inputTextures.at(0)->imageView;
            this->inputImageInfo.sampler = inputTextures.at(0)->textureSampler;

            pushImageWrite(b, &this->inputImageInfo);
            continue;
        }

        if (br.name == "InputImage_2")
        {
            this->inputImageInfo_2 = {};
            this->inputImageInfo_2.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            this->inputImageInfo_2.imageView = inputTextures.at(1)->imageView;
            this->inputImageInfo_2.sampler = inputTextures.at(1)->textureSampler;

            pushImageWrite(b, &this->inputImageInfo_2);
            continue;
        }

        if (br.name == "OutputImage")
        {
            this->outputImageInfo = {};
            this->outputImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            this->outputImageInfo.imageView = outputTexture->imageView;
            this->outputImageInfo.sampler = outputTexture->textureSampler;

            pushImageWrite(b, &this->outputImageInfo);
            continue;
        }
    }

    return writes;
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
