#pragma once
#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include "Mesh.h"
#include "Transform.h"
#include "Material.h"
#include <PrimitiveTypes.h>
#include "Camera.h"
#include <PhysicBody.h>
#include <Collider.h>
#include <AnimationManager.h>
#include <SkeletalComponent.h>
#include <Numbered.h>

typedef class GameObject GameObject;

enum MeshImportedType
{
    COMMON_GEO,
    EDITOR_GEO,
    PRIMITIVE_GEO,
    ANIMATED_GEO,
    NONE_GEO
};

class GameObject : Numbered
{
private:
    bool isMeshShading = false;
    PFN_vkCmdDrawMeshTasksNV vkCmdDrawMeshTasksNV;

protected:
    DeviceModule*       deviceModule = nullptr;
    QueueModule*        queueModule = nullptr;
    MaterialManager*    materialManager = nullptr;
    AnimationManager*   animationManager = nullptr;
    MeshImportedType    meshImportedType;
    PushConstantStruct  pushConstant;

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
    GameObject(PRIMITIVE_TYPE type, bool isMeshShading = false);
    GameObject(std::string meshPath, bool isMeshShading = false);
    inline std::string ID() const { return id; }
    void cleanup();
    virtual void drawCommand(VkCommandBuffer& commandBuffer, uint32_t idx);
    void addMaterial(std::shared_ptr<Material> material_ptr);
    void addPhysicBody(std::shared_ptr<PhysicBody> physicBody_ptr);
    void addCollider(std::shared_ptr<Collider> collider_ptr);
    void addAnimation(std::shared_ptr<Animation> animation_ptr);
    void InitializePhysics();
    virtual bool IsValid();
    void UpdatePhysicTransform();

protected:
    void InitializeComponents(size_t numMeshAttributes);
    void InitializeAnimationComponent();
    bool CreateChildsGameObject(std::string pathfile);
    virtual void CreateDrawCommand(VkCommandBuffer& commandBuffer, uint32_t idx);
    virtual void CreateAnimationDrawCommand(VkCommandBuffer& commandBuffer, uint32_t idx, std::shared_ptr<Animator> animator);
    size_t CheckNumAttributes();
};

#endif
