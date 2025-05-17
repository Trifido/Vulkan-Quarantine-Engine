#pragma once

#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H

#include "GameObject.h"
#include "Compute/ComputeNode.h"
#include <Compute/ComputeNodeManager.h>
#include <Timer.h>

struct NewParticlesUBO
{
    uint32_t  newParticles;
    uint32_t  frameCount;
};

class ParticleSystem : public GameObject
{
private:
    ComputeNodeManager* computeNodeManager;
    SwapChainModule* swapchainModule;
    std::shared_ptr<ComputeNode> computeNodeEmitParticles;
    std::shared_ptr<ComputeNode> computeNodeUpdateParticles;
    Timer* timer;
    float acumulativeTimer = 0.0f;

    uint32_t currentParticlesNum = 0;
    double currentLifeTime = 0.0;
    std::vector<int32_t> deadParticles;
    ParticleSystemUniform particleSystemParams;
    ParticleTextureParamsUniform particleTextureParams;
    NewParticlesUBO newParticles;

    bool isAlreadySpawnZero = false;

public:
    float   ParticleLifeTime = 10.0f;
    float   LifeTime = 0.0f;
    float   SpawnTime = 0.1f;
    uint32_t    ParticlesPerSpawn = 10;
    float   EmissionAngle = 0.35f;
    float   EmissionRadius = 0.0f;
    float   Gravity = 0.0f;// 9.8f;
    float   Speed = 0.75f;// 10.5f;
    float   AngularSpeed = 0.1f;
    float   InitialAngle = 1.0f;
    float   InitialSize = 1.0f;
    float   Mass = 1.0f;
    float   NumRows = 4.0f;
    float   NumCols = 4.0f;
    float   TotalSprites = 16.0f;
    bool    InfinityLifeTime = true;
    int32_t MaxNumParticles = 500;
    glm::vec4 InitialColor = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);

private:
    void createShaderStorageBuffers();
    void SetDrawCommand(VkCommandBuffer& commandBuffer, uint32_t idx, std::shared_ptr<Animator> animator_ptr = nullptr) override;
    void InitializeDeadList();
    void InitializeParticleSystemParameters();
    void SetNewParticlesUBO(uint32_t newParticles, uint32_t nFrame);
    void InitializeMaterial();

public:
    ParticleSystem();
    void InitializeParticleSystem();
    void GenerateParticles();
    void AddParticleTexture(std::shared_ptr<CustomTexture> texture);
    void Update();
    void cleanup();
    void CreateDrawCommand(VkCommandBuffer& commandBuffer, uint32_t idx) override;
    bool IsValidRender() override;
};

#endif // !PARTICLE_SYSTEM_H
