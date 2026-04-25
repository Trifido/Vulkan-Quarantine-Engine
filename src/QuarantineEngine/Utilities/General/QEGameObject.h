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

public:
    struct MaterialBindingInfo
    {
        std::string SourceMaterialName;
        std::string BoundMaterialName;
        bool UseCopy = false;
    };

private:
    bool _isStarted = false;
    bool _isDestroyed = false;
    std::vector<MaterialBindingInfo> materialBindings;

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
    void EnsureMaterialBindingIndex(size_t materialIndex);
    void UpdateMaterialBinding(
        size_t materialIndex,
        const std::string& sourceMaterialName,
        const std::string& boundMaterialName,
        bool useCopy);
    std::string ResolveMaterialBindingName(size_t materialIndex) const;
    std::string ResolveMaterialSourceName(size_t materialIndex) const;
    std::string GetBoundMaterialFilePath(size_t materialIndex) const;
    void DeleteOwnedMaterialCopy(const std::string& materialPath);

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

            const size_t materialIndex = materials.size();
            materials.push_back(component_ptr);

            if (materialBindings.size() <= materialIndex)
            {
                MaterialBindingInfo binding;
                binding.SourceMaterialName = component_ptr->Name;
                binding.BoundMaterialName = component_ptr->Name;
                materialBindings.push_back(binding);
            }
            else
            {
                if (materialBindings[materialIndex].SourceMaterialName.empty())
                {
                    materialBindings[materialIndex].SourceMaterialName = component_ptr->Name;
                }

                if (materialBindings[materialIndex].BoundMaterialName.empty())
                {
                    materialBindings[materialIndex].BoundMaterialName = component_ptr->Name;
                }
            }

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

        std::vector<std::string> removedCopyPaths;
        for (size_t materialIndex = 0; materialIndex < materialBindings.size(); ++materialIndex)
        {
            if (materialBindings[materialIndex].UseCopy)
            {
                removedCopyPaths.push_back(GetBoundMaterialFilePath(materialIndex));
            }
        }

        materials.clear();
        materialBindings.clear();

        materials.push_back(material);
        MaterialBindingInfo binding;
        binding.SourceMaterialName = material->Name;
        binding.BoundMaterialName = material->Name;
        materialBindings.push_back(binding);

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

        for (const auto& removedCopyPath : removedCopyPaths)
        {
            DeleteOwnedMaterialCopy(removedCopyPath);
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

        const std::string previousMaterialName =
            !ResolveMaterialBindingName(materialIndex).empty()
            ? ResolveMaterialBindingName(materialIndex)
            : (materials[materialIndex] ? materials[materialIndex]->Name : "");
        const bool removedCopy = materialIndex < materialBindings.size() && materialBindings[materialIndex].UseCopy;
        const std::string removedCopyPath = removedCopy ? GetBoundMaterialFilePath(materialIndex) : "";
        const std::string sourceMaterialName =
            materialIndex < materialBindings.size() && !materialBindings[materialIndex].SourceMaterialName.empty()
            ? materialBindings[materialIndex].SourceMaterialName
            : material->Name;

        materials[materialIndex] = material;
        UpdateMaterialBinding(materialIndex, sourceMaterialName, material->Name, false);
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

        if (removedCopy)
        {
            DeleteOwnedMaterialCopy(removedCopyPath);
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
        MaterialBindingInfo binding;
        binding.SourceMaterialName = material->Name;
        binding.BoundMaterialName = material->Name;
        materialBindings.push_back(binding);
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
        if (materialIndex >= materials.size() || materialIndex >= materialBindings.size())
            return false;

        const MaterialBindingInfo removedBinding = materialBindings[materialIndex];
        const std::string removedMaterialName = removedBinding.BoundMaterialName;
        const std::string removedCopyPath = removedBinding.UseCopy ? GetBoundMaterialFilePath(materialIndex) : "";

        materials.erase(materials.begin() + static_cast<std::ptrdiff_t>(materialIndex));
        materialBindings.erase(materialBindings.begin() + static_cast<std::ptrdiff_t>(materialIndex));

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

        if (removedBinding.UseCopy)
        {
            DeleteOwnedMaterialCopy(removedCopyPath);
        }

        return true;
    }

    const std::vector<std::shared_ptr<QEMaterial>>& GetMaterials() const
    {
        return materials;
    }

    const std::vector<MaterialBindingInfo>& GetMaterialBindings() const
    {
        return materialBindings;
    }

    bool IsMaterialUsingCopy(size_t materialIndex) const;
    bool SetMaterialUseCopy(size_t materialIndex, bool useCopy);

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



namespace QE
{
    using ::QEGameObject;
} // namespace QE
// QE namespace aliases
#endif
