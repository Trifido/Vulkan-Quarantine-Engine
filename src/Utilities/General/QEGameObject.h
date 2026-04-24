#pragma once
#ifndef QE_GAME_OBJECT_H
#define QE_GAME_OBJECT_H

#include <Numbered.h>
#include <GameObjectDto.h>
#include <QECharacterController.h>
#include <QEGeometryComponent.h>
#include <QEMeshRenderer.h>
#include <Material.h>
#include <yaml-cpp/yaml.h>
#include <string>

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
    unsigned int UpdateOrder = 0;
    std::list<std::shared_ptr<QEGameComponent>> components;
    std::vector<std::shared_ptr<QEMaterial>>    materials;
    std::vector<std::shared_ptr<QEGameObject>>    childs;
    QEGameObject*     parent = nullptr;

private:
    void InitializeResources();

public:
    QEGameObject(std::string name = "");
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

            if (it != materials.end())
                return false;

            materials.push_back(component_ptr);
            bindedMaterials.push_back(component_ptr->Name);
            return true;
        }
        else
        {
            return AddComponent(std::static_pointer_cast<QEGameComponent>(component_ptr));
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

    void SetMaterial(const std::shared_ptr<QEMaterial>& material)
    {
        if (!material)
            return;

        materials.clear();
        bindedMaterials.clear();

        materials.push_back(material);
        bindedMaterials.push_back(material->Name);

        if (auto geometry = GetComponent<QEGeometryComponent>())
        {
            if (auto mesh = geometry->GetMesh())
            {
                mesh->MaterialRel.resize(mesh->MeshData.size(), material->Name);
            }
        }

        if (auto meshRenderer = GetComponent<QEMeshRenderer>())
        {
            meshRenderer->RefreshMaterials();
        }
    }

    void SetMaterialAt(size_t materialIndex, const std::shared_ptr<QEMaterial>& material)
    {
        if (!material)
            return;

        if (materials.size() <= materialIndex)
        {
            materials.resize(materialIndex + 1);
        }

        if (bindedMaterials.size() <= materialIndex)
        {
            bindedMaterials.resize(materialIndex + 1);
        }

        const std::string previousMaterialName =
            !bindedMaterials[materialIndex].empty()
            ? bindedMaterials[materialIndex]
            : (materials[materialIndex] ? materials[materialIndex]->Name : "");

        materials[materialIndex] = material;
        bindedMaterials[materialIndex] = material->Name;
        material->InitializeMaterialData();

        if (auto geometry = GetComponent<QEGeometryComponent>())
        {
            if (auto mesh = geometry->GetMesh())
            {
                if (mesh->MaterialRel.empty())
                {
                    mesh->MaterialRel.resize(mesh->MeshData.size(), material->Name);
                }
                else if (!previousMaterialName.empty())
                {
                    for (auto& materialRel : mesh->MaterialRel)
                    {
                        if (materialRel == previousMaterialName)
                        {
                            materialRel = material->Name;
                        }
                    }
                }
                else if (materialIndex < mesh->MaterialRel.size())
                {
                    mesh->MaterialRel[materialIndex] = material->Name;
                }
            }
        }

        if (auto meshRenderer = GetComponent<QEMeshRenderer>())
        {
            meshRenderer->RefreshMaterials();
        }
    }

    bool AddMaterialBinding(const std::shared_ptr<QEMaterial>& material)
    {
        if (!material)
            return false;

        const auto duplicated = std::find_if(
            materials.begin(),
            materials.end(),
            [&](const std::shared_ptr<QEMaterial>& current)
            {
                return current && current->id == material->id;
            });

        if (duplicated != materials.end())
            return false;

        materials.push_back(material);
        bindedMaterials.push_back(material->Name);
        material->InitializeMaterialData();

        if (auto geometry = GetComponent<QEGeometryComponent>())
        {
            if (auto mesh = geometry->GetMesh())
            {
                if (mesh->MaterialRel.size() < mesh->MeshData.size())
                {
                    mesh->MaterialRel.push_back(material->Name);
                }
            }
        }

        if (auto meshRenderer = GetComponent<QEMeshRenderer>())
        {
            meshRenderer->RefreshMaterials();
        }

        return true;
    }

    bool RemoveMaterialAt(size_t materialIndex)
    {
        if (materialIndex >= materials.size() || materialIndex >= bindedMaterials.size())
            return false;

        const std::string removedMaterialName = bindedMaterials[materialIndex];

        materials.erase(materials.begin() + static_cast<std::ptrdiff_t>(materialIndex));
        bindedMaterials.erase(bindedMaterials.begin() + static_cast<std::ptrdiff_t>(materialIndex));

        if (auto geometry = GetComponent<QEGeometryComponent>())
        {
            if (auto mesh = geometry->GetMesh())
            {
                const std::string replacementName =
                    materials.empty() ? "" : materials.front()->Name;

                for (auto& materialRel : mesh->MaterialRel)
                {
                    if (materialRel == removedMaterialName)
                    {
                        materialRel = replacementName;
                    }
                }

                while (!mesh->MaterialRel.empty() && mesh->MaterialRel.back().empty())
                {
                    mesh->MaterialRel.pop_back();
                }
            }
        }

        if (auto meshRenderer = GetComponent<QEMeshRenderer>())
        {
            meshRenderer->RefreshMaterials();
        }

        return true;
    }

    const std::vector<std::shared_ptr<QEMaterial>>& GetMaterials() const
    {
        return materials;
    }

    bool AddComponent(const std::shared_ptr<QEGameComponent>& component_ptr)
    {
        if (!component_ptr)
            return false;

        const std::string newTypeName = component_ptr->getTypeName();

        auto it = std::find_if(
            components.begin(),
            components.end(),
            [&](const std::shared_ptr<QEGameComponent>& comp)
            {
                return comp && comp->getTypeName() == newTypeName;
            }
        );

        if (it != components.end())
            return false;

        components.push_back(component_ptr);
        component_ptr->BindGameObject(this);

        if (auto transform = std::dynamic_pointer_cast<QETransform>(component_ptr))
        {
            transform->SetSelf(transform);
        }

        return true;
    }

    bool RemoveComponent(const std::shared_ptr<QEGameComponent>& component_ptr);
    bool RemoveComponentByType(const std::string& typeName);

    template<typename T>
    bool RemoveComponent()
    {
        for (auto it = components.begin(); it != components.end(); ++it)
        {
            if (std::dynamic_pointer_cast<T>(*it))
            {
                (*it)->QEDestroy();
                (*it)->Owner = nullptr;
                components.erase(it);
                return true;
            }
        }

        return false;
    }
};

#endif
