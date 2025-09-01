#pragma once
#ifndef QE_GAME_OBJECT_H
#define QE_GAME_OBJECT_H

#include <CullingSceneManager.h>
#include <Numbered.h>
#include <GameObjectDto.h>
#include <QECharacterController.h>
#include <yaml-cpp/yaml.h>

typedef class QEGameObject QEGameObject;

class QEGameObject : Numbered
{
    friend class CullingSceneManager;

private:
    CullingSceneManager* cullingSceneManager = nullptr;
    std::vector<std::string> bindedMaterials;

protected:
    DeviceModule*       deviceModule = nullptr;
    QueueModule*        queueModule = nullptr;
    MaterialManager*    materialManager = nullptr;

public:
    std::string Name;
    std::list<std::shared_ptr<QEGameComponent>> components;
    std::vector<std::shared_ptr<QEMaterial>>    materials;
    std::vector<std::shared_ptr<QEGameObject>>    childs;
    QEGameObject*     parent = nullptr;

private:
    void InitializeResources();

public:
    QEGameObject();
    //QEGameObject(const GameObjectDto& gameObjectDto);
    inline std::string ID() const { return id; }

    void QEStart();
    void QEUpdate();
    void QEDestroy();

    YAML::Node ToYaml() const;
    static std::shared_ptr<QEGameObject> FromYaml(const YAML::Node& node);

    template<typename T>
    bool AddComponent(std::shared_ptr<T> component_ptr)
    {
        if (!component_ptr)
            return false;

        if constexpr (std::is_base_of_v<QEMaterial, T>)
        {
            const auto& newID = component_ptr->id;
            auto it = std::find_if(materials.begin(), materials.end(),
                [&](const std::shared_ptr<QEMaterial>& m) {
                    return m->id == newID;
                }
            );
            if (it != materials.end()) {
                // Ya hay un QEMaterial con ese ID: no lo añadimos
                return false;
            }

            materials.push_back(component_ptr);
            bindedMaterials.push_back(component_ptr->Name);
            return true;
        }
        else
        {
            auto it = std::find_if(
                components.begin(), components.end(),
                [&](const std::shared_ptr<QEGameComponent>& comp)
                {
                    return dynamic_cast<T*>(comp.get()) != nullptr;
                }
            );
            if (it != components.end())
                return false;

            components.push_back(component_ptr);
            component_ptr->BindGameObject(this);
            return true;
        }
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

    std::shared_ptr<QEMaterial> GetMaterial(const std::string& matID = "") const
    {
        if (materials.empty())
            return nullptr;

        if (matID == "")
        {
            return materials.front();
        }

        for (auto& mat : materials)
        {
            if (mat->Name == matID)
            {
                return mat;
            }
        }
        return nullptr;
    }

    const std::vector<std::shared_ptr<QEMaterial>>& GetMaterials() const
    {
        return materials;
    }
};

#endif
