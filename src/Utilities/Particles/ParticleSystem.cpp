#include "ParticleSystem.h"
#include "SynchronizationModule.h"
#include "Vertex.h"
#include <random>
#include <filesystem>
#include <ShaderManager.h>
#include <SwapChainModule.h>
#include <BufferManageModule.h>
#include <Timer.h>

ParticleSystem::ParticleSystem() : QEGameObject()
{
    this->computeNodeManager = ComputeNodeManager::getInstance();
    swapchainModule = SwapChainModule::getInstance();
    auto shaderManager = ShaderManager::getInstance();

    this->computeNodeEmitParticles = std::make_shared<ComputeNode>(shaderManager->GetShader("emit_compute_particles"));
    this->computeNodeEmitParticles->computeDescriptor->IsProgressiveComputation = true;
    this->computeNodeManager->AddComputeNode("emit_compute_particles", this->computeNodeEmitParticles);

    this->computeNodeUpdateParticles = std::make_shared<ComputeNode>(shaderManager->GetShader("update_compute_particles"));
    this->computeNodeUpdateParticles->computeDescriptor->IsProgressiveComputation = true;
    this->computeNodeManager->AddComputeNode("update_compute_particles", this->computeNodeUpdateParticles);

    this->InitializeParticleSystem();
}

void ParticleSystem::InitializeParticleSystem()
{
    this->createShaderStorageBuffers();
    this->InitializeDeadList();
    this->InitializeParticleSystemParameters();
    this->InitializeMaterial();
}

void ParticleSystem::cleanup()
{
    QEGameObject::QEDestroy();
}

void ParticleSystem::InitializeMaterial()
{
    std::shared_ptr<QEMaterial> mat = this->materialManager->GetMaterial("defaultParticlesMat");
    auto newMatInstance = mat->CreateMaterialInstance();
    this->materialManager->AddMaterial(newMatInstance);

    this->AddComponent<QEMaterial>(newMatInstance);
    newMatInstance->InitializeMaterialData();
    newMatInstance->descriptor->ssboData["ParticleSSBO"] = computeNodeUpdateParticles->computeDescriptor->ssboData[0];
    newMatInstance->descriptor->ssboSize["ParticleSSBO"] = computeNodeUpdateParticles->computeDescriptor->ssboSize[0];

    newMatInstance->descriptor->ubos["particleSystemUBO"] = std::make_shared<UniformBufferObject>();
    newMatInstance->descriptor->ubos["particleSystemUBO"]->CreateUniformBuffer(sizeof(ParticleTextureParamsUniform), MAX_FRAMES_IN_FLIGHT, *deviceModule);
    newMatInstance->descriptor->uboSizes["particleSystemUBO"] = sizeof(ParticleTextureParamsUniform);
    for (int currentFrame = 0; currentFrame < MAX_FRAMES_IN_FLIGHT; currentFrame++)
    {
        void* data;
        vkMapMemory(deviceModule->device, newMatInstance->descriptor->ubos["particleSystemUBO"]->uniformBuffersMemory[currentFrame], 0, sizeof(ParticleTextureParamsUniform), 0, &data);
        memcpy(data, static_cast<const void*>(&this->particleTextureParams), sizeof(ParticleTextureParamsUniform));
        vkUnmapMemory(deviceModule->device, newMatInstance->descriptor->ubos["particleSystemUBO"]->uniformBuffersMemory[currentFrame]);
    }
}

void ParticleSystem::createShaderStorageBuffers()
{
    this->computeNodeEmitParticles->computeDescriptor->InitializeSSBOData();
    //Create particle dead list
    this->computeNodeEmitParticles->computeDescriptor->ssboData.push_back(std::make_shared<UniformBufferObject>());
    this->computeNodeEmitParticles->computeDescriptor->ssboSize.push_back(VkDeviceSize());

    this->computeNodeEmitParticles->NElements = this->MaxNumParticles;
    this->computeNodeEmitParticles->InitializeComputeBuffer(0, sizeof(Particle) * this->MaxNumParticles);
    this->computeNodeEmitParticles->InitializeComputeBuffer(1, sizeof(int32_t) * (this->MaxNumParticles + 1));

    this->computeNodeUpdateParticles->computeDescriptor->AssignSSBO(this->computeNodeEmitParticles->computeDescriptor->ssboData[0], this->computeNodeEmitParticles->computeDescriptor->ssboSize[0]);
    this->computeNodeUpdateParticles->computeDescriptor->AssignSSBO(this->computeNodeEmitParticles->computeDescriptor->ssboData[1], this->computeNodeEmitParticles->computeDescriptor->ssboSize[1]);
    this->computeNodeUpdateParticles->NElements = this->MaxNumParticles;
    this->computeNodeUpdateParticles->UseDependencyBuffer = true;
}

void ParticleSystem::SetDrawCommand(VkCommandBuffer& commandBuffer, uint32_t idx, std::shared_ptr<Animator> animator)
{
    auto mat = this->GetMaterial();
    auto pipelineModule = mat->shader->PipelineModule;
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineModule->pipeline);

    vkCmdSetDepthTestEnable(commandBuffer, true);
    vkCmdSetDepthWriteEnable(commandBuffer, false);
    vkCmdSetCullMode(commandBuffer, false);

    if (mat->HasDescriptorBuffer())
    {
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineModule->pipelineLayout, 0, 1, mat->descriptor->getDescriptorSet(idx), 0, nullptr);
    }

    auto transform = this->GetComponent<QETransform>();
    vkCmdPushConstants(commandBuffer, pipelineModule->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantStruct), &transform->GetWorldMatrix());

    vkCmdDraw(commandBuffer, this->MaxNumParticles * 6, 1, 0, 0);
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

    std::vector<Particle> particles;
    particles.reserve(this->MaxNumParticles);

    for (int32_t p = 0; p < this->MaxNumParticles; p++)
    {
        Particle part = {};
        part.color = glm::vec4(0.0f);
        part.lifeTime = 0.0f;
        part.position = glm::vec3(0.0f);
        part.velocity = glm::vec3(0.0f);
        part.angle = 0.0f;
        part.auxiliarData = glm::vec4(0.0f);

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
    this->particleSystemParams.angularSpeed = this->AngularSpeed;
    this->particleSystemParams.initAngle = this->InitialAngle;
    this->particleSystemParams.initSize = this->InitialSize;
    this->particleSystemParams.auxData = 0.0f;

    this->particleTextureParams.numCols = this->NumCols;
    this->particleTextureParams.numRows = this->NumRows;
    this->particleTextureParams.totalSprites = this->NumCols * this->NumRows;
    this->particleTextureParams.auxiliarData = 0.0f;

    for (int currentFrame = 0; currentFrame < MAX_FRAMES_IN_FLIGHT; currentFrame++)
    {
        void* data;
        vkMapMemory(deviceModule->device, this->computeNodeEmitParticles->computeDescriptor->ubos["UniformParticleSystem"]->uniformBuffersMemory[currentFrame], 0, this->computeNodeEmitParticles->computeDescriptor->uboSizes["UniformParticleSystem"], 0, &data);
        memcpy(data, static_cast<const void*>(&this->particleSystemParams), this->computeNodeEmitParticles->computeDescriptor->uboSizes["UniformParticleSystem"]);
        vkUnmapMemory(deviceModule->device, this->computeNodeEmitParticles->computeDescriptor->ubos["UniformParticleSystem"]->uniformBuffersMemory[currentFrame]);

        vkMapMemory(deviceModule->device, this->computeNodeUpdateParticles->computeDescriptor->ubos["UniformParticleSystem"]->uniformBuffersMemory[currentFrame], 0, this->computeNodeUpdateParticles->computeDescriptor->uboSizes["UniformParticleSystem"], 0, &data);
        memcpy(data, static_cast<const void*>(&this->particleSystemParams), this->computeNodeUpdateParticles->computeDescriptor->uboSizes["UniformParticleSystem"]);
        vkUnmapMemory(deviceModule->device, this->computeNodeUpdateParticles->computeDescriptor->ubos["UniformParticleSystem"]->uniformBuffersMemory[currentFrame]);

        vkMapMemory(deviceModule->device, this->computeNodeUpdateParticles->computeDescriptor->ubos["UniformParticleTexture"]->uniformBuffersMemory[currentFrame], 0, this->computeNodeUpdateParticles->computeDescriptor->uboSizes["UniformParticleTexture"], 0, &data);
        memcpy(data, static_cast<const void*>(&this->particleTextureParams), this->computeNodeUpdateParticles->computeDescriptor->uboSizes["UniformParticleTexture"]);
        vkUnmapMemory(deviceModule->device, this->computeNodeUpdateParticles->computeDescriptor->ubos["UniformParticleTexture"]->uniformBuffersMemory[currentFrame]);
    }

    this->SetNewParticlesUBO(0, 0);
}

void ParticleSystem::SetNewParticlesUBO(uint32_t newParticles, uint32_t nFrame)
{
    this->newParticles.frameCount = nFrame;
    this->newParticles.newParticles = newParticles;

    for (int currentFrame = 0; currentFrame < MAX_FRAMES_IN_FLIGHT; currentFrame++)
    {
        void* data;
        vkMapMemory(deviceModule->device, this->computeNodeEmitParticles->computeDescriptor->ubos["UniformNewParticles"]->uniformBuffersMemory[currentFrame], 0, this->computeNodeEmitParticles->computeDescriptor->uboSizes["UniformNewParticles"], 0, &data);
        memcpy(data, static_cast<const void*>(&this->newParticles), this->computeNodeEmitParticles->computeDescriptor->uboSizes["UniformNewParticles"]);
        vkUnmapMemory(deviceModule->device, this->computeNodeEmitParticles->computeDescriptor->ubos["UniformNewParticles"]->uniformBuffersMemory[currentFrame]);

        vkMapMemory(deviceModule->device, this->computeNodeUpdateParticles->computeDescriptor->ubos["UniformDeltaTime"]->uniformBuffersMemory[currentFrame], 0, this->computeNodeUpdateParticles->computeDescriptor->uboSizes["UniformDeltaTime"], 0, &data);
        memcpy(data, static_cast<const void*>(&Timer::getInstance()->DeltaTime), this->computeNodeUpdateParticles->computeDescriptor->uboSizes["UniformDeltaTime"]);
        vkUnmapMemory(deviceModule->device, this->computeNodeUpdateParticles->computeDescriptor->ubos["UniformDeltaTime"]->uniformBuffersMemory[currentFrame]);
    }
}

void ParticleSystem::GenerateParticles()
{
    this->ParticlesPerSpawn = 0;
    this->acumulativeTimer += Timer::getInstance()->DeltaTime;

    unsigned int particlesToSpawn = static_cast<uint32_t>(this->acumulativeTimer / this->SpawnTime);
    this->acumulativeTimer = std::fmod(this->acumulativeTimer, this->SpawnTime);

    if (particlesToSpawn >= 1)
    {
        this->isAlreadySpawnZero = false;
        this->ParticlesPerSpawn = particlesToSpawn;
    }

    //if (!this->isAlreadySpawnZero)
    {
        this->SetNewParticlesUBO(this->ParticlesPerSpawn, Timer::getInstance()->LimitFrameCounter);
    }

    if (particlesToSpawn < 1)
    {
        this->isAlreadySpawnZero = true;
    }
}

void ParticleSystem::AddParticleTexture(std::shared_ptr<CustomTexture> texture)
{
    auto mat = this->GetMaterial();
    mat->materialData.texture_vector->at(0) = texture;
}

void ParticleSystem::Update()
{
    this->GenerateParticles();
}

void ParticleSystem::CreateDrawCommand(VkCommandBuffer& commandBuffer, uint32_t idx)
{
    this->SetDrawCommand(commandBuffer, idx);
}
