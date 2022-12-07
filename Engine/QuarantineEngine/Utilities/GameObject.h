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
#include <AnimationManager.h>
#include <SkeletalComponent.h>

typedef class GameObject GameObject;

enum MeshImportedType
{
    COMMON_GEO,
    PRIMITIVE_GEO,
    ANIMATED_GEO,
    NONE_GEO
};

class GameObject
{
private:
    std::string         id;
    DeviceModule*       deviceModule = nullptr;
    QueueModule*        queueModule = nullptr;
    MaterialManager*    materialManager = nullptr;
    AnimationManager*   animationManager = nullptr;
    MeshImportedType    meshImportedType;

    std::shared_ptr<Camera>             cameraEditor = nullptr;

public:
    std::shared_ptr<GeometryComponent>  mesh = nullptr;
    std::shared_ptr<Transform>          transform = nullptr;
    std::shared_ptr<Material>           material = nullptr;
    std::shared_ptr<PhysicBody>         physicBody = nullptr;
    std::shared_ptr<Collider>           collider = nullptr;
    std::shared_ptr<AnimationComponent> animationComponent = nullptr;
    std::shared_ptr<SkeletalComponent>  skeletalComponent = nullptr;

    std::shared_ptr<GameObject>                 parent = nullptr;
    std::vector<std::shared_ptr<GameObject>>    childs;

public:
    GameObject();
    GameObject(PRIMITIVE_TYPE type);
    GameObject(std::string meshPath);
    inline std::string ID() const { return id; }
    void cleanup();
    void drawCommand(VkCommandBuffer& commandBuffer, uint32_t idx);
    void addMaterial(std::shared_ptr<Material> material_ptr);
    void addEditorCamera(std::shared_ptr<Camera> camera_ptr);
    void addPhysicBody(std::shared_ptr<PhysicBody> physicBody_ptr);
    void addCollider(std::shared_ptr<Collider> collider_ptr);
    void addAnimation(std::shared_ptr<Animation> animation_ptr);
    void InitializePhysics();
    void UpdatePhysicTransform();

private:
    void CreateGameObjectID(size_t length);
    void InitializeComponents(size_t numMeshAttributes);
    void InitializeAnimationComponent();
    void CreateChildsGameObject(std::string pathfile);
    void DrawChilds(VkCommandBuffer& commandBuffer, uint32_t idx);
    void CreateDrawCommand(VkCommandBuffer& commandBuffer, uint32_t idx);
    size_t CheckNumAttributes();
};

#endif
