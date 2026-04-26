#include "QEGameObject.h"
#include <QEAnimationComponent.h>
#include <CullingSceneManager.h>
#include <QEAnimationGraphAssetHelper.h>
#include <QEProjectManager.h>
#include <cctype>

namespace
{
    std::string SanitizeMaterialInstanceToken(const std::string& value)
    {
        std::string sanitized;
        sanitized.reserve(value.size());

        for (unsigned char c : value)
        {
            if (std::isalnum(c) || c == '_' || c == '-')
            {
                sanitized.push_back(static_cast<char>(c));
            }
            else
            {
                sanitized.push_back('_');
            }
        }

        if (sanitized.empty())
            sanitized = "MaterialCopy";

        return sanitized;
    }
}

QEGameObject::QEGameObject(std::string name)
{
    this->Name = (name.empty()) ? ID() : name;
    this->childs.resize(0);
    this->InitializeResources();
}

void QEGameObject::QEStart()
{
    if (!IsActiveInHierarchy())
    {
        return;
    }

    if (_isStarted)
    {
        return;
    }

    _isStarted = true;

    auto geometryComponent = this->GetComponent<QEGeometryComponent>();
    auto transform = this->GetComponent<QETransform>();

    // Set Visual Components
    if (geometryComponent != nullptr)
    {
        geometryComponent->QEStart();

        auto mesh = geometryComponent->GetMesh();
        if (mesh != nullptr)
        {
            // Set the AABB component
            auto cullingSceneManager = CullingSceneManager::getInstance();
            auto aabbCulling = cullingSceneManager->GenerateAABB(mesh->BoundingBox, transform);
            this->AddComponent<AABBObject>(aabbCulling);

            // Set the materials
            if (mesh->MaterialRel.empty())
            {
                auto mat = this->GetMaterial();
                if (mat != nullptr)
                {
                    mesh->MaterialRel.push_back(mat->Name);
                }
                else if (!materialBindings.empty())
                {
                    std::shared_ptr<QEMaterial> material = this->materialManager->GetMaterial(ResolveMaterialBindingName(0));
                    if (material != nullptr)
                    {
                        mesh->MaterialRel.push_back(material->Name);
                        this->AddComponent<QEMaterial>(material);
                    }
                }
            }
            else
            {
                for (auto matID : mesh->MaterialRel)
                {
                    std::shared_ptr<QEMaterial> material = this->materialManager->GetMaterial(matID);
                    if (material != nullptr)
                    {
                        this->AddComponent<QEMaterial>(material);
                    }
                }
            }

            // Set the animations
            if (!mesh->AnimationData.empty())
            {
                auto animationComponent = this->GetComponent<QEAnimationComponent>();

                if (animationComponent == nullptr)
                {
                    this->AddComponent<QEAnimationComponent>(std::make_shared<QEAnimationComponent>());
                    animationComponent = this->GetComponent<QEAnimationComponent>();
                }

                for (auto anim : mesh->AnimationData)
                {
                    if (anim.m_Duration > 0.0f)
                    {
                        animationComponent->AddAnimation(std::make_shared<Animation>(anim));
                    }
                }
            }
        }
    }

    for (auto gameComponent : this->components)
    {
        gameComponent->QEStart();
    }

    for (auto mat : this->materials)
    {
        mat->InitializeMaterialData();
    }
}

void QEGameObject::QEInit()
{
    if (!IsActiveInHierarchy())
    {
        return;
    }

    for (auto gameComponent : this->components)
    {
        if (!gameComponent->QEStarted() || gameComponent->QEInitialized())
        {
            continue;
        }

        gameComponent->QEInit();
    }
}

void QEGameObject::QEUpdate()
{
    if (!IsActiveInHierarchy())
    {
        return;
    }

    for (auto gameComponent : this->components)
    {
        if (!gameComponent->QEInitialized())
        {
            gameComponent->QEInit();
        }

        gameComponent->QEUpdate();
    }
}

void QEGameObject::QEDestroy()
{
    if (_isDestroyed)
    {
        return;
    }

    _isDestroyed = true;

    for (auto gameComponent : this->components)
    {
        gameComponent->QEDestroy();
    }
}

bool QEGameObject::IsActiveInHierarchy() const
{
    if (!QEActive)
    {
        return false;
    }

    return parent == nullptr || parent->IsActiveInHierarchy();
}

YAML::Node QEGameObject::ToYaml() const  
{  
    YAML::Node node;  
    node["id"] = this->id;  
    node["name"] = this->Name;  
    node["active"] = this->QEActive;

    auto comps = node["components"];
    for (const auto& comp : components)
    {
        if (!comp->IsSerializable())
            continue;

        if (auto* animationComponent = dynamic_cast<QEAnimationComponent*>(comp.get()))
        {
            comps.push_back(QEAnimationGraphAssetHelper::SerializeAnimationComponentReference(*animationComponent));
        }
        else
        {
            comps.push_back(serializeComponent(comp.get()));
        }
    }

    auto mats = node["materials"];
    for (size_t materialIndex = 0; materialIndex < materials.size(); ++materialIndex)
    {
        YAML::Node materialNode;
        materialNode["Source"] = ResolveMaterialSourceName(materialIndex);
        materialNode["Bound"] = ResolveMaterialBindingName(materialIndex);
        materialNode["UseCopy"] = IsMaterialUsingCopy(materialIndex);
        mats.push_back(materialNode);
    }

    auto chs = node["children"];
    for (const auto& ch : childs)
        chs.push_back(ch->ToYaml());

    return node;  
}

std::shared_ptr<QEGameObject> QEGameObject::FromYaml(const YAML::Node& node)
{
    if (!node) return nullptr;

    std::shared_ptr<QEGameObject> go = std::make_shared<QEGameObject>();
    go->components.clear();

    if (node["id"])   go->id = node["id"].as<std::string>();
    if (node["name"]) go->Name = node["name"].as<std::string>();
    if (node["active"]) go->QEActive = node["active"].as<bool>();

    if (node["components"] && node["components"].IsSequence())
    {
        for (const auto& cnode : node["components"])
        {
            // Esperamos "type" como en serializeComponent
            std::string typeName = cnode["type"] ? cnode["type"].as<std::string>() : "";
            if (typeName.empty()) continue;

            auto& reg = getFactoryRegistry();
            auto it = reg.find(typeName);
            if (it == reg.end())
            {
                // tipo no registrado -> lo ignoramos o avisamos
                continue;
            }

            std::unique_ptr<QEGameComponent> uptr = it->second(); // createInstance()
            if (!uptr) continue;

            // rellena campos
            deserializeComponent(uptr.get(), cnode);

            // binario: convertir a shared_ptr y bind
            std::shared_ptr<QEGameComponent> sptr(uptr.release());
            sptr->BindGameObject(go.get());

            if (auto animationComponent = std::dynamic_pointer_cast<QEAnimationComponent>(sptr))
            {
                QEAnimationGraphAssetHelper::LoadAnimationComponentFromReference(*animationComponent, cnode);
            }

            if (auto tr = std::dynamic_pointer_cast<QETransform>(sptr))
            {
                go->AddComponent<QETransform>(tr);
            }
            else
            {
                go->components.push_back(sptr);
            }
        }
    }

    if (node["materials"] && node["materials"].IsSequence())
    {
        for (const auto& matnode : node["materials"])
        {
            MaterialBindingInfo binding;

            if (matnode.IsScalar())
            {
                binding.SourceMaterialName = matnode.as<std::string>();
                binding.BoundMaterialName = binding.SourceMaterialName;
            }
            else
            {
                binding.SourceMaterialName = matnode["Source"] ? matnode["Source"].as<std::string>() : "";
                binding.BoundMaterialName = matnode["Bound"] ? matnode["Bound"].as<std::string>() : "";
                binding.UseCopy = matnode["UseCopy"] ? matnode["UseCopy"].as<bool>() : false;

                if (binding.SourceMaterialName.empty())
                    binding.SourceMaterialName = binding.BoundMaterialName;

                if (binding.BoundMaterialName.empty())
                    binding.BoundMaterialName = binding.SourceMaterialName;
            }

            go->materialBindings.push_back(binding);
        }
    }

    if (node["children"] && node["children"].IsSequence())
    {
        for (const auto& chnode : node["children"])
        {
            auto child = QEGameObject::FromYaml(chnode);
            if (child)
            {
                go->AddChild(child, /*keepWorldTransform=*/false);
            }
        }
    }

    return go;
}

void QEGameObject::AddChild(const std::shared_ptr<QEGameObject>& child, bool keepWorldTransform)
{
    if (!child) return;

    if (child.get() == this ||
        std::find_if(childs.begin(), childs.end(),
            [&](auto& c) { return c.get() == child.get(); }) != childs.end())
        return;

    child->parent = this;
    childs.push_back(child);

    auto transform = this->GetComponent<QETransform>();
    auto childTransform = child->GetComponent<QETransform>();

    if (transform && childTransform)
    {
        childTransform->SetParent(transform, keepWorldTransform);
    }

    //child->QEStart();
}

void QEGameObject::RemoveChild(const std::shared_ptr<QEGameObject>& child)
{
    if (!child) return;

    auto it = std::remove_if(childs.begin(), childs.end(),
        [&](auto& c) { return c.get() == child.get(); });

    if (it != childs.end())
    {
        childs.erase(it, childs.end());
        child->parent = nullptr;

        auto childTransform = child->GetComponent<QETransform>();
        if (childTransform)
        {
            childTransform->SetParent(nullptr, true);
        }
    }
}

void QEGameObject::InitializeResources()
{
    this->deviceModule = DeviceModule::getInstance();
    this->queueModule = QueueModule::getInstance();
    this->materialManager = MaterialManager::getInstance();

    AddComponent<QETransform>(std::make_shared<QETransform>());
}

void QEGameObject::EnsureMaterialBindingIndex(size_t materialIndex)
{
    if (materialBindings.size() <= materialIndex)
    {
        materialBindings.resize(materialIndex + 1);
    }
}

void QEGameObject::UpdateMaterialBinding(
    size_t materialIndex,
    const std::string& sourceMaterialName,
    const std::string& boundMaterialName,
    bool useCopy)
{
    EnsureMaterialBindingIndex(materialIndex);
    materialBindings[materialIndex].SourceMaterialName = sourceMaterialName;
    materialBindings[materialIndex].BoundMaterialName = boundMaterialName;
    materialBindings[materialIndex].UseCopy = useCopy;
}

std::string QEGameObject::ResolveMaterialBindingName(size_t materialIndex) const
{
    if (materialIndex >= materialBindings.size())
        return "";

    if (!materialBindings[materialIndex].BoundMaterialName.empty())
        return materialBindings[materialIndex].BoundMaterialName;

    if (!materialBindings[materialIndex].SourceMaterialName.empty())
        return materialBindings[materialIndex].SourceMaterialName;

    if (materialIndex < materials.size() && materials[materialIndex])
        return materials[materialIndex]->Name;

    return "";
}

std::string QEGameObject::ResolveMaterialSourceName(size_t materialIndex) const
{
    if (materialIndex < materialBindings.size() &&
        !materialBindings[materialIndex].SourceMaterialName.empty())
    {
        return materialBindings[materialIndex].SourceMaterialName;
    }

    return ResolveMaterialBindingName(materialIndex);
}

std::string QEGameObject::GetBoundMaterialFilePath(size_t materialIndex) const
{
    if (materialIndex < materials.size() && materials[materialIndex])
    {
        return materials[materialIndex]->GetMaterialFilePath();
    }

    const std::string boundMaterialName = ResolveMaterialBindingName(materialIndex);
    if (boundMaterialName.empty() || !materialManager)
        return "";

    auto material = materialManager->GetMaterial(boundMaterialName);
    return material ? material->GetMaterialFilePath() : "";
}

void QEGameObject::DeleteOwnedMaterialCopy(const std::string& materialPath)
{
    if (!materialPath.empty())
    {
        QEProjectManager::DeletePath(materialPath, false);
    }
}

bool QEGameObject::IsMaterialUsingCopy(size_t materialIndex) const
{
    return materialIndex < materialBindings.size() && materialBindings[materialIndex].UseCopy;
}

bool QEGameObject::SetMaterialUseCopy(size_t materialIndex, bool useCopy)
{
    if (materialIndex >= materials.size() || materialIndex >= materialBindings.size())
        return false;

    auto currentMaterial = materials[materialIndex];
    if (!currentMaterial || !materialManager)
        return false;

    if (materialBindings[materialIndex].UseCopy == useCopy)
        return false;

    if (useCopy)
    {
        const std::string sourceMaterialName = ResolveMaterialSourceName(materialIndex);
        const std::string copyBaseName =
            SanitizeMaterialInstanceToken(Name) + "_" +
            SanitizeMaterialInstanceToken(sourceMaterialName) + "_" +
            std::to_string(materialIndex);
        const std::filesystem::path copyPath =
            QEProjectManager::GetMaterialFolderPath() / "Instances" / (copyBaseName + ".qemat");

        auto materialCopy = currentMaterial->CreateMaterialInstance(
            copyBaseName,
            QEProjectManager::ToProjectRelativePath(copyPath));
        if (!materialCopy)
            return false;

        materialManager->AddMaterial(materialCopy);
        materialCopy->SaveMaterialFile();

        SetMaterialAt(materialIndex, materialCopy);
        UpdateMaterialBinding(materialIndex, sourceMaterialName, materialCopy->Name, true);
        return true;
    }

    const std::string sourceMaterialName = ResolveMaterialSourceName(materialIndex);
    auto sourceMaterial = materialManager->GetMaterial(sourceMaterialName);
    if (!sourceMaterial)
        return false;

    SetMaterialAt(materialIndex, sourceMaterial);
    UpdateMaterialBinding(materialIndex, sourceMaterialName, sourceMaterial->Name, false);
    return true;
}

bool QEGameObject::RemoveComponent(const std::shared_ptr<QEGameComponent>& component_ptr)
{
    if (!component_ptr)
        return false;

    if (std::dynamic_pointer_cast<QETransform>(component_ptr))
        return false;

    auto it = std::find_if(
        components.begin(),
        components.end(),
        [&](const std::shared_ptr<QEGameComponent>& comp)
        {
            return comp == component_ptr;
        }
    );

    if (it == components.end())
        return false;

    (*it)->QEDestroy();
    (*it)->Owner = nullptr;
    components.erase(it);
    return true;
}

bool QEGameObject::RemoveComponentByType(const std::string& typeName)
{
    if (typeName.empty())
        return false;

    if (typeName == "QETransform")
        return false;

    auto it = std::find_if(
        components.begin(),
        components.end(),
        [&](const std::shared_ptr<QEGameComponent>& comp)
        {
            return comp && comp->getTypeName() == typeName;
        }
    );

    if (it == components.end())
        return false;

    (*it)->QEDestroy();
    (*it)->Owner = nullptr;
    components.erase(it);
    return true;
}
