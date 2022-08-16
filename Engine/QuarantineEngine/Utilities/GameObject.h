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
    GameObject(std::string meshPath, VkCommandPool& commandPool, std::shared_ptr<DescriptorModule> descriptor);
    void cleanup();
    void drawCommand(VkCommandBuffer& commandBuffer, uint32_t idx);
    void addMaterial(std::shared_ptr<Material> material_ptr);
};

#endif
