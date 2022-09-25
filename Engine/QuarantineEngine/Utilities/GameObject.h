#pragma once
#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include "Mesh.h"
#include "Transform.h"
#include "Material.h"
#include "DescriptorModule.h"
#include <PrimitiveTypes.h>
#include "Camera.h"

class GameObject
{
private:
    DeviceModule*       deviceModule;
    QueueModule*        queueModule;

    std::shared_ptr<Camera>             cameraEditor;
public:
    std::shared_ptr<GeometryComponent>  mesh;
    std::shared_ptr<Transform>          transform;
    std::shared_ptr<Material>           material;

public:
    GameObject();
    GameObject(PRIMITIVE_TYPE type);
    GameObject(std::string meshPath);
    void cleanup();
    void drawCommand(VkCommandBuffer& commandBuffer, uint32_t idx);
    void addMaterial(std::shared_ptr<Material> material_ptr);
    void addEditorCamera(std::shared_ptr<Camera> camera_ptr);

private:
    void InitializeComponents();
};

#endif
