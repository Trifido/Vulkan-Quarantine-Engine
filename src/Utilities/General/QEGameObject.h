#pragma once
#ifndef QE_GAME_OBJECT_H
#define QE_GAME_OBJECT_H

#include <Numbered.h>
#include <GameObjectDto.h>
#include <QECharacterController.h>
#include <Material.h>
#include <yaml-cpp/yaml.h>

typedef class QEGameObject QEGameObject;

class QEGameObject : Numbered
{
    friend class CullingSceneManager;

private:
    bool _isStarted = false;
    bool _isDestroyed = false;
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
    QEGameObject(string name = "");
    //QEGameObject(const GameObjectDto& gameObjectDto);
    inline std::string ID() const { return id; }

    void QEStart();
    void QEInit();
    void QEUpdate();
    void QEDestroy();

    YAML::Node ToYaml() const;
    static std::shared_ptr<QEGameObject> FromYaml(const YAML::Node& node);

    void AddChild(const std::shared_ptr<QEGameObject>& child, bool keepWorldTransform);
    void RemoveChild(const std::shared_ptr<QEGameObject>& child);

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

    template<typename T>
    std::shared_ptr<T> GetComponentInChildren(bool includeSelf = true)
    {
        if (includeSelf)
        {
            if (auto found = GetComponent<T>())
                return found;
        }

        auto tr = GetComponent<QETransform>();
        if (!tr)
            return nullptr;

        for (auto& child : tr->GetChildren())
        {
            if (!child)
                continue;

            auto childGO = child->Owner;
            if (!childGO)
                continue;

            if (auto found = childGO->GetComponent<T>())
                return found;

            if (auto foundDeep = childGO->GetComponentInChildren<T>(false))
                return foundDeep;
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
