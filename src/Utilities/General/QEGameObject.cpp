#include "QEGameObject.h"
#include <AnimationComponent.h>
#include <CullingSceneManager.h>

QEGameObject::QEGameObject()
{
    this->Name = "QEGameObject";
    this->childs.resize(0);
    this->InitializeResources();
}

void QEGameObject::QEStart()
{
    if (_isStarted)
    {
        return;
    }

    _isStarted = true;

    auto geometryComponent = this->GetComponent<QEGeometryComponent>();
    auto transform = this->GetComponent<QETransform>();

    if (geometryComponent != nullptr)
    {
        geometryComponent->QEStart();

        // Set the AABB component
        auto mesh = geometryComponent->GetMesh();

        auto cullingSceneManager = CullingSceneManager::getInstance();
        auto aabbCulling = cullingSceneManager->GenerateAABB(mesh->BoundingBox, transform);
        this->AddComponent<AABBObject>(aabbCulling);

        //Set the materials
        std::set<std::string> matIDs;

        if (mesh->MaterialRel.empty())
        {
            auto mat = this->GetMaterial();
            if (mat != nullptr)
            {
                mesh->MaterialRel.push_back(mat->Name);
            }
            else
            {
                std::shared_ptr<QEMaterial> material = this->materialManager->GetMaterial(bindedMaterials.front());
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

        //Set the animations
        if (mesh->AnimationData.size())
        {
            auto animationComponent = this->GetComponent<AnimationComponent>();

            if (animationComponent == nullptr)
            {
                this->AddComponent<AnimationComponent>(std::make_shared<AnimationComponent>());
                animationComponent = this->GetComponent<AnimationComponent>();
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

    for (auto gameComponent : this->components)
    {
        gameComponent->QEStart();
    }

    for (auto mat : this->materials)
    {
        mat->InitializeMaterialData();
    }
}

void QEGameObject::QEUpdate()
{
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

YAML::Node QEGameObject::ToYaml() const  
{  
    YAML::Node node;  
    node["id"] = this->id;  
    node["name"] = this->Name;  

    auto comps = node["components"];
    for (const auto& comp : components)  
        comps.push_back(serializeComponent(comp.get()));

    auto mats = node["materials"];
    for (const auto& mat : materials)
        mats.push_back(mat->Name);

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
            go->components.push_back(sptr);
        }
    }

    if (node["materials"] && node["materials"].IsSequence())
    {
        for (const auto& matnode : node["materials"])
        {
            go->bindedMaterials.push_back(matnode.as<std::string>());
        }
    }

    if (node["children"] && node["children"].IsSequence())
    {
        for (const auto& chnode : node["children"])
        {
            auto child = QEGameObject::FromYaml(chnode);
            if (child)
            {
                child->parent = go.get();
                go->childs.push_back(child);
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
