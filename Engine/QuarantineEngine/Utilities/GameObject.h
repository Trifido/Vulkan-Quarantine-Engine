#pragma once
#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include "Mesh.h"
#include "Transform.h"
#include "Material.h"
#include "DescriptorModule.h"
#include <PrimitiveTypes.h>
#include "Camera.h"
#include <PhysicBody.h>
#include <Collider.h>

typedef class GameObject GameObject;

class GameObject
{
private:
    DeviceModule*       deviceModule;
    QueueModule*        queueModule;
    MaterialManager*    materialManager;

    std::shared_ptr<Camera>             cameraEditor = nullptr;
public:
    std::shared_ptr<GeometryComponent>  mesh = nullptr;
    std::shared_ptr<Transform>          transform = nullptr;
    std::shared_ptr<Material>           material = nullptr;
    std::shared_ptr<PhysicBody>         physicBody = nullptr;
    std::shared_ptr<Collider>           collider = nullptr;

    std::shared_ptr<GameObject>                 parent = nullptr;
    std::vector<std::shared_ptr<GameObject>>    childs;

public:
    GameObject();
    GameObject(PRIMITIVE_TYPE type);
    GameObject(std::string meshPath);
    void cleanup();
    void drawCommand(VkCommandBuffer& commandBuffer, uint32_t idx);
    void addMaterial(std::shared_ptr<Material> material_ptr);
    void addEditorCamera(std::shared_ptr<Camera> camera_ptr);
    void addPhysicBody(std::shared_ptr<PhysicBody> physicBody_ptr);
    void addCollider(std::shared_ptr<Collider> collider_ptr);
    void InitializePhysics();
    void UpdatePhysicTransform();

private:
    void InitializeComponents();
    void CreateChildsGameObject(std::string pathfile);
    void DrawChilds(VkCommandBuffer& commandBuffer, uint32_t idx);
    void CreateDrawCommand(VkCommandBuffer& commandBuffer, uint32_t idx);
};

#endif
