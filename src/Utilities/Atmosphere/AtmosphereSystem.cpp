#include "AtmosphereSystem.h"
#include <ShaderManager.h>
#include <MaterialManager.h>
#include <filesystem>
#include <SynchronizationModule.h>
#include <yaml-cpp/yaml.h>
#include <yaml-cpp/node/convert.h> 
#include "glm_yaml_conversions.h"
#include <QESessionManager.h>
#include <Helpers/MathHelpers.h>
#include <QERenderTarget.h>
#include <Helpers/QEMemoryTrack.h>

AtmosphereSystem::AtmosphereSystem()
{
    this->atmosphereType = AtmosphereType::PHYSICALLY_BASED_SKY;
    this->deviceModule = DeviceModule::getInstance();
    this->lightManager = LightManager::getInstance();
    this->swapChainModule = SwapChainModule::getInstance();
    this->IsInitialized = false;
}

AtmosphereSystem::~AtmosphereSystem()
{
    this->deviceModule = nullptr;
}

void AtmosphereSystem::LoadAtmosphereDto(AtmosphereDto atmosphereDto)
{
    this->pendingAtmosphereDto = atmosphereDto;
    this->hasPendingAtmosphereDto = true;

    this->atmosphereType = static_cast<AtmosphereType>(atmosphereDto.environmentType);
    this->IsInitialized = atmosphereDto.hasAtmosphere;
    this->ResourcesReady = false;

    ApplyDtoToAtmosphereData(atmosphereDto);
}

void AtmosphereSystem::EnsureSunLightCreated()
{
    auto existingSun = this->lightManager->GetLight(SUN_NAME);
    if (existingSun)
    {
        this->sunLight = std::static_pointer_cast<QESunLight>(existingSun);
        return;
    }

    this->sunLight = std::static_pointer_cast<QESunLight>(
        this->lightManager->CreateLight(LightType::SUN_LIGHT, this->SUN_NAME)
    );

    if (!this->sunLight)
        return;

    std::string sunName = this->SUN_NAME;
    this->lightManager->AddNewLight(this->sunLight, sunName);
}

void AtmosphereSystem::ApplyPendingSunState()
{
    if (!this->hasPendingAtmosphereDto)
        return;

    EnsureSunLightCreated();

    if (!this->sunLight)
        return;

    this->sunLight->baseIntensity = pendingAtmosphereDto.sunBaseIntensity;
    this->sunLight->SetSunEulerDegrees(pendingAtmosphereDto.sunEulerDegrees);
}

void AtmosphereSystem::InitializeAtmosphereResources()
{
    this->ResourcesReady = false;

    if (!this->IsInitialized)
        return;

    if (this->hasPendingAtmosphereDto)
    {
        ApplyPendingSunState();
    }

    switch (this->atmosphereType)
    {
    case AtmosphereType::CUBEMAP:
        this->InitializeAtmosphere(this->atmosphereType, nullptr, 0);
        break;
    case AtmosphereType::SPHERICALMAP:
        this->InitializeAtmosphere(this->atmosphereType, nullptr, 0);
        break;
    default:
    case AtmosphereType::PHYSICALLY_BASED_SKY:
        this->InitializeAtmosphere();
        break;
    }

    const bool shaderReady =
        (this->environment_shader != nullptr) &&
        (this->environment_shader->PipelineModule != nullptr);

    const bool meshReady =
        (this->atmosphereType == AtmosphereType::PHYSICALLY_BASED_SKY) ||
        (this->_Mesh != nullptr && this->_Mesh->GetMesh() != nullptr);

    const bool descriptorsReady = !this->descriptorSets.empty();

    this->ResourcesReady = shaderReady && descriptorsReady && meshReady;
    this->hasPendingAtmosphereDto = false;
}

AtmosphereDto AtmosphereSystem::CreateAtmosphereDto()
{
    AtmosphereDto dto = BuildDtoFromAtmosphereData();

    dto.hasAtmosphere = this->IsInitialized;
    dto.environmentType = static_cast<int>(this->atmosphereType);
    dto.sunEulerDegrees = this->sunLight
        ? this->sunLight->GetSunEulerDegrees()
        : this->pendingAtmosphereDto.sunEulerDegrees;

    dto.sunBaseIntensity = this->sunLight
        ? this->sunLight->baseIntensity
        : this->pendingAtmosphereDto.sunBaseIntensity;

    return dto;
}

void AtmosphereSystem::AddTextureResources(const std::string* texturePaths, uint32_t numTextures)
{
    TEXTURE_TYPE textureType = (this->atmosphereType == AtmosphereType::CUBEMAP) ? TEXTURE_TYPE::CUBEMAP_TYPE : TEXTURE_TYPE::DIFFUSE_TYPE;

    std::vector<std::string> resultPaths;
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

void AtmosphereSystem::InitializeAtmosphere()
{
    if (!this->sunLight)
        return;

    this->SetUpResources();

    if (this->resolutionUBO == nullptr)
        return;

    this->UpdateSun();
    this->CreateDescriptorPool();
    this->CreateDescriptorSet();
}

void AtmosphereSystem::SetUpResources()
{
    this->Cleanup();

    auto shaderManager = ShaderManager::getInstance();

    const std::string absolute_sky_vert_shader_path = this->GetAbsolutePath("../../resources/shaders", this->shaderPaths[this->atmosphereType]);
    const std::string absolute_sky_frag_shader_path = this->GetAbsolutePath("../../resources/shaders", this->shaderPaths[this->atmosphereType + (int)(shaderPaths.size() * 0.5f)]);

    GraphicsPipelineData pipelineData = {};
    pipelineData.HasVertexData = this->atmosphereType != AtmosphereType::PHYSICALLY_BASED_SKY;

    this->environment_shader = std::make_shared<ShaderModule>(
        ShaderModule("environment_map", absolute_sky_vert_shader_path, absolute_sky_frag_shader_path, pipelineData)
    );
    shaderManager->AddShader(this->environment_shader);

    if (this->atmosphereType == AtmosphereType::PHYSICALLY_BASED_SKY)
    {
        if (!this->sunLight || !this->sunLight->sunUBO)
            return;

        this->resolutionUBO = std::make_shared<UniformBufferObject>();
        this->resolutionUBO->CreateUniformBuffer(sizeof(ScreenResolutionUniform), MAX_FRAMES_IN_FLIGHT, *deviceModule);

        this->atmosphereUBO = std::make_shared<UniformBufferObject>();
        this->atmosphereUBO->CreateUniformBuffer(sizeof(AtmosphereUniform), MAX_FRAMES_IN_FLIGHT, *deviceModule);
        this->UpdateAtmosphereUBO();

        this->UpdateAtmopshereResolution();
        this->computeNodeManager = ComputeNodeManager::getInstance();

        this->TLUT_ComputeNode = std::make_shared<ComputeNode>(shaderManager->GetShader("transmittance_lut"));
        this->TLUT_ComputeNode->OnDemandCompute = true;
        this->TLUT_ComputeNode->Compute = true;
        this->TLUT_ComputeNode->NElements = 16;
        this->TLUT_ComputeNode->InitializeOutputTextureComputeNode(256, 64, VK_FORMAT_R32G32B32A32_SFLOAT);
        this->TLUT_ComputeNode->computeDescriptor->ubos["AtmosphereUniform"] = this->atmosphereUBO;
        this->computeNodeManager->UpsertComputeNode("transmittance_lut", this->TLUT_ComputeNode);

        this->MSLUT_ComputeNode = std::make_shared<ComputeNode>(shaderManager->GetShader("multi_scattering_lut"));
        this->MSLUT_ComputeNode->OnDemandCompute = true;
        this->MSLUT_ComputeNode->Compute = true;
        this->MSLUT_ComputeNode->NElements = 16;
        this->MSLUT_ComputeNode->InitializeOutputTextureComputeNode(32, 32, VK_FORMAT_R32G32B32A32_SFLOAT);
        this->MSLUT_ComputeNode->computeDescriptor->inputTextures.push_back(this->TLUT_ComputeNode->computeDescriptor->outputTexture);
        this->MSLUT_ComputeNode->computeDescriptor->ubos["AtmosphereUniform"] = this->atmosphereUBO;
        this->computeNodeManager->UpsertComputeNode("multi_scattering_lut", this->MSLUT_ComputeNode);

        this->SVLUT_ComputeNode = std::make_shared<ComputeNode>(shaderManager->GetShader("sky_view_lut"));
        this->SVLUT_ComputeNode->OnDemandCompute = true;
        this->SVLUT_ComputeNode->Compute = true;
        this->SVLUT_ComputeNode->NElements = 16;
        this->SVLUT_ComputeNode->InitializeOutputTextureComputeNode(640, 360, VK_FORMAT_R32G32B32A32_SFLOAT);
        this->SVLUT_ComputeNode->computeDescriptor->inputTextures.push_back(this->TLUT_ComputeNode->computeDescriptor->outputTexture);
        this->SVLUT_ComputeNode->computeDescriptor->inputTextures.push_back(this->MSLUT_ComputeNode->computeDescriptor->outputTexture);
        this->SVLUT_ComputeNode->computeDescriptor->ubos["SunUniform"] = this->sunLight->sunUBO;
        this->SVLUT_ComputeNode->computeDescriptor->ubos["AtmosphereUniform"] = this->atmosphereUBO;
        this->computeNodeManager->UpsertComputeNode("sky_view_lut", this->SVLUT_ComputeNode);

        this->TLUT_ComputeNode->InitializeComputeNode();
        this->MSLUT_ComputeNode->InitializeComputeNode();
        this->SVLUT_ComputeNode->InitializeComputeNode();
    }

    switch (this->atmosphereType)
    {
    default:
    case AtmosphereType::PHYSICALLY_BASED_SKY:
        this->_Mesh = std::make_shared<QEGeometryComponent>(std::make_unique<QuadGenerator>());
        break;
    case AtmosphereType::CUBEMAP:
        this->_Mesh = std::make_shared<QEGeometryComponent>(std::make_unique<CubeGenerator>());
        break;
    case AtmosphereType::SPHERICALMAP:
        this->_Mesh = std::make_shared<QEGeometryComponent>(std::make_unique<SphereGenerator>());
        break;
    }

    this->_Mesh->QEStart();
}

void AtmosphereSystem::InitializeAtmosphere(AtmosphereType type, const std::string* texturePaths, uint32_t numTextures)
{
    if (this->atmosphereType == type) return;

    this->atmosphereType = type;

    this->SetUpResources();

    this->CreateDescriptorPool();

    this->AddTextureResources(texturePaths, numTextures);
}

void AtmosphereSystem::ApplyDtoToAtmosphereData(const AtmosphereDto& dto)
{
    constexpr float kMetersToShaderUnits = 1e-6f;

    this->atmosphereData.RayleighScattering_Height =
        glm::vec4(dto.rayleighScattering, dto.rayleighScaleHeight * kMetersToShaderUnits);

    this->atmosphereData.MieScattering_Height =
        glm::vec4(dto.mieScattering, dto.mieScaleHeight * kMetersToShaderUnits);

    this->atmosphereData.OzoneAbsorption_Density =
        glm::vec4(dto.ozoneAbsorption, dto.ozoneDensity);

    this->atmosphereData.SunColor_Intensity =
        glm::vec4(dto.sunColor, dto.sunIntensityMultiplier);

    this->atmosphereData.Planet_Atmosphere_G_Exposure =
        glm::vec4(
            dto.planetRadius * kMetersToShaderUnits,
            dto.atmosphereRadius * kMetersToShaderUnits,
            dto.mieAnisotropy,
            dto.exposure
        );

    this->atmosphereData.Sky_Horizon_Multi_SunDisk =
        glm::vec4(dto.skyTint, dto.horizonSoftness, dto.multiScatteringFactor, dto.sunDiskSize);

    this->atmosphereData.SunDiskIntensity_Glow_Padding =
        glm::vec4(dto.sunDiskIntensity, dto.sunGlow, 0.0f, 0.0f);
}

AtmosphereDto AtmosphereSystem::BuildDtoFromAtmosphereData() const
{
    constexpr float kShaderUnitsToMeters = 1e6f;

    AtmosphereDto dto;

    dto.rayleighScattering = glm::vec3(this->atmosphereData.RayleighScattering_Height);
    dto.rayleighScaleHeight = this->atmosphereData.RayleighScattering_Height.w * kShaderUnitsToMeters;

    dto.mieScattering = glm::vec3(this->atmosphereData.MieScattering_Height);
    dto.mieScaleHeight = this->atmosphereData.MieScattering_Height.w * kShaderUnitsToMeters;

    dto.ozoneAbsorption = glm::vec3(this->atmosphereData.OzoneAbsorption_Density);
    dto.ozoneDensity = this->atmosphereData.OzoneAbsorption_Density.w;

    dto.sunColor = glm::vec3(this->atmosphereData.SunColor_Intensity);
    dto.sunIntensityMultiplier = this->atmosphereData.SunColor_Intensity.w;

    dto.planetRadius = this->atmosphereData.Planet_Atmosphere_G_Exposure.x * kShaderUnitsToMeters;
    dto.atmosphereRadius = this->atmosphereData.Planet_Atmosphere_G_Exposure.y * kShaderUnitsToMeters;
    dto.mieAnisotropy = this->atmosphereData.Planet_Atmosphere_G_Exposure.z;
    dto.exposure = this->atmosphereData.Planet_Atmosphere_G_Exposure.w;

    dto.skyTint = this->atmosphereData.Sky_Horizon_Multi_SunDisk.x;
    dto.horizonSoftness = this->atmosphereData.Sky_Horizon_Multi_SunDisk.y;
    dto.multiScatteringFactor = this->atmosphereData.Sky_Horizon_Multi_SunDisk.z;
    dto.sunDiskSize = this->atmosphereData.Sky_Horizon_Multi_SunDisk.w;

    dto.sunDiskIntensity = this->atmosphereData.SunDiskIntensity_Glow_Padding.x;
    dto.sunGlow = this->atmosphereData.SunDiskIntensity_Glow_Padding.y;

    return dto;
}

void AtmosphereSystem::UpdateAtmosphereUBO()
{
    if (this->atmosphereType != AtmosphereType::PHYSICALLY_BASED_SKY || this->atmosphereUBO == nullptr)
        return;

    for (uint32_t currentFrame = 0; currentFrame < MAX_FRAMES_IN_FLIGHT; currentFrame++)
    {
        void* data = nullptr;
        vkMapMemory(
            deviceModule->device,
            this->atmosphereUBO->uniformBuffersMemory[currentFrame],
            0,
            sizeof(AtmosphereUniform),
            0,
            &data
        );

        memcpy(data, &this->atmosphereData, sizeof(AtmosphereUniform));
        vkUnmapMemory(deviceModule->device, this->atmosphereUBO->uniformBuffersMemory[currentFrame]);
    }
}

void AtmosphereSystem::MarkAtmosphereLutsDirty()
{
    if (this->TLUT_ComputeNode) this->TLUT_ComputeNode->Compute = true;
    if (this->MSLUT_ComputeNode) this->MSLUT_ComputeNode->Compute = true;
    if (this->SVLUT_ComputeNode) this->SVLUT_ComputeNode->Compute = true;
}

void AtmosphereSystem::CreateDescriptorPool()
{
    std::vector<VkDescriptorPoolSize> poolSizes;

    if (this->atmosphereType != AtmosphereType::PHYSICALLY_BASED_SKY)
    {
        poolSizes.resize(2);
        poolSizes[0] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         1u * MAX_FRAMES_IN_FLIGHT };
        poolSizes[1] = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1u * MAX_FRAMES_IN_FLIGHT };
    }
    else
    {
        poolSizes.resize(2);
        poolSizes[0] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         4u * MAX_FRAMES_IN_FLIGHT };
        poolSizes[1] = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2u * MAX_FRAMES_IN_FLIGHT };
    }

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = (uint32_t)poolSizes.size();
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = MAX_FRAMES_IN_FLIGHT;

    vkCreateDescriptorPool(deviceModule->device, &poolInfo, nullptr, &descriptorPool);
}

void AtmosphereSystem::CreateDescriptorSet()
{
    if (this->atmosphereType == AtmosphereType::PHYSICALLY_BASED_SKY)
    {
        if (!this->TLUT_ComputeNode || !this->SVLUT_ComputeNode || !this->sunLight || !this->sunLight->sunUBO || !this->resolutionUBO || !this->atmosphereUBO)
            return;
    }

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

    int numWrites = (this->atmosphereType != AtmosphereType::PHYSICALLY_BASED_SKY) ? 2 : 6;
    int numBuffers = (this->atmosphereType != AtmosphereType::PHYSICALLY_BASED_SKY) ? 1 : 4;
    this->buffersInfo.resize(numBuffers);

    for (size_t frameIdx = 0; frameIdx < MAX_FRAMES_IN_FLIGHT; frameIdx++)
    {
        std::vector<VkWriteDescriptorSet> descriptorWrites;
        descriptorWrites.resize(numWrites);

        auto cameraUBO = QESessionManager::getInstance()->GetCameraUBO();

        if (this->atmosphereType != AtmosphereType::PHYSICALLY_BASED_SKY)
        {
            this->SetDescriptorWrite(descriptorWrites[0], this->descriptorSets[frameIdx], 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, cameraUBO->uniformBuffers[frameIdx], sizeof(UniformCamera));
            this->SetSamplerDescriptorWrite(descriptorWrites[1], this->descriptorSets[frameIdx], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, this->environmentTexture, this->imageInfo_1);
        }
        else
        {
            this->SetSamplerDescriptorWrite(descriptorWrites[0], this->descriptorSets[frameIdx], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, this->TLUT_ComputeNode->computeDescriptor->outputTexture, this->imageInfo_1);
            this->SetSamplerDescriptorWrite(descriptorWrites[1], this->descriptorSets[frameIdx], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, this->SVLUT_ComputeNode->computeDescriptor->outputTexture, this->imageInfo_2);

            this->SetDescriptorWrite(descriptorWrites[2], this->descriptorSets[frameIdx], 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, cameraUBO->uniformBuffers[frameIdx], sizeof(UniformCamera));
            this->SetDescriptorWrite(descriptorWrites[3], this->descriptorSets[frameIdx], 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3, this->resolutionUBO->uniformBuffers[frameIdx], sizeof(ScreenResolutionUniform));
            this->SetDescriptorWrite(descriptorWrites[4], this->descriptorSets[frameIdx], 2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4, this->sunLight->sunUBO->uniformBuffers[frameIdx], sizeof(SunUniform));
            this->SetDescriptorWrite(descriptorWrites[5], this->descriptorSets[frameIdx], 3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 5, this->atmosphereUBO->uniformBuffers[frameIdx], sizeof(AtmosphereUniform));
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

std::string AtmosphereSystem::GetAbsolutePath(std::string relativePath, std::string filename)
{
    auto absPath = std::filesystem::absolute(relativePath) / filename;

    return absPath.string();
}

void AtmosphereSystem::DrawCommand(VkCommandBuffer& commandBuffer, uint32_t frameIdx)
{
    if (!this->IsInitialized || !this->ResourcesReady)
        return;

    if (this->environment_shader == nullptr)
        return;

    if (this->environment_shader->PipelineModule == nullptr)
        return;

    if (frameIdx >= this->descriptorSets.size())
        return;

    if (this->descriptorSets[frameIdx] == VK_NULL_HANDLE)
        return;

    if (this->atmosphereType != AtmosphereType::PHYSICALLY_BASED_SKY)
    {
        if (this->_Mesh == nullptr)
            return;

        auto mesh = this->_Mesh->GetMesh();
        if (mesh == nullptr || this->_Mesh->vertexBuffer.empty() || this->_Mesh->indexBuffer.empty())
            return;
    }

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
            textureTLUT->TransitionImageLayoutImmediate(textureTLUT->image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange);
        }
        if (textureSVLUT->currentLayout == VK_IMAGE_LAYOUT_GENERAL)
        {
            textureSVLUT->TransitionImageLayoutImmediate(textureSVLUT->image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange);
        }
    }

    auto pipelineModule = this->environment_shader->PipelineModule;
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineModule->pipeline);

    vkCmdSetDepthTestEnable(commandBuffer, false);
    vkCmdSetDepthWriteEnable(commandBuffer, false);

    vkCmdSetCullMode(commandBuffer, VK_CULL_MODE_NONE);
    vkCmdSetFrontFace(commandBuffer, VK_FRONT_FACE_COUNTER_CLOCKWISE);

    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipelineModule->pipelineLayout,
        0,
        1,
        &descriptorSets[frameIdx],
        0,
        nullptr);

    if (this->atmosphereType != AtmosphereType::PHYSICALLY_BASED_SKY)
    {
        auto mesh = this->_Mesh->GetMesh();
        VkDeviceSize offsets[] = { 0 };
        VkBuffer vertexBuffers[] = { this->_Mesh->vertexBuffer[0] };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, this->_Mesh->indexBuffer[0], 0, VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mesh->MeshData[0].Indices.size()), 1, 0, 0, 0);
    }
    else
    {
        vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    }
}

void AtmosphereSystem::Cleanup()
{
    this->ResourcesReady = false;

    if (_Mesh != nullptr)
    {
        this->_Mesh->QEDestroy();
        this->_Mesh = nullptr;
    }

    if (this->resolutionUBO != nullptr)
    {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            QE_DESTROY_BUFFER(deviceModule->device, this->resolutionUBO->uniformBuffers[i], "AtmosphereSystem::Cleanup");
            QE_FREE_MEMORY(deviceModule->device, this->resolutionUBO->uniformBuffersMemory[i], "AtmosphereSystem::Cleanup");
        }
        this->resolutionUBO = nullptr;
    }

    if (this->atmosphereUBO != nullptr)
    {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            QE_DESTROY_BUFFER(deviceModule->device, this->atmosphereUBO->uniformBuffers[i], "AtmosphereSystem::Cleanup");
            QE_FREE_MEMORY(deviceModule->device, this->atmosphereUBO->uniformBuffersMemory[i], "AtmosphereSystem::Cleanup");
        }
        this->atmosphereUBO = nullptr;
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

    if (this->TLUT_ComputeNode != nullptr)
    {
        this->TLUT_ComputeNode->cleanup();
        this->TLUT_ComputeNode = nullptr;
    }

    if (this->MSLUT_ComputeNode != nullptr)
    {
        this->MSLUT_ComputeNode->cleanup();
        this->MSLUT_ComputeNode = nullptr;
    }

    if (this->SVLUT_ComputeNode != nullptr)
    {
        this->SVLUT_ComputeNode->cleanup();
        this->SVLUT_ComputeNode = nullptr;
    }
}

void AtmosphereSystem::CleanLastResources()
{
    this->sunLight.reset();

    if (this->environment_shader != nullptr)
    {
        this->environment_shader.reset();
        this->environment_shader = nullptr;
    }

    this->TLUT_ComputeNode.reset();
    this->MSLUT_ComputeNode.reset();
    this->SVLUT_ComputeNode.reset();

    this->computeNodeManager = nullptr;
    this->lightManager = nullptr;
    this->swapChainModule = nullptr;
}

void AtmosphereSystem::UpdateSun()
{
    auto activeCamera = QESessionManager::getInstance()->ActiveCamera();
    if (!activeCamera)
    {
        throw std::runtime_error("AtmosphereSystem requires an active camera before InitializeAtmosphere.");
    }

    if (!this->sunLight)
        return;

    if (this->atmosphereType == AtmosphereType::PHYSICALLY_BASED_SKY)
    {
        this->sunLight->UpdateSun();

        if (this->SVLUT_ComputeNode)
        {
            this->SVLUT_ComputeNode->Compute = true;
        }
    }
}

void AtmosphereSystem::UpdateAtmopshereResolution()
{
    if (this->atmosphereType != AtmosphereType::PHYSICALLY_BASED_SKY)
        return;

    if (this->resolutionUBO == nullptr)
        return;

    auto extraRenderTarget = QESessionManager::getInstance()->ExtraRenderTarget;
    ScreenResolutionUniform resolution = {};

    if (extraRenderTarget != nullptr)
    {
        resolution.resolution = glm::vec2(extraRenderTarget->Extent.width, extraRenderTarget->Extent.height);
    }
    else
    {
        resolution.resolution = glm::vec2(this->swapChainModule->swapChainExtent.width, this->swapChainModule->swapChainExtent.height);
    }

    for (int currentFrame = 0; currentFrame < MAX_FRAMES_IN_FLIGHT; currentFrame++)
    {
        void* data = nullptr;
        vkMapMemory(
            deviceModule->device,
            this->resolutionUBO->uniformBuffersMemory[currentFrame],
            0,
            sizeof(ScreenResolutionUniform),
            0,
            &data);

        memcpy(data, &resolution, sizeof(ScreenResolutionUniform));
        vkUnmapMemory(deviceModule->device, this->resolutionUBO->uniformBuffersMemory[currentFrame]);
    }
}

void AtmosphereSystem::UpdatePerFrame(uint32_t frame)
{
    if (!this->IsInitialized)
        return;

    UpdateSun();
    UpdateAtmopshereResolution();
    UpdateAtmosphereUBO();
}

glm::vec3 AtmosphereSystem::GetSunEulerDegrees() const
{
    return this->sunLight ? this->sunLight->GetSunEulerDegrees() : glm::vec3(0.0f);
}

void AtmosphereSystem::SetSunEulerDegrees(const glm::vec3& eulerDeg)
{
    if (!this->sunLight) return;

    this->sunLight->SetSunEulerDegrees(eulerDeg);
    this->UpdateSun();
}

float AtmosphereSystem::GetSunBaseIntensity() const
{
    return this->sunLight ? this->sunLight->baseIntensity : 0.0f;
}

void AtmosphereSystem::SetSunBaseIntensity(float intensity)
{
    if (!this->sunLight) return;

    this->sunLight->baseIntensity = intensity;
    this->sunLight->SetSunEulerDegrees(this->sunLight->GetSunEulerDegrees());

    this->UpdateSun();
}

AtmosphereDto AtmosphereSystem::GetEditableAtmosphereDto()
{
    return CreateAtmosphereDto();
}

void AtmosphereSystem::ApplyEditableAtmosphereDto(const AtmosphereDto& dto, bool rebuildLuts)
{
    this->IsInitialized = dto.hasAtmosphere;
    this->atmosphereType = static_cast<AtmosphereType>(dto.environmentType);

    if (this->sunLight)
    {
        this->sunLight->baseIntensity = dto.sunBaseIntensity;
        this->sunLight->SetSunEulerDegrees(dto.sunEulerDegrees);
        this->sunLight->UpdateSun();
    }

    ApplyDtoToAtmosphereData(dto);
    UpdateAtmosphereUBO();

    if (rebuildLuts)
    {
        MarkAtmosphereLutsDirty();
    }
    else if (this->SVLUT_ComputeNode)
    {
        this->SVLUT_ComputeNode->Compute = true;
    }
}

void AtmosphereSystem::ResetSceneState()
{
    if (this->computeNodeManager)
    {
        this->computeNodeManager->EraseComputeNodesByPrefix(TLUT_NODE_NAME);
        this->computeNodeManager->EraseComputeNodesByPrefix(MSLUT_NODE_NAME);
        this->computeNodeManager->EraseComputeNodesByPrefix(SVLUT_NODE_NAME);
    }

    Cleanup();
    this->sunLight.reset();
    this->IsInitialized = false;
    this->ResourcesReady = false;
    this->hasPendingAtmosphereDto = false;
    this->pendingAtmosphereDto = AtmosphereDto{};
}
