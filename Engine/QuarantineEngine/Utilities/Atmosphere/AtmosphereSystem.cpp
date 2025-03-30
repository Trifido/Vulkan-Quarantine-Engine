#include "AtmosphereSystem.h"
#include <ShaderManager.h>
#include <MaterialManager.h>
#include <filesystem>
#include <PrimitiveMesh.h>
#include <SynchronizationModule.h>

AtmosphereSystem::AtmosphereSystem()
{
    this->environmentType = ENVIRONMENT_TYPE::PHYSICALLY_BASED_SKY;
    this->deviceModule = DeviceModule::getInstance();
    this->lightManager = LightManager::getInstance();
    this->swapChainModule = SwapChainModule::getInstance();

    this->Sun.Direction = glm::normalize(glm::vec3(0.0, -0.1, 0.1));
    this->Sun.Intensity = 100.0f;
}

AtmosphereSystem::~AtmosphereSystem()
{
    this->deviceModule = nullptr;
}

void AtmosphereSystem::LoadAtmosphereDto(AtmosphereDto atmosphereDto, Camera* cameraPtr)
{
    this->Sun.Direction = glm::normalize(atmosphereDto.sunDirection);
    this->Sun.Intensity = atmosphereDto.sunIntensity;
    this->environmentType = static_cast<ENVIRONMENT_TYPE>(atmosphereDto.environmentType);
    this->IsInitialized = atmosphereDto.hasAtmosphere;

    switch (this->environmentType)
    {
    case ENVIRONMENT_TYPE::CUBEMAP:
        this->InitializeAtmosphere(this->environmentType, nullptr, 0, cameraPtr);
        break;
    case ENVIRONMENT_TYPE::SPHERICALMAP:
        this->InitializeAtmosphere(this->environmentType, nullptr, 0, cameraPtr);
        break;
    default:
    case ENVIRONMENT_TYPE::PHYSICALLY_BASED_SKY:
        this->InitializeAtmosphere(cameraPtr);
        break;
    }
}

AtmosphereDto AtmosphereSystem::CreateAtmosphereDto()
{
    return
    {
        this->IsInitialized, this->environmentType, this->Sun.Direction, this->Sun.Intensity
    };
}

void AtmosphereSystem::AddTextureResources(const string* texturePaths, uint32_t numTextures)
{
    TEXTURE_TYPE textureType = (this->environmentType == ENVIRONMENT_TYPE::CUBEMAP) ? TEXTURE_TYPE::CUBEMAP_TYPE : TEXTURE_TYPE::DIFFUSE_TYPE;

    vector<string> resultPaths;
    for (size_t i = 0; i < numTextures; i++)
    {
        resultPaths.push_back(this->GetAbsolutePath("../../resources/textures", texturePaths[i]));
    }

    switch (numTextures)
    {
    default:
    case 0:
        break;
    case 1:
        this->environmentTexture = std::make_shared<CustomTexture>(resultPaths[0], textureType);
        break;
    case 6:
        this->environmentTexture = std::make_shared<CustomTexture>(resultPaths);
        break;
    }
    this->CreateDescriptorSet();
}

void AtmosphereSystem::InitializeAtmosphere(Camera* cameraPtr)
{
    this->SetUpResources(cameraPtr);
    this->UpdateSun();
    this->CreateDescriptorPool();
    this->CreateDescriptorSet();
}

void AtmosphereSystem::SetUpResources(Camera* cameraPtr)
{
    this->Cleanup();
    this->CleanLastResources();

    this->camera = cameraPtr;

    auto shaderManager = ShaderManager::getInstance();

    const string absolute_sky_vert_shader_path = this->GetAbsolutePath("../../resources/shaders", this->shaderPaths[this->environmentType]);
    const string absolute_sky_frag_shader_path = this->GetAbsolutePath("../../resources/shaders", this->shaderPaths[this->environmentType + (shaderPaths.size() * 0.5f)]);

    GraphicsPipelineData pipelineData = {};
    pipelineData.HasVertexData = this->environmentType != ENVIRONMENT_TYPE::PHYSICALLY_BASED_SKY;

    this->environment_shader = std::make_shared<ShaderModule>(
        ShaderModule(absolute_sky_vert_shader_path, absolute_sky_frag_shader_path, pipelineData)
    );
    shaderManager->AddShader("environment_map", this->environment_shader);

    if (this->environmentType == ENVIRONMENT_TYPE::PHYSICALLY_BASED_SKY)
    {
        this->resolutionUBO = std::make_shared<UniformBufferObject>();
        this->resolutionUBO->CreateUniformBuffer(sizeof(ScreenResolutionUniform), MAX_FRAMES_IN_FLIGHT, *deviceModule);

        this->sunUBO = std::make_shared<UniformBufferObject>();
        this->sunUBO->CreateUniformBuffer(sizeof(SunUniform), MAX_FRAMES_IN_FLIGHT, *deviceModule);

        this->UpdateAtmopshereResolution();
        this->computeNodeManager = ComputeNodeManager::getInstance();

        this->TLUT_ComputeNode = std::make_shared<ComputeNode>(shaderManager->GetShader("transmittance_lut"));
        this->TLUT_ComputeNode->OnDemandCompute = true;
        this->TLUT_ComputeNode->Compute = true;
        this->TLUT_ComputeNode->NElements = 16;
        this->TLUT_ComputeNode->InitializeOutputTextureComputeNode(256, 64, VK_FORMAT_R32G32B32A32_SFLOAT);
        this->computeNodeManager->AddComputeNode("transmittance_lut", this->TLUT_ComputeNode);

        this->MSLUT_ComputeNode = std::make_shared<ComputeNode>(shaderManager->GetShader("multi_scattering_lut"));
        this->MSLUT_ComputeNode->OnDemandCompute = true;
        this->MSLUT_ComputeNode->Compute = true;
        this->MSLUT_ComputeNode->NElements = 16;
        this->MSLUT_ComputeNode->InitializeOutputTextureComputeNode(32, 32, VK_FORMAT_R32G32B32A32_SFLOAT);
        this->MSLUT_ComputeNode->computeDescriptor->inputTextures.push_back(this->TLUT_ComputeNode->computeDescriptor->outputTexture);
        this->computeNodeManager->AddComputeNode("multi_scattering_lut", this->MSLUT_ComputeNode);

        this->SVLUT_ComputeNode = std::make_shared<ComputeNode>(shaderManager->GetShader("sky_view_lut"));
        this->SVLUT_ComputeNode->OnDemandCompute = true;
        this->SVLUT_ComputeNode->Compute = true;
        this->SVLUT_ComputeNode->NElements = 16;
        this->SVLUT_ComputeNode->InitializeOutputTextureComputeNode(640.0, 360, VK_FORMAT_R32G32B32A32_SFLOAT);
        this->SVLUT_ComputeNode->computeDescriptor->inputTextures.push_back(this->TLUT_ComputeNode->computeDescriptor->outputTexture);
        this->SVLUT_ComputeNode->computeDescriptor->inputTextures.push_back(this->MSLUT_ComputeNode->computeDescriptor->outputTexture);
        this->SVLUT_ComputeNode->computeDescriptor->ubos["SunUniform"] = this->sunUBO;
        this->computeNodeManager->AddComputeNode("sky_view_lut", this->SVLUT_ComputeNode);
    }

    switch (this->environmentType)
    {
    default:
    case ENVIRONMENT_TYPE::PHYSICALLY_BASED_SKY:
        this->_Mesh = std::make_shared<PrimitiveMesh>(PRIMITIVE_TYPE::QUAD_TYPE);
        break;
    case ENVIRONMENT_TYPE::CUBEMAP:
        this->_Mesh = std::make_shared<PrimitiveMesh>(PRIMITIVE_TYPE::CUBE_TYPE);
        break;
    case ENVIRONMENT_TYPE::SPHERICALMAP:
        this->_Mesh = std::make_shared<PrimitiveMesh>(PRIMITIVE_TYPE::SPHERE_TYPE);
        break;
    }

    this->_Mesh->InitializeMesh();
}

void AtmosphereSystem::InitializeAtmosphere(ENVIRONMENT_TYPE type, const string* texturePaths, uint32_t numTextures, Camera* cameraPtr)
{
    if (this->environmentType == type) return;

    this->environmentType = type;

    this->SetUpResources(cameraPtr);

    this->CreateDescriptorPool();

    this->AddTextureResources(texturePaths, numTextures);
}

void AtmosphereSystem::SetCamera(Camera* cameraPtr)
{
    this->camera = cameraPtr;
}

void AtmosphereSystem::CreateDescriptorPool()
{
    std::vector<VkDescriptorPoolSize> poolSizes;

    int numPool = (this->environmentType != ENVIRONMENT_TYPE::PHYSICALLY_BASED_SKY) ? 2 : 1;
    poolSizes.resize(numPool);
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = 1;

    if (this->environmentType != ENVIRONMENT_TYPE::PHYSICALLY_BASED_SKY)
    {
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = 1;
    }

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    if (vkCreateDescriptorPool(this->deviceModule->device, &poolInfo, nullptr, &this->descriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void AtmosphereSystem::CreateDescriptorSet()
{
    std::vector<VkDescriptorSetLayout> layouts;
    for (uint32_t i = 0; i < this->environment_shader->descriptorSetLayouts.size(); i++)
    {
        for (uint32_t f = 0; f < MAX_FRAMES_IN_FLIGHT; f++)
        {
            layouts.push_back(this->environment_shader->descriptorSetLayouts.at(i));
        }
    }

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = this->descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    this->descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(this->deviceModule->device, &allocInfo, descriptorSets.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    int numWrites = (this->environmentType != ENVIRONMENT_TYPE::PHYSICALLY_BASED_SKY) ? 2 : 5;
    int numBuffers = (this->environmentType != ENVIRONMENT_TYPE::PHYSICALLY_BASED_SKY) ? 1 : 3;
    this->buffersInfo.resize(numBuffers);

    for (size_t frameIdx = 0; frameIdx < MAX_FRAMES_IN_FLIGHT; frameIdx++)
    {
        std::vector<VkWriteDescriptorSet> descriptorWrites;
        descriptorWrites.resize(numWrites);


        if (this->environmentType != ENVIRONMENT_TYPE::PHYSICALLY_BASED_SKY)
        {
            this->SetDescriptorWrite(descriptorWrites[0], this->descriptorSets[frameIdx], 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, this->camera->cameraUBO->uniformBuffers[frameIdx], sizeof(CameraUniform));
            this->SetSamplerDescriptorWrite(descriptorWrites[1], this->descriptorSets[frameIdx], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, this->environmentTexture, this->imageInfo_1);
        }
        else
        {
            this->SetSamplerDescriptorWrite(descriptorWrites[0], this->descriptorSets[frameIdx], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, this->TLUT_ComputeNode->computeDescriptor->outputTexture, this->imageInfo_1);
            this->SetSamplerDescriptorWrite(descriptorWrites[1], this->descriptorSets[frameIdx], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, this->SVLUT_ComputeNode->computeDescriptor->outputTexture, this->imageInfo_2);
            this->SetDescriptorWrite(descriptorWrites[2], this->descriptorSets[frameIdx], 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, this->camera->cameraUBO->uniformBuffers[frameIdx], sizeof(CameraUniform));
            this->SetDescriptorWrite(descriptorWrites[3], this->descriptorSets[frameIdx], 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3, this->resolutionUBO->uniformBuffers[frameIdx], sizeof(ScreenResolutionUniform));
            this->SetDescriptorWrite(descriptorWrites[4], this->descriptorSets[frameIdx], 2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4, this->sunUBO->uniformBuffers[frameIdx], sizeof(SunUniform));
        }

        vkUpdateDescriptorSets(deviceModule->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}

VkDescriptorBufferInfo AtmosphereSystem::GetBufferInfo(VkBuffer buffer, VkDeviceSize bufferSize)
{
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = bufferSize;
    return bufferInfo;
}

void AtmosphereSystem::SetDescriptorWrite(VkWriteDescriptorSet& descriptorWrite, VkDescriptorSet descriptorSet, uint32_t idBuffer, VkDescriptorType descriptorType, uint32_t binding, VkBuffer buffer, VkDeviceSize bufferSize)
{
    this->buffersInfo[idBuffer] = GetBufferInfo(buffer, bufferSize);

    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = binding;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = descriptorType;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = VK_NULL_HANDLE;
    descriptorWrite.pBufferInfo = &this->buffersInfo[idBuffer];
}

void AtmosphereSystem::SetSamplerDescriptorWrite(VkWriteDescriptorSet& descriptorWrite, VkDescriptorSet descriptorSet, VkDescriptorType descriptorType, uint32_t binding, std::shared_ptr<CustomTexture> texture, VkDescriptorImageInfo& imageInfo)
{
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = texture->imageView; // ImageView del cubemap
    imageInfo.sampler = texture->textureSampler; // Sampler para el cubemap

    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = binding;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = descriptorType;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;
}

string AtmosphereSystem::GetAbsolutePath(string relativePath, string filename)
{
    auto absPath = std::filesystem::absolute(relativePath).generic_string();

    std::string substring = "/Engine";
    std::size_t ind = absPath.find(substring);

    if (ind != std::string::npos) {
        absPath.erase(ind, substring.length());
    }

    return absPath + "/" + filename;
}

void AtmosphereSystem::DrawCommand(VkCommandBuffer& commandBuffer, uint32_t frameIdx)
{
    if (!this->IsInitialized)
        return;

    if (this->TLUT_ComputeNode != nullptr && this->SVLUT_ComputeNode != nullptr)
    {
        auto textureTLUT = this->TLUT_ComputeNode->computeDescriptor->outputTexture;
        auto textureSVLUT = this->SVLUT_ComputeNode->computeDescriptor->outputTexture;

        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = 1;
        subresourceRange.layerCount = 1;

        if (textureTLUT->currentLayout == VK_IMAGE_LAYOUT_GENERAL)
        {
            textureTLUT->transitionImageLayout(textureTLUT->image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange);
        }
        if (textureSVLUT->currentLayout == VK_IMAGE_LAYOUT_GENERAL)
        {
            textureSVLUT->transitionImageLayout(textureSVLUT->image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange);
        }
    }

    auto pipelineModule = this->environment_shader->PipelineModule;
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineModule->pipeline);

    vkCmdSetDepthTestEnable(commandBuffer, false);
    vkCmdSetDepthWriteEnable(commandBuffer, false);

    vkCmdSetCullMode(commandBuffer, VK_CULL_MODE_NONE);
    vkCmdSetFrontFace(commandBuffer, VK_FRONT_FACE_COUNTER_CLOCKWISE);

    if (!this->descriptorSets.empty())
    {
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->environment_shader->PipelineModule->pipelineLayout, 0, 1, &descriptorSets.at(frameIdx), 0, nullptr);
    }

    if (this->environmentType != ENVIRONMENT_TYPE::PHYSICALLY_BASED_SKY)
    {
        VkDeviceSize offsets[] = { 0 };
        VkBuffer vertexBuffers[] = { this->_Mesh->vertexBuffer };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, this->_Mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(this->_Mesh->indices.size()), 1, 0, 0, 0);
    }
    else
    {
        vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    }
}

void AtmosphereSystem::Cleanup()
{
    if (_Mesh != nullptr)
    {
        this->_Mesh->cleanup();
        this->_Mesh = nullptr;
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (this->resolutionUBO != nullptr)
        {
            vkDestroyBuffer(deviceModule->device, this->resolutionUBO->uniformBuffers[i], nullptr);
            vkFreeMemory(deviceModule->device, this->resolutionUBO->uniformBuffersMemory[i], nullptr);
        }
    }

    if (!this->descriptorSets.empty())
    {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            if (this->descriptorSets[i] != VK_NULL_HANDLE)
            {
                this->descriptorSets[i] = VK_NULL_HANDLE;
            }
        }
        this->descriptorSets.clear();
    }

    if (this->descriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(deviceModule->device, this->descriptorPool, nullptr);
        this->descriptorPool = VK_NULL_HANDLE;
    }

    if (this->environmentTexture != nullptr)
    {
        this->environmentTexture->cleanup();
        this->environmentTexture = nullptr;
    }
}

void AtmosphereSystem::CleanLastResources()
{
    if (this->environment_shader != nullptr)
    {
        this->environment_shader.reset();
        this->environment_shader = nullptr;
    }
}

void AtmosphereSystem::UpdateSun()
{
    if (this->environmentType == ENVIRONMENT_TYPE::PHYSICALLY_BASED_SKY)
    {
        for (int currentFrame = 0; currentFrame < MAX_FRAMES_IN_FLIGHT; currentFrame++)
        {
            void* data;
            vkMapMemory(deviceModule->device, this->sunUBO->uniformBuffersMemory[currentFrame], 0, sizeof(SunUniform), 0, &data);
            memcpy(data, &Sun, sizeof(SunUniform));
            vkUnmapMemory(deviceModule->device, this->sunUBO->uniformBuffersMemory[currentFrame]);
        }

        this->SVLUT_ComputeNode->Compute = true;
    }
}

void AtmosphereSystem::UpdateAtmopshereResolution()
{
    if (this->environmentType == ENVIRONMENT_TYPE::PHYSICALLY_BASED_SKY)
    {
        ScreenResolutionUniform resolution = {};
        resolution.resolution = glm::vec2(this->swapChainModule->swapChainExtent.width, this->swapChainModule->swapChainExtent.height);

        for (int currentFrame = 0; currentFrame < MAX_FRAMES_IN_FLIGHT; currentFrame++)
        {
            void* data;
            vkMapMemory(deviceModule->device, this->resolutionUBO->uniformBuffersMemory[currentFrame], 0, sizeof(ScreenResolutionUniform), 0, &data);
            memcpy(data, &resolution, sizeof(ScreenResolutionUniform));
            vkUnmapMemory(deviceModule->device, this->resolutionUBO->uniformBuffersMemory[currentFrame]);
        }
    }
}
