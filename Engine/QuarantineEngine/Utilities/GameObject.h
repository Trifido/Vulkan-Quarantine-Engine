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
#include <CullingSceneManager.h>
#include <SkeletalComponent.h>
#include <Numbered.h>
#include <AABBObject.h>

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
    friend class CullingSceneManager;

private:
    bool isMeshShading = false;
    PFN_vkCmdDrawMeshTasksEXT vkCmdDrawMeshTasksEXT = nullptr;
    CullingSceneManager* cullingSceneManager = nullptr;
    std::shared_ptr<AABBObject> aabbculling = nullptr;

protected:
    DeviceModule*       deviceModule = nullptr;
    QueueModule*        queueModule = nullptr;
    MaterialManager*    materialManager = nullptr;
    AnimationManager*   animationManager = nullptr;
    MeshImportedType    meshImportedType;
    PushConstantStruct  pushConstant;
    PCOmniShadowStruct  omniPushConstant;

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

private:
    void InitializeResources();
    bool isRenderEnable();

public:
    GameObject();
    GameObject(PRIMITIVE_TYPE type, bool isMeshShading = false);
    GameObject(std::string meshPath, bool isMeshShading = false);
    inline std::string ID() const { return id; }
    void cleanup();
    virtual void drawCommand(VkCommandBuffer& commandBuffer, uint32_t idx);
    virtual void drawShadowCommand(VkCommandBuffer& commandBuffer, uint32_t idx, VkPipelineLayout pipelineLayout, bool isOmniShadow = false);
    virtual void drawOmniShadowCommand(VkCommandBuffer& commandBuffer, uint32_t idx, VkPipelineLayout pipelineLayout, PCOmniShadowStruct shadowParameters);
    void addMaterial(std::shared_ptr<Material> material_ptr);
    void addPhysicBody(std::shared_ptr<PhysicBody> physicBody_ptr);
    void addCollider(std::shared_ptr<Collider> collider_ptr);
    void addAnimation(std::shared_ptr<Animation> animation_ptr);
    void InitializePhysics();
    virtual bool IsValid();
    void UpdatePhysicTransform();
    void SetViewOmniShadowParameter(PCOmniShadowStruct shadowConstantParameter);

protected:
    void InitializeComponents();
    void InitializeAnimationComponent();
    bool CreateChildsGameObject(std::string pathfile);
    virtual void CreateDrawCommand(VkCommandBuffer& commandBuffer, uint32_t idx, std::shared_ptr<Animator> animator_ptr = nullptr);
    void CreateDrawShadowCommand(VkCommandBuffer& commandBuffer, uint32_t idx, VkPipelineLayout pipelineLayout, std::shared_ptr<Animator> animator_ptr = nullptr, bool isOmniShadow = false);
};

#endif
