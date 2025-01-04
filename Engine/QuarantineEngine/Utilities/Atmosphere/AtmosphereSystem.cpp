#include "AtmosphereSystem.h"
#include <ShaderManager.h>
#include <MaterialManager.h>
#include <filesystem>
#include <PrimitiveMesh.h>
#include <SynchronizationModule.h>

AtmosphereSystem::AtmosphereSystem()
{
    this->deviceModule = DeviceModule::getInstance();
}

AtmosphereSystem::~AtmosphereSystem()
{
    this->deviceModule = nullptr;
}

void AtmosphereSystem::AddSkyboxResources(std::string texturePath)
{
    const std::string path = this->GetAbsolutePath(texturePath);

    this->skyboxTexture = std::make_shared<CustomTexture>(path, TEXTURE_TYPE::CUBEMAP_TYPE);
    this->CreateDescriptorSet();
}

void AtmosphereSystem::AddSkyboxResources(vector<string> texturePaths)
{
    assert(texturePaths.size() == 6);

    for (size_t i = 0; i < texturePaths.size(); i++)
    {
        texturePaths[i] = this->GetAbsolutePath(texturePaths[i]);
    }

    this->skyboxTexture = std::make_shared<CustomTexture>(texturePaths);
    this->CreateDescriptorSet();
}

void AtmosphereSystem::InitializeResources()
{
    auto absPath = std::filesystem::absolute("../../resources/shaders").generic_string();

    std::string substring = "/Engine";
    std::size_t ind = absPath.find(substring);

    if (ind != std::string::npos) {
        absPath.erase(ind, substring.length());
    }

    const std::string absolute_skybox_cubemap_vert_shader_path = absPath + "/Atmosphere/skybox_cubemap_vert.spv";
    const std::string absolute_skybox_cubemap_frag_shader_path = absPath + "/Atmosphere/skybox_cubemap_frag.spv";

    auto shaderManager = ShaderManager::getInstance();
    this->skybox_cubemap_shader = std::make_shared<ShaderModule>(
        ShaderModule(absolute_skybox_cubemap_vert_shader_path, absolute_skybox_cubemap_frag_shader_path)
    );
    shaderManager->AddShader("skybox_cubemap", this->skybox_cubemap_shader);

    this->_Mesh = std::make_shared<PrimitiveMesh>(PRIMITIVE_TYPE::CUBE_TYPE);
    this->_Mesh->InitializeMesh();

    this->CreateDescriptorPool();
}

void AtmosphereSystem::SetCamera(Camera* cameraPtr)
{
    this->camera = cameraPtr;
}

void AtmosphereSystem::CreateDescriptorPool()
{
    std::vector<VkDescriptorPoolSize> poolSizes;
    poolSizes.resize(2);
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = 1;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = 1;

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
    for (uint32_t i = 0; i < this->skybox_cubemap_shader->descriptorSetLayouts.size(); i++)
    {
        for (uint32_t f = 0; f < MAX_FRAMES_IN_FLIGHT; f++)
        {
            layouts.push_back(this->skybox_cubemap_shader->descriptorSetLayouts.at(i));
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

    for (size_t frameIdx = 0; frameIdx < MAX_FRAMES_IN_FLIGHT; frameIdx++)
    {
        std::vector<VkWriteDescriptorSet> descriptorWrites;
        descriptorWrites.resize(2);

        this->SetDescriptorWrite(descriptorWrites[0], this->descriptorSets[frameIdx], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, this->camera->cameraUBO->uniformBuffers[frameIdx], sizeof(CameraUniform));
        this->SetCubeMapDescriptorWrite(descriptorWrites[1], this->descriptorSets[frameIdx], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1);

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

void AtmosphereSystem::SetDescriptorWrite(VkWriteDescriptorSet& descriptorWrite, VkDescriptorSet descriptorSet, VkDescriptorType descriptorType, uint32_t binding, VkBuffer buffer, VkDeviceSize bufferSize)
{
    this->buffersInfo = GetBufferInfo(buffer, bufferSize);

    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = binding;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = descriptorType;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = VK_NULL_HANDLE;
    descriptorWrite.pBufferInfo = &this->buffersInfo;
}

void AtmosphereSystem::SetCubeMapDescriptorWrite(VkWriteDescriptorSet& descriptorWrite, VkDescriptorSet descriptorSet, VkDescriptorType descriptorType, uint32_t binding)
{
    this->imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    this->imageInfo.imageView = this->skyboxTexture->imageView; // ImageView del cubemap
    this->imageInfo.sampler = this->skyboxTexture->textureSampler; // Sampler para el cubemap

    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = binding;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = descriptorType;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &this->imageInfo;
}

string AtmosphereSystem::GetAbsolutePath(string relativePath)
{
    auto absPath = std::filesystem::absolute("../../resources/textures").generic_string();

    std::string substring = "/Engine";
    std::size_t ind = absPath.find(substring);

    if (ind != std::string::npos) {
        absPath.erase(ind, substring.length());
    }

    return absPath + "/" + relativePath;
}

void AtmosphereSystem::DrawCommand(VkCommandBuffer& commandBuffer, uint32_t frameIdx)
{
    if (this->_Mesh != nullptr)
    {
        auto pipelineModule = this->skybox_cubemap_shader->PipelineModule;
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineModule->pipeline);

        vkCmdSetDepthTestEnable(commandBuffer, false);
        vkCmdSetDepthWriteEnable(commandBuffer, false);

        vkCmdSetCullMode(commandBuffer, VK_CULL_MODE_NONE);
        vkCmdSetFrontFace(commandBuffer, VK_FRONT_FACE_COUNTER_CLOCKWISE);

        VkDeviceSize offsets[] = { 0 };
        VkBuffer vertexBuffers[] = { this->_Mesh->vertexBuffer };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, this->_Mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->skybox_cubemap_shader->PipelineModule->pipelineLayout, 0, 1, &descriptorSets.at(frameIdx), 0, nullptr);

        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(this->_Mesh->indices.size()), 1, 0, 0, 0);
    }
}

void AtmosphereSystem::Cleanup()
{
    this->_Mesh->cleanup();
    this->_Mesh = nullptr;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (this->descriptorSets[i] != VK_NULL_HANDLE)
        {
            this->descriptorSets[i] = VK_NULL_HANDLE;
        }
    }

    if (this->descriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(deviceModule->device, this->descriptorPool, nullptr);
        this->descriptorPool = VK_NULL_HANDLE;
    }

    this->skyboxTexture->cleanup();
}

void AtmosphereSystem::CleanLastResources()
{
    this->skybox_cubemap_shader.reset();
    this->skybox_cubemap_shader = nullptr;
}
