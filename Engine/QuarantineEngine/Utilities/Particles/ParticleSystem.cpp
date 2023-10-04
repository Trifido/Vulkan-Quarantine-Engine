#include "ParticleSystem.h"
#include "SynchronizationModule.h"
#include "Vertex.h"
#include <random>
#include <filesystem>
#include <ShaderManager.h>
#include <SwapChainModule.h>
#include <BufferManageModule.h>

ParticleSystem::ParticleSystem() : GameObject()
{
    this->timer = Timer::getInstance();
    this->computeNodeManager = ComputeNodeManager::getInstance();
    swapchainModule = SwapChainModule::getInstance();
    auto shaderManager = ShaderManager::getInstance();

    this->computeNodeEmitParticles = std::make_shared<ComputeNode>(shaderManager->GetShader("emit_compute_particles"));
    this->computeNodeEmitParticles->computeDescriptor->IsProgressiveComputation = true;
    this->computeNodeManager->AddComputeNode("emit_compute_particles", this->computeNodeEmitParticles);

    this->computeNodeUpdateParticles = std::make_shared<ComputeNode>(shaderManager->GetShader("update_compute_particles"));
    this->computeNodeUpdateParticles->computeDescriptor->IsProgressiveComputation = true;
    this->computeNodeManager->AddComputeNode("update_compute_particles", this->computeNodeUpdateParticles);

    this->createShaderStorageBuffers();
    this->InitializeDeadList();
    this->InitializeParticleSystemParameters();

    auto mat = this->materialManager->GetMaterial("defaultParticlesMat");

    auto newMatInstance = mat->CreateMaterialInstance();
    this->materialManager->AddMaterial("defaultParticlesMat", newMatInstance);

    this->addMaterial(newMatInstance);
    this->material->InitializeMaterialDataUBO();
}

void ParticleSystem::cleanup()
{
    GameObject::cleanup();
}

void ParticleSystem::createShaderStorageBuffers()
{
    this->computeNodeEmitParticles->computeDescriptor->InitializeSSBOData();
    //Create particle dead list
    this->computeNodeEmitParticles->computeDescriptor->ssboData.push_back(std::make_shared<UniformBufferObject>());
    this->computeNodeEmitParticles->computeDescriptor->ssboSize.push_back(VkDeviceSize());

    this->computeNodeEmitParticles->NElements = this->MaxNumParticles;
    this->computeNodeEmitParticles->InitializeComputeBuffer(0, sizeof(ParticleV2Input) * this->MaxNumParticles);
    this->computeNodeEmitParticles->InitializeComputeBuffer(1, sizeof(int32_t) * (this->MaxNumParticles + 1));

    this->computeNodeUpdateParticles->computeDescriptor->AssignSSBO(this->computeNodeEmitParticles->computeDescriptor->ssboData[0], this->computeNodeEmitParticles->computeDescriptor->ssboSize[0]);
    this->computeNodeUpdateParticles->computeDescriptor->AssignSSBO(this->computeNodeEmitParticles->computeDescriptor->ssboData[1], this->computeNodeEmitParticles->computeDescriptor->ssboSize[1]);
    this->computeNodeUpdateParticles->NElements = this->MaxNumParticles;
    this->computeNodeUpdateParticles->UseDependencyBuffer = true;
}

void ParticleSystem::CreateDrawCommand(VkCommandBuffer& commandBuffer, uint32_t idx)
{
    auto pipelineModule = this->material->shader->PipelineModule;
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineModule->pipeline);

    vkCmdSetDepthTestEnable(commandBuffer, true);
    vkCmdSetCullMode(commandBuffer, false);

    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &computeNodeUpdateParticles->computeDescriptor->ssboData[0]->uniformBuffers.at(idx), offsets);

    if (this->material->HasDescriptorBuffer())
    {
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineModule->pipelineLayout, 0, 1, this->material->descriptor->getDescriptorSet(idx), 0, nullptr);
    }

    this->pushConstant.model = this->transform->GetModel();
    if (this->parent != nullptr)
    {
        pushConstant.model = this->parent->transform->GetModel() * pushConstant.model;
    }
    vkCmdPushConstants(commandBuffer, pipelineModule->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantStruct), &pushConstant);

    vkCmdDraw(commandBuffer, this->MaxNumParticles, 1, 0, 0);
}

void ParticleSystem::InitializeDeadList()
{
    this->deadParticles.reserve(this->MaxNumParticles + 1);

    for (int32_t p = 0; p < this->MaxNumParticles; p++)
    {
        this->deadParticles.push_back(p);
    }

    this->deadParticles.push_back(this->MaxNumParticles);

    for (int currentFrame = 0; currentFrame < MAX_FRAMES_IN_FLIGHT; currentFrame++)
    {
        void* data;
        vkMapMemory(deviceModule->device, this->computeNodeEmitParticles->computeDescriptor->ssboData[1]->uniformBuffersMemory[currentFrame], 0, this->computeNodeEmitParticles->computeDescriptor->ssboSize[1], 0, &data);
        memcpy(data, this->deadParticles.data(), this->computeNodeEmitParticles->computeDescriptor->ssboSize[1]);
        vkUnmapMemory(deviceModule->device, this->computeNodeEmitParticles->computeDescriptor->ssboData[1]->uniformBuffersMemory[currentFrame]);
    }

    std::vector<ParticleV2Input> particles;
    particles.reserve(this->MaxNumParticles);

    for (int32_t p = 0; p < this->MaxNumParticles; p++)
    {
        ParticleV2Input part = {};
        part.seed = 0.0f;
        part.color = glm::vec4(0.0f);
        part.lifeTime = 0.0f;
        part.position = glm::vec3(0.0f);
        part.velocity = glm::vec3(0.0f);

        particles.push_back(part);
    }

    // Initialize particles
    for (int currentFrame = 0; currentFrame < MAX_FRAMES_IN_FLIGHT; currentFrame++)
    {
        void* data;
        vkMapMemory(deviceModule->device, this->computeNodeEmitParticles->computeDescriptor->ssboData[0]->uniformBuffersMemory[currentFrame], 0, this->computeNodeEmitParticles->computeDescriptor->ssboSize[0], 0, &data);
        memcpy(data, particles.data(), this->computeNodeEmitParticles->computeDescriptor->ssboSize[0]);
        vkUnmapMemory(deviceModule->device, this->computeNodeEmitParticles->computeDescriptor->ssboData[0]->uniformBuffersMemory[currentFrame]);
    }
}

void ParticleSystem::InitializeParticleSystemParameters()
{
    this->particleSystemParams.emissionAngle = this->EmissionAngle;
    this->particleSystemParams.emissionRadius = this->EmissionRadius;
    this->particleSystemParams.gravity = this->Gravity;
    this->particleSystemParams.initialColor = this->InitialColor;
    this->particleSystemParams.maxParticles = this->MaxNumParticles;
    this->particleSystemParams.particleLifeTime = this->ParticleLifeTime;
    this->particleSystemParams.particlePerFrame = this->SpawnTime;
    this->particleSystemParams.particleSystemDuration = this->LifeTime;
    this->particleSystemParams.speed = this->Speed;

    for (int currentFrame = 0; currentFrame < MAX_FRAMES_IN_FLIGHT; currentFrame++)
    {
        void* data;
        vkMapMemory(deviceModule->device, this->computeNodeEmitParticles->computeDescriptor->ubos["UniformParticleSystem"]->uniformBuffersMemory[currentFrame], 0, this->computeNodeEmitParticles->computeDescriptor->uboSizes["UniformParticleSystem"], 0, &data);
        memcpy(data, static_cast<const void*>(&this->particleSystemParams), this->computeNodeEmitParticles->computeDescriptor->uboSizes["UniformParticleSystem"]);
        vkUnmapMemory(deviceModule->device, this->computeNodeEmitParticles->computeDescriptor->ubos["UniformParticleSystem"]->uniformBuffersMemory[currentFrame]);

        vkMapMemory(deviceModule->device, this->computeNodeUpdateParticles->computeDescriptor->ubos["UniformParticleSystem"]->uniformBuffersMemory[currentFrame], 0, this->computeNodeUpdateParticles->computeDescriptor->uboSizes["UniformParticleSystem"], 0, &data);
        memcpy(data, static_cast<const void*>(&this->particleSystemParams), this->computeNodeUpdateParticles->computeDescriptor->uboSizes["UniformParticleSystem"]);
        vkUnmapMemory(deviceModule->device, this->computeNodeUpdateParticles->computeDescriptor->ubos["UniformParticleSystem"]->uniformBuffersMemory[currentFrame]);
    }

    this->newParticles.frameCount = 0;
    this->newParticles.newParticles = 0;

    for (int currentFrame = 0; currentFrame < MAX_FRAMES_IN_FLIGHT; currentFrame++)
    {
        void* data;
        vkMapMemory(deviceModule->device, this->computeNodeEmitParticles->computeDescriptor->ubos["UniformNewParticles"]->uniformBuffersMemory[currentFrame], 0, this->computeNodeEmitParticles->computeDescriptor->uboSizes["UniformNewParticles"], 0, &data);
        memcpy(data, static_cast<const void*>(&this->newParticles), this->computeNodeEmitParticles->computeDescriptor->uboSizes["UniformNewParticles"]);
        vkUnmapMemory(deviceModule->device, this->computeNodeEmitParticles->computeDescriptor->ubos["UniformNewParticles"]->uniformBuffersMemory[currentFrame]);

        vkMapMemory(deviceModule->device, this->computeNodeUpdateParticles->computeDescriptor->ubos["UniformDeltaTime"]->uniformBuffersMemory[currentFrame], 0, this->computeNodeUpdateParticles->computeDescriptor->uboSizes["UniformDeltaTime"], 0, &data);
        memcpy(data, static_cast<const void*>(&this->timer->DeltaTime), this->computeNodeUpdateParticles->computeDescriptor->uboSizes["UniformDeltaTime"]);
        vkUnmapMemory(deviceModule->device, this->computeNodeUpdateParticles->computeDescriptor->ubos["UniformDeltaTime"]->uniformBuffersMemory[currentFrame]);
    }
}

void ParticleSystem::GenerateParticles()
{
    this->ParticlesPerSpawn = 0;
    this->acumulativeTimer += this->timer->DeltaTime;

    unsigned int particlesToSpawn = this->acumulativeTimer / this->SpawnTime;
    this->acumulativeTimer = std::fmod(this->acumulativeTimer, this->SpawnTime);

    if (particlesToSpawn >= 1)
    {
        this->ParticlesPerSpawn = 5;
    }

    this->newParticles.frameCount = this->timer->LimitFrameCounter;
    this->newParticles.newParticles = this->ParticlesPerSpawn;

    for (int currentFrame = 0; currentFrame < MAX_FRAMES_IN_FLIGHT; currentFrame++)
    {
        void* data;
        vkMapMemory(deviceModule->device, this->computeNodeEmitParticles->computeDescriptor->ubos["UniformNewParticles"]->uniformBuffersMemory[currentFrame], 0, this->computeNodeEmitParticles->computeDescriptor->uboSizes["UniformNewParticles"], 0, &data);
        memcpy(data, static_cast<const void*>(&this->newParticles), this->computeNodeEmitParticles->computeDescriptor->uboSizes["UniformNewParticles"]);
        vkUnmapMemory(deviceModule->device, this->computeNodeEmitParticles->computeDescriptor->ubos["UniformNewParticles"]->uniformBuffersMemory[currentFrame]);

        vkMapMemory(deviceModule->device, this->computeNodeUpdateParticles->computeDescriptor->ubos["UniformDeltaTime"]->uniformBuffersMemory[currentFrame], 0, this->computeNodeUpdateParticles->computeDescriptor->uboSizes["UniformDeltaTime"], 0, &data);
        memcpy(data, static_cast<const void*>(&this->timer->DeltaTime), this->computeNodeUpdateParticles->computeDescriptor->uboSizes["UniformDeltaTime"]);
        vkUnmapMemory(deviceModule->device, this->computeNodeUpdateParticles->computeDescriptor->ubos["UniformDeltaTime"]->uniformBuffersMemory[currentFrame]);
    }
}

void ParticleSystem::Update()
{
    this->GenerateParticles();
}

void ParticleSystem::drawCommand(VkCommandBuffer& commandBuffer, uint32_t idx)
{
    this->CreateDrawCommand(commandBuffer, idx);
}

bool ParticleSystem::IsValid()
{
    return true;
}
