#pragma once

#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H

#include "GameObject.h"
#include "Compute/ComputeNode.h"
#include <Compute/ComputeNodeManager.h>
#include <Timer.h>

struct ParticleSystemUBO
{
    glm::vec4  initialColor;
    float particleLifeTime;
    float particleSystemDuration;
    float particlePerFrame;
    float gravity;
    float emissionAngle;
    float emissionRadius;
    float speed;
    uint32_t  maxParticles;
};

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

    uint32_t currentParticlesNum = 0;
    double currentLifeTime = 0.0;
    std::vector<int32_t> deadParticles;
    ParticleSystemUBO particleSystemParams;
    NewParticlesUBO newParticles;

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
    int32_t MaxNumParticles = 100;
    glm::vec4 InitialColor = glm::vec4(1.0f);

private:
    void createShaderStorageBuffers();
    void CreateDrawCommand(VkCommandBuffer& commandBuffer, uint32_t idx) override;
    void InitializeDeadList();
    void InitializeParticleSystemParameters();
public:
    ParticleSystem();
    void GenerateParticles(size_t currentFrame);
    void Update();
    void cleanup();
    void drawCommand(VkCommandBuffer& commandBuffer, uint32_t idx) override;
    bool IsValid() override;
};

#endif // !PARTICLE_SYSTEM_H
