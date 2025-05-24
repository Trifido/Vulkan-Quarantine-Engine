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

void QEGameObject::QEInitialize()
{
    auto geometryComponent = this->GetComponent<QEGeometryComponent>();
    auto transform = this->GetComponent<Transform>();

    if (geometryComponent == nullptr)
    {
        return;
    }

    geometryComponent->BuildMesh();

    // Set the AABB component
    auto mesh = geometryComponent->GetMesh();
    auto aabbCulling = this->cullingSceneManager->GenerateAABB(mesh->BoundingBox, transform);
    this->AddComponent<AABBObject>(aabbCulling);

    //Set the materials
    std::set<std::string> matIDs;

    for (auto m : mesh->MeshData)
    {
        matIDs.insert(m.MaterialID);
    }

    for (auto matID : matIDs)
    {
        std::shared_ptr<Material> material = this->materialManager->GetMaterial(matID);
        if (material != nullptr)
        {
            this->AddComponent<Material>(material);
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
}

void QEGameObject::InitializeResources()
{
    this->deviceModule = DeviceModule::getInstance();
    this->queueModule = QueueModule::getInstance();
    this->materialManager = MaterialManager::getInstance();
    this->cullingSceneManager = CullingSceneManager::getInstance();

    AddComponent<Transform>(std::make_shared<Transform>());
}

void QEGameObject::Cleanup()
{
    auto mesh = this->GetComponent<QEGeometryComponent>();
    if (mesh != nullptr)
    {
        mesh->cleanup();
    }

    auto animationComponent = this->GetComponent<AnimationComponent>();
    if (animationComponent != nullptr)
    {
        animationComponent->CleanLastResources();
    } 
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

void QEGameObject::AddCharacterController(std::shared_ptr<QECharacterController> characterController_ptr)
{
    this->AddComponent<QECharacterController>(characterController_ptr);
    auto physicsBody = this->GetComponent<PhysicsBody>();
    auto collider = this->GetComponent<Collider>();
    characterController_ptr->BindGameObjectProperties(physicsBody, collider);
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
    }
}

void QEGameObject::InitializePhysics()
{
    auto physicsBody = this->GetComponent<PhysicsBody>();
    auto collider = this->GetComponent<Collider>();

    if (physicsBody != nullptr && collider != nullptr)
    {
        auto transform = this->GetComponent<Transform>();
        physicsBody->Initialize(transform, collider);
    }

    auto characterController = this->GetComponent<QECharacterController>();
    if (characterController != nullptr)
    {
        characterController->Initialize();
    }
}

bool QEGameObject::IsValidRender()
{
    auto transform = this->GetComponent<Transform>();
    if (transform == nullptr)
        return false;

    auto material = this->GetComponent<Material>();
    auto mesh = this->GetComponent<QEGeometryComponent>();
    if (mesh != nullptr && material != nullptr)
        return true;

    for (auto child : childs)
    {
        auto childMat = child->GetComponent<Material>();
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

void QEGameObject::UpdatePhysicTransform()
{
    auto physicsBody = this->GetComponent<PhysicsBody>();
    auto collider = this->GetComponent<Collider>();

    if (physicsBody != nullptr && collider != nullptr)
    {
        physicsBody->UpdateTransform();
    }
}
