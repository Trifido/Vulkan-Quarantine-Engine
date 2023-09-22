#pragma once

#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H

#include "GameObject.h"
#include "Compute/ComputeNode.h"
#include <Compute/ComputeNodeManager.h>
#include <Timer.h>

class ParticleSystem : public GameObject
{
private:
    ComputeNodeManager* computeNodeManager;
    SwapChainModule* swapchainModule;
    std::shared_ptr<ComputeNode> computeNodeEmitParticles;
    std::shared_ptr<ComputeNode> computeNodeUpdateParticles;
    Timer* timer;

    uint32_t currentParticlesNum = 0;
    double currentLifeTime = 0.0;
    DeadParticlesSSBO deadParticleListSSBO;

public:
    float  ParticleLifeTime = 10.0;
    float  LifeTime = 0.0;
    float  ParticlesPerSecond = 5.5;
    float   EmissionAngle = 0.0f;
    float   EmissionRadius = 0.0f;
    float   Gravity = 0.0f;
    float   Speed = 1.0f;
    float   Mass = 1.0f;
    bool    InfinityLifeTime = true;
    uint32_t MaxNumParticles = 100;

private:
    void createShaderStorageBuffers();
    void CreateDrawCommand(VkCommandBuffer& commandBuffer, uint32_t idx) override;
    void InitializeDeadList();
public:
    ParticleSystem();
    void GenerateParticles(size_t currentFrame);
    void cleanup();
    void drawCommand(VkCommandBuffer& commandBuffer, uint32_t idx) override;
    bool IsValid() override;
};

#endif // !PARTICLE_SYSTEM_H
