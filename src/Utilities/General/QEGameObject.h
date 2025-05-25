#pragma once
#ifndef QE_GAME_OBJECT_H
#define QE_GAME_OBJECT_H

#include <AnimationManager.h>
#include <CullingSceneManager.h>
#include <Numbered.h>
#include <GameObjectDto.h>
#include <QECharacterController.h>

typedef class QEGameObject QEGameObject;

class QEGameObject : Numbered
{
    friend class CullingSceneManager;

private:
    CullingSceneManager* cullingSceneManager = nullptr;
    std::string bindMaterialName = "NULL_MATERIAL";

protected:
    DeviceModule*       deviceModule = nullptr;
    QueueModule*        queueModule = nullptr;
    MaterialManager*    materialManager = nullptr;
    AnimationManager*   animationManager = nullptr;

public:
    std::list<std::shared_ptr<QEGameComponent>> components;
    std::vector<std::shared_ptr<QEGameObject>>    childs;
    QEGameObject*     parent = nullptr;

private:
    void InitializeResources();

public:
    QEGameObject();
    //QEGameObject(const GameObjectDto& gameObjectDto);
    inline std::string ID() const { return id; }
    void AddAnimation(std::shared_ptr<Animation> animation_ptr);
    void AddCharacterController(std::shared_ptr<QECharacterController> characterController_ptr);
    void InitializePhysics();
    virtual bool IsValidRender();
    void UpdatePhysicTransform();

    void QEInitialize();
    void QEUpdate() {}
    void QERelease();

    template<typename T>
    bool AddComponent(std::shared_ptr<T> component_ptr)
    {
        if (component_ptr == nullptr)
            return false;

        if (std::find_if(components.begin(), components.end(), [&](const std::shared_ptr<QEGameComponent>& comp) {
            return dynamic_cast<T*>(comp.get()) != nullptr;
            }) != components.end())
        {
            return false;
        }

        components.push_back(component_ptr);
        component_ptr->BindGameObject(this);

        return true;
    }

    template<typename T>
    std::shared_ptr<T> GetComponent()
    {
        for (auto& comp : components)
        {
            if (auto ptr = std::dynamic_pointer_cast<T>(comp))
            {
                return ptr;
            }
        }

        return nullptr;
    }

protected:
    virtual bool IsValidGameObject();
    void InitializeAnimationComponent();
};

#endif
