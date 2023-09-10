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
    size_t numParticles;
    ComputeNodeManager* computeNodeManager;
    SwapChainModule* swapchainModule;
    std::shared_ptr<ComputeNode> computeNode;
    Timer* timer;

private:
    void createShaderStorageBuffers();
    void CreateDrawCommand(VkCommandBuffer& commandBuffer, uint32_t idx) override;
public:
    ParticleSystem();
    void cleanup();
    void drawCommand(VkCommandBuffer& commandBuffer, uint32_t idx) override;
    bool IsValid() override;
};

#endif // !PARTICLE_SYSTEM_H
