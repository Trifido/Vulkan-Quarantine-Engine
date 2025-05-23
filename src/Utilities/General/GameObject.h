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
#include <GameObjectDto.h>
#include <QECharacterController.h>

typedef class GameObject GameObject;

class GameObject : Numbered
{
    friend class CullingSceneManager;

private:
    bool isMeshShading = false;
    PFN_vkCmdDrawMeshTasksEXT vkCmdDrawMeshTasksEXT = nullptr;
    CullingSceneManager* cullingSceneManager = nullptr;
    std::shared_ptr<AABBObject> aabbculling = nullptr;
    std::string bindMaterialName = "NULL_MATERIAL";

protected:
    DeviceModule*       deviceModule = nullptr;
    QueueModule*        queueModule = nullptr;
    MaterialManager*    materialManager = nullptr;
    AnimationManager*   animationManager = nullptr;

public:
    std::shared_ptr<GeometryComponent>  _Mesh = nullptr;
    MeshImportedType                    _meshImportedType;
    PRIMITIVE_TYPE                      _primitiveMeshType;
    std::string                         MeshFilePath;
    std::shared_ptr<Transform>          _Transform = nullptr;
    std::shared_ptr<Material>           _Material = nullptr;
    std::shared_ptr<PhysicBody>         physicBody = nullptr;
    std::shared_ptr<Collider>           collider = nullptr;
    std::shared_ptr<AnimationComponent> animationComponent = nullptr;
    std::shared_ptr<SkeletalComponent>  skeletalComponent = nullptr;
    std::shared_ptr<QECharacterController> characterController = nullptr;

    std::vector<std::shared_ptr<GameObject>>    childs;
    GameObject*     parent = nullptr;

private:
    void InitializeResources();
    bool IsRenderEnable();
    void InitializeGameObject(std::string meshPath, bool isMeshShading = false);
    void InitializeGameObject(PRIMITIVE_TYPE type, bool isMeshShading = false);

public:
    GameObject();
    GameObject(PRIMITIVE_TYPE type, bool isMeshShading = false);
    GameObject(std::string meshPath, bool isMeshShading = false);
    GameObject(const GameObjectDto& gameObjectDto);
    inline std::string ID() const { return id; }
    void Cleanup();
    virtual void CreateDrawCommand(VkCommandBuffer& commandBuffer, uint32_t idx);
    virtual void CreateShadowCommand(VkCommandBuffer& commandBuffer, uint32_t idx, VkPipelineLayout pipelineLayout);
    void AddMaterial(std::shared_ptr<Material> material_ptr);
    void AddPhysicBody(std::shared_ptr<PhysicBody> physicBody_ptr);
    void AddCollider(std::shared_ptr<Collider> collider_ptr);
    void AddAnimation(std::shared_ptr<Animation> animation_ptr);
    void AddCharacterController(std::shared_ptr<QECharacterController> characterController_ptr);
    void InitializePhysics();
    virtual bool IsValidRender();
    void UpdatePhysicTransform();

protected:
    virtual bool IsValidGameObject();
    void InitializeComponents();
    void InitializeAnimationComponent();
    bool CreateChildsGameObject(std::string pathfile);
    virtual void SetDrawCommand(VkCommandBuffer& commandBuffer, uint32_t idx, std::shared_ptr<Animator> animator_ptr = nullptr);
    void SetDrawShadowCommand(VkCommandBuffer& commandBuffer, uint32_t idx, VkPipelineLayout pipelineLayout, std::shared_ptr<Animator> animator_ptr = nullptr);
};

#endif
