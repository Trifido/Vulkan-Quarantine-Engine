#pragma once
#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include "Mesh.h"
#include "Transform.h"
#include "Material.h"
#include "DescriptorModule.h"

class GameObject
{
private:
    DeviceModule*       deviceModule;
    QueueModule*        queueModule;
public:
    std::shared_ptr<Mesh>               mesh;
    std::shared_ptr<Transform>          transform;
    std::shared_ptr<Material>           material;
    std::shared_ptr<DescriptorModule>   descriptorModule;

public:
    GameObject();
    GameObject(std::string meshPath, std::string albedoPath, uint32_t numSwapChain, VkCommandPool& commandPool);
    void cleanup();
    void drawCommand(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, VkPipeline& pipeline, unsigned int idx);
};

#endif