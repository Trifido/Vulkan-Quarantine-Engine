#include "QEGameObject.h"

QEGameObject::QEGameObject()
{
    this->childs.resize(0);
    this->InitializeResources();
}

/*
QEGameObject::QEGameObject(const GameObjectDto& gameObjectDto) : Numbered(gameObjectDto.Id)
{
    PRIMITIVE_TYPE primitiveType = static_cast<PRIMITIVE_TYPE>(gameObjectDto.MeshPrimitiveType);
    MeshImportedType meshType = static_cast<MeshImportedType>(gameObjectDto.MeshImportedType);

    this->bindMaterialName = gameObjectDto.BindMaterialName;

    if (meshType == MeshImportedType::PRIMITIVE_GEO && primitiveType != PRIMITIVE_TYPE::NONE_TYPE)
    {
        this->InitializeGameObject(primitiveType);
    }
    else if (meshType != MeshImportedType::PRIMITIVE_GEO)
    {
        this->InitializeGameObject(gameObjectDto.MeshPath);
    }
    else
    {
        return;
    }

    this->_meshImportedType = static_cast<MeshImportedType>(gameObjectDto.MeshImportedType);
    this->GetComponent<Transform>()->SetModel(gameObjectDto.WorldTransform);
}
*/

void QEGameObject::QEStart()
{
    auto geometryComponent = this->GetComponent<QEGeometryComponent>();
    auto transform = this->GetComponent<Transform>();

    if (geometryComponent == nullptr)
    {
        return;
    }

    geometryComponent->QEStart();

    // Set the AABB component
    auto mesh = geometryComponent->GetMesh();
    auto aabbCulling = this->cullingSceneManager->GenerateAABB(mesh->BoundingBox, transform);
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
        this->AddComponent<AnimationComponent>(std::make_shared<AnimationComponent>());

        for (auto anim : mesh->AnimationData)
        {
            if (anim.m_Duration > 0.0f)
            {
                this->AddAnimation(std::make_shared<Animation>(anim));
            }
        }

        this->InitializeAnimationComponent();
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
    for (auto gameComponent : this->components)
    {
        gameComponent->QEDestroy();
    }
}

void QEGameObject::InitializeResources()
{
    this->deviceModule = DeviceModule::getInstance();
    this->queueModule = QueueModule::getInstance();
    this->materialManager = MaterialManager::getInstance();
    this->cullingSceneManager = CullingSceneManager::getInstance();

    AddComponent<Transform>(std::make_shared<Transform>());
}

void QEGameObject::AddAnimation(std::shared_ptr<Animation> animation_ptr)
{
    auto animationComponent = this->GetComponent<AnimationComponent>();
    if (animationComponent == nullptr)
    {
        this->AddComponent<AnimationComponent>(std::make_shared<AnimationComponent>());
    }

    animationComponent = this->GetComponent<AnimationComponent>();
    animationComponent->AddAnimation(animation_ptr);

    if (this->animationManager == nullptr)
    {
        this->animationManager = AnimationManager::getInstance();
        this->animationManager->AddAnimationComponent(this->id, animationComponent);
    }
}

void QEGameObject::InitializeAnimationComponent()
{
    auto animationComponent = this->GetComponent<AnimationComponent>();
    if (animationComponent != nullptr)
    {
        auto meshComponent = this->GetComponent<QEGeometryComponent>();
        auto mesh = meshComponent->GetMesh();

        if (mesh->MeshData.empty())
        {
            return;
        }

        std::vector<std::string> idMeshes;
        for (unsigned int i = 0; i < mesh->MeshData.size(); i++)
        {
            idMeshes.push_back(std::to_string(i));
        }

        animationComponent->animator->InitializeComputeNodes(idMeshes);

        for (unsigned int i = 0; i < mesh->MeshData.size(); i++)
        {
            animationComponent->animator->SetVertexBufferInComputeNode(idMeshes[i], meshComponent->vertexBuffer[i], meshComponent->animationBuffer[i], mesh->MeshData[i].NumVertices);
        }

        animationComponent->animator->InitializeDescriptorsComputeNodes();
    }
}

bool QEGameObject::IsValidRender()
{
    auto transform = this->GetComponent<Transform>();
    if (transform == nullptr)
        return false;

    auto material = this->GetMaterial();
    auto mesh = this->GetComponent<QEGeometryComponent>();
    if (mesh != nullptr && material != nullptr)
        return true;

    for (auto child : childs)
    {
        auto childMat = child->GetMaterial();
        auto childMesh = child->GetComponent<QEGeometryComponent>();
        if (childMesh != nullptr && childMat != nullptr)
            return true;
    }

    return false;
}

bool QEGameObject::IsValidGameObject()
{
    auto transform = this->GetComponent<Transform>();
    return transform != nullptr;
}
