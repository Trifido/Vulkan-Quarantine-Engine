#pragma once
#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include "Mesh.h"
#include "Transform.h"
#include "Material.h"
#include "DescriptorModule.h"
#include <PrimitiveTypes.h>

class GameObject
{
private:
    DeviceModule*       deviceModule;
    QueueModule*        queueModule;
public:
    std::shared_ptr<GeometryComponent>  mesh;
    std::shared_ptr<Transform>          transform;
    std::shared_ptr<Material>           material;
    std::shared_ptr<DescriptorModule>   descriptorModule;

public:
    GameObject();
    GameObject(PRIMITIVE_TYPE type, std::shared_ptr<DescriptorModule> descriptor);
    GameObject(std::string meshPath, std::shared_ptr<DescriptorModule> descriptor);
    void cleanup();
    void drawCommand(VkCommandBuffer& commandBuffer, uint32_t idx);
    void addMaterial(std::shared_ptr<Material> material_ptr);
};

#endif
