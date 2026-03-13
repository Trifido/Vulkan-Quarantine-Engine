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
    // Solo set 0 en esta clase (según tu código actual)
    const uint32_t TARGET_SET = 0;

    auto itSet = shader_ptr->reflectShader.bindings.find(TARGET_SET);
    if (itSet == shader_ptr->reflectShader.bindings.end())
        throw std::runtime_error("DescriptorBuffer::StartResources: shader has no set 0");

    const auto& setBindings = itSet->second; // map<bindingIndex, DescriptorBindingReflect>

    // 1) Acumular counts por VkDescriptorType para el pool (y nunca dejar counts=0)
    std::unordered_map<VkDescriptorType, uint32_t> typeCounts;
    typeCounts.reserve(16);

    this->swapChainModule = SwapChainModule::getInstance();
    this->deviceModule = DeviceModule::getInstance();
    this->lightManager = LightManager::getInstance();

    // 3) Recorremos todos los bindings del set 0
    for (const auto& kv : setBindings)
    {
        const DescriptorBindingReflect& br = kv.second;

        // Safety
        if (br.set != TARGET_SET)
            continue;

        const uint32_t countPerSet = std::max(1u, br.arraySize);

        // Pool count: por frame
        typeCounts[br.type] += countPerSet * uint32_t(MAX_FRAMES_IN_FLIGHT);

        if (br.name == "UniformAnimation")
        {
            // Si tu motor crea este UBO en esta clase:
            if (this->ubos.find("animationUBO") == this->ubos.end())
                this->ubos["animationUBO"] = std::make_shared<UniformBufferObject>();

            if (this->uboSizes.find("animationUBO") == this->uboSizes.end())
                this->uboSizes["animationUBO"] = VkDeviceSize(0);
        }
        else if (br.name == "UniformParticleTexture")
        {
            if (this->ubos.find("particleSystemUBO") == this->ubos.end())
                this->ubos["particleSystemUBO"] = std::make_shared<UniformBufferObject>();

            if (this->uboSizes.find("particleSystemUBO") == this->uboSizes.end())
                this->uboSizes["particleSystemUBO"] = VkDeviceSize(0);
        }
        else if (br.name == "ParticleSSBO")
        {
            this->ssboData["ParticleSSBO"] = std::make_shared<UniformBufferObject>();
            this->ssboSize["ParticleSSBO"] = VkDeviceSize(0);
            this->numSSBOs++;
        }
        else if (br.name == "Meshlets")
        {
            this->ssboData["Meshlets"] = std::make_shared<UniformBufferObject>();
            this->ssboSize["Meshlets"] = VkDeviceSize(0);
            this->numSSBOs++;
        }
        else if (br.name == "MeshletVertices")
        {
            this->ssboData["MeshletVertices"] = std::make_shared<UniformBufferObject>();
            this->ssboSize["MeshletVertices"] = VkDeviceSize(0);
            this->numSSBOs++;
        }
        else if (br.name == "IndexBuffer")
        {
            this->ssboData["IndexBuffer"] = std::make_shared<UniformBufferObject>();
            this->ssboSize["IndexBuffer"] = VkDeviceSize(0);
            this->numSSBOs++;
        }
        else if (br.name == "LightSSBO" || br.name == "LightIndices" || br.name == "ZBins" || br.name == "Tiles")
        {
            this->numSSBOs++;
        }
    }

    // 5) Convertimos typeCounts a VkDescriptorPoolSize (solo los que tengan count>0)
    std::vector<VkDescriptorPoolSize> poolSizes;
    poolSizes.reserve(typeCounts.size());

    for (const auto& tc : typeCounts)
    {
        if (tc.second == 0) continue;
        poolSizes.push_back(VkDescriptorPoolSize{ tc.first, tc.second });
    }

    if (poolSizes.empty())
        throw std::runtime_error("DescriptorBuffer::StartResources: poolSizes empty (no descriptors found?)");

    // 6) Crear pool
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();

    // Tu implementación actual solo aloca MAX_FRAMES_IN_FLIGHT descriptor sets (set 0)
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkResult res = vkCreateDescriptorPool(deviceModule->device, &poolInfo, nullptr, &descriptorPool);
    if (res != VK_SUCCESS)
        throw std::runtime_error("DescriptorBuffer::StartResources: failed to create descriptor pool");
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
    std::vector<VkWriteDescriptorSet> writes;
    writes.reserve(32);

    this->buffersInfo.clear();
    this->buffersInfo.reserve(32);

    this->imageInfo.clear();
    this->imageInfo.reserve(64);

    const uint32_t TARGET_SET = 0;
    auto itSet = shader_ptr->reflectShader.bindings.find(TARGET_SET);
    if (itSet == shader_ptr->reflectShader.bindings.end())
        return writes;

    const auto& setBindings = itSet->second;

    auto pushUBO = [&](uint32_t dstBinding, VkBuffer buffer, VkDeviceSize range)
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
            w.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            w.descriptorCount = 1;
            w.pBufferInfo = &this->buffersInfo.back();
            writes.push_back(w);
        };

    auto pushSSBO = [&](uint32_t dstBinding, VkBuffer buffer, VkDeviceSize range)
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
            w.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            w.descriptorCount = 1;
            w.pBufferInfo = &this->buffersInfo.back();
            writes.push_back(w);
        };

    auto pushCombinedImageSamplerArray = [&](uint32_t dstBinding, uint32_t count, uint32_t texBaseIndex)
        {
            // reservamos un segmento en imageInfo
            const size_t start = this->imageInfo.size();
            this->imageInfo.resize(start + count);

            for (uint32_t i = 0; i < count; ++i)
            {
                const auto& tex = textures->at(texBaseIndex + i);
                VkDescriptorImageInfo ii{};
                ii.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                ii.imageView = tex->imageView;
                ii.sampler = tex->textureSampler;
                this->imageInfo[start + i] = ii;
            }

            VkWriteDescriptorSet w{};
            w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            w.dstSet = descriptorSets[frameIdx];
            w.dstBinding = dstBinding;
            w.dstArrayElement = 0;
            w.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            w.descriptorCount = count;
            w.pImageInfo = &this->imageInfo[start];
            writes.push_back(w);
        };

    // Recorremos bindings del set0
    for (const auto& kv : setBindings)
    {
        const DescriptorBindingReflect& br = kv.second;
        const uint32_t dstBinding = br.binding;

        // Cámara
        if (br.name == "UniformCamera")
        {
            auto cameraUBO = QESessionManager::getInstance()->GetCameraUBO();
            pushUBO(dstBinding, cameraUBO->uniformBuffers[frameIdx], sizeof(UniformCamera));
        }
        // Pantalla
        else if (br.name == "ScreenData")
        {
            pushUBO(dstBinding, this->swapChainModule->screenData->uniformBuffers[frameIdx], sizeof(glm::uvec2));
        }
        // Light manager (int numLights)
        else if (br.name == "UniformManagerLight")
        {
            pushUBO(dstBinding, this->lightManager->lightUBO->uniformBuffers[frameIdx], sizeof(LightManagerUniform));
        }
        // Material UBO
        else if (br.name == "UniformMaterial")
        {
            pushUBO(dstBinding, this->ubos["materialUBO"]->uniformBuffers[frameIdx], this->uboSizes["materialUBO"]);
        }
        // Animación UBO
        else if (br.name == "UniformAnimation")
        {
            pushUBO(dstBinding, this->ubos["animationUBO"]->uniformBuffers[frameIdx], this->uboSizes["animationUBO"]);
        }
        // Partículas UBO
        else if (br.name == "UniformParticleTexture")
        {
            pushUBO(dstBinding, this->ubos["particleSystemUBO"]->uniformBuffers[frameIdx], this->uboSizes["particleSystemUBO"]);
        }

        // SSBOs de luces (si los tienes en set0: LightSSBO, LightIndices, ZBins, Tiles)
        else if (br.name == "LightSSBO")
        {
            pushSSBO(dstBinding, this->lightManager->lightSSBO->uniformBuffers[frameIdx], this->lightManager->lightSSBOSize);
        }
        else if (br.name == "LightIndices")
        {
            pushSSBO(dstBinding, this->lightManager->lightIndexSSBO->uniformBuffers[frameIdx], this->lightManager->lightIndexSSBOSize);
        }
        else if (br.name == "ZBins")
        {
            pushSSBO(dstBinding, this->lightManager->lightBinSSBO->uniformBuffers[frameIdx], this->lightManager->lightBinSSBOSize);
        }
        else if (br.name == "Tiles")
        {
            pushSSBO(dstBinding, this->lightManager->lightTilesSSBO->uniformBuffers[frameIdx], this->lightManager->lightTilesSSBOSize);
        }

        // Meshlets SSBOs (si están en set0)
        else if (br.name == "Meshlets")
        {
            pushSSBO(dstBinding, this->ssboData["Meshlets"]->uniformBuffers[frameIdx], this->ssboSize["Meshlets"]);
        }
        else if (br.name == "MeshletVertices")
        {
            pushSSBO(dstBinding, this->ssboData["MeshletVertices"]->uniformBuffers[frameIdx], this->ssboSize["MeshletVertices"]);
        }
        else if (br.name == "IndexBuffer")
        {
            pushSSBO(dstBinding, this->ssboData["IndexBuffer"]->uniformBuffers[frameIdx], this->ssboSize["IndexBuffer"]);
        }
        else if (br.name == "ParticleSSBO")
        {
            pushSSBO(dstBinding, this->ssboData["ParticleSSBO"]->uniformBuffers[frameIdx], this->ssboSize["ParticleSSBO"]);
        }

        // Texturas:
        // layout(set = 0, binding = 7) uniform sampler2D texSampler[5];
        else if (br.name == "texSampler" || br.name == "Texture2DArray")
        {
            const uint32_t count = std::max(1u, br.arraySize);
            if (!textures || textures->size() < count)
                throw std::runtime_error("DescriptorBuffer::GetDescriptorWrites: not enough textures for texSampler[]");

            pushCombinedImageSamplerArray(dstBinding, count, /*texBaseIndex*/0);
        }

        else if (br.name == "Texture1")
        {
            if (!textures || textures->empty())
                throw std::runtime_error("DescriptorBuffer::GetDescriptorWrites: textures empty for Texture1");

            pushCombinedImageSamplerArray(dstBinding, 1, 0);
        }
    }

    return writes;
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
            if (i < this->ubos["particleSystemUBO"]->uniformBuffers.size() && this->ubos["particleSystemUBO"]->uniformBuffers[i] != VK_NULL_HANDLE)
                vkDestroyBuffer(deviceModule->device, this->ubos["particleSystemUBO"]->uniformBuffers[i], nullptr);

            if (i < this->ubos["particleSystemUBO"]->uniformBuffersMemory.size() && this->ubos["particleSystemUBO"]->uniformBuffersMemory[i] != VK_NULL_HANDLE)
                vkFreeMemory(deviceModule->device, this->ubos["particleSystemUBO"]->uniformBuffersMemory[i], nullptr);
        }
    }

    if (this->ubos["particleSystemUBO"] != nullptr)
        this->ubos["particleSystemUBO"] = nullptr;

    this->ssboData.clear();
    this->buffersInfo.clear();
}
