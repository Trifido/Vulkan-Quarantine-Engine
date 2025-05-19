#include "QEGameObject.h"
#include <PrimitiveMesh.h>
#include <MeshImporter.h>

#include "PrimitiveTypes.h"
#include <AnimationImporter.h>
#include <CapsuleMesh.h>

QEGameObject::QEGameObject()
{
    this->childs.resize(0);
    this->InitializeResources();
    this->_meshImportedType = MeshImportedType::NONE_GEO;

    this->InitializeComponents();

    this->vkCmdDrawMeshTasksEXT =
        (PFN_vkCmdDrawMeshTasksEXT)vkGetDeviceProcAddr(this->deviceModule->device, "vkCmdDrawMeshTasksEXT");
}

QEGameObject::QEGameObject(PRIMITIVE_TYPE type, bool isMeshShading)
{
    this->InitializeGameObject(type, isMeshShading);
}

QEGameObject::QEGameObject(std::string meshPath, bool isMeshShading)
{
    this->InitializeGameObject(meshPath, isMeshShading);
}

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

void QEGameObject::QEInitialize()
{
    this->InitializeResources();
}

void QEGameObject::InitializeResources()
{
    this->deviceModule = DeviceModule::getInstance();
    this->queueModule = QueueModule::getInstance();
    this->materialManager = MaterialManager::getInstance();
    this->cullingSceneManager = CullingSceneManager::getInstance();
}

void QEGameObject::Cleanup()
{
    auto mesh = this->GetComponent<GeometryComponent>();
    if (mesh != nullptr)
    {
        mesh->cleanup();
    }

    auto animationComponent = this->GetComponent<AnimationComponent>();
    if (animationComponent != nullptr)
    {
        animationComponent->CleanLastResources();
    }

    if (!this->childs.empty())
    {
        for (auto& child : this->childs)
        {
            auto childMesh = child->GetComponent<Mesh>();
            childMesh->cleanup();

            auto childAC = child->GetComponent<AnimationComponent>();
            if (childAC != nullptr)
            {
                childAC->CleanLastResources();
            }
        }
    }    
}

void QEGameObject::CreateDrawCommand(VkCommandBuffer& commandBuffer, uint32_t idx)
{
    bool isAnimationPipeline = this->_meshImportedType == ANIMATED_GEO;
    auto animationComponent = this->GetComponent<AnimationComponent>();
    auto animatorPtr = isAnimationPipeline ? animationComponent->animator : nullptr;

    this->SetDrawCommand(commandBuffer, idx, animatorPtr);

    for (auto child : childs)
    {
        child->SetDrawCommand(commandBuffer, idx, animatorPtr);
    }
}

void QEGameObject::CreateShadowCommand(VkCommandBuffer& commandBuffer, uint32_t idx, VkPipelineLayout pipelineLayout)
{
    bool isAnimationPipeline = this->_meshImportedType == ANIMATED_GEO;
    auto animationComponent = this->GetComponent<AnimationComponent>();
    auto animatorPtr = isAnimationPipeline ? animationComponent->animator : nullptr;

    this->SetDrawShadowCommand(commandBuffer, idx, pipelineLayout, animatorPtr);
    for (auto child : childs)
    {
        child->SetDrawShadowCommand(commandBuffer, idx, pipelineLayout, animatorPtr);
    }
}

void QEGameObject::AddMaterial(std::shared_ptr<Material> material_ptr)
{
    if (material_ptr == nullptr)
        return;

    this->AddComponent<Material>(material_ptr);
    this->bindMaterialName = material_ptr->Name;

    auto mesh = this->GetComponent<GeometryComponent>();
    if (mesh != nullptr)
    {
        material_ptr->bindingMesh(mesh);
    }
}

void QEGameObject::AddAnimation(std::shared_ptr<Animation> animation_ptr)
{
    auto animationComponent = this->GetComponent<AnimationComponent>();
    if (animationComponent == nullptr)
    {
        this->AddComponent(std::make_shared<AnimationComponent>());
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
    this->AddComponent(characterController_ptr);
    auto physicsBody = this->GetComponent<PhysicsBody>();
    auto collider = this->GetComponent<Collider>();
    characterController_ptr->BindGameObjectProperties(physicsBody, collider);
}

void QEGameObject::InitializeComponents()
{
    this->AddComponent<Transform>(std::make_shared<Transform>());

    auto mesh = this->GetComponent<GeometryComponent>();
    if (mesh != nullptr)
    {
        mesh->InitializeMesh();

        if (this->isMeshShading)
        {
            auto material = this->GetComponent<Material>();
            material->descriptor->SetMeshletBuffers(mesh->meshlets_ptr);
        }
    }

    if (!this->childs.empty())
    {
        for (auto& child : this->childs)
        {
            child->InitializeComponents();
        }
    }
}

void QEGameObject::InitializeAnimationComponent()
{
    auto animationComponent = this->GetComponent<AnimationComponent>();
    if (animationComponent != nullptr)
    {
        std::vector<std::string> idChilds;
        if (!this->childs.empty())
        {
            for (auto child : this->childs)
            {
                idChilds.push_back(child->id);
            }
        }
        else
        {
            idChilds.push_back(this->id);
        }

        animationComponent->animator->InitializeComputeNodes(idChilds);

        if (!this->childs.empty())
        {
            for (int idChild = 0; idChild < this->childs.size(); idChild++)
            {
                auto childMesh = this->childs[idChild]->GetComponent<Mesh>();
                uint32_t numVertices = childMesh->numVertices;
                animationComponent->animator->SetVertexBufferInComputeNode(this->childs[idChild]->id, childMesh->vertexBuffer, childMesh->animationBuffer, numVertices);
            }
        }
        else
        {
            auto mesh = this->GetComponent<GeometryComponent>();
            animationComponent->animator->SetVertexBufferInComputeNode(0, mesh->vertexBuffer, mesh->animationBuffer, mesh->numVertices);
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
    auto mesh = this->GetComponent<GeometryComponent>();
    if (mesh != nullptr && material != nullptr)
        return true;

    for (auto child : childs)
    {
        auto childMat = child->GetComponent<Material>();
        auto childMesh = child->GetComponent<GeometryComponent>();
        if (childMesh != nullptr && childMat != nullptr)
            return true;
    }

    return false;
}

bool QEGameObject::IsRenderEnable()
{
    bool isRender = true;
    auto material = this->GetComponent<Material>();
    auto mesh = this->GetComponent<GeometryComponent>();
    isRender = isRender && material != nullptr;
    isRender = isRender && mesh != nullptr;
    isRender = isRender && this->aabbculling->isGameObjectVisible;

    return isRender;
}

void QEGameObject::InitializeGameObject(std::string meshPath, bool isMeshShading)
{
    this->InitializeResources();
    this->isMeshShading = isMeshShading;

    this->MeshFilePath = meshPath;
    bool loadResult = this->CreateChildsGameObject(meshPath);

    if (loadResult)
    {
        this->InitializeComponents();
        this->InitializeAnimationComponent();
    }

    this->vkCmdDrawMeshTasksEXT =
        (PFN_vkCmdDrawMeshTasksEXT)vkGetDeviceProcAddr(this->deviceModule->device, "vkCmdDrawMeshTasksEXT");
}

void QEGameObject::InitializeGameObject(PRIMITIVE_TYPE type, bool isMeshShading)
{
    this->InitializeResources();
    this->isMeshShading = isMeshShading;
    auto mesh = this->GetComponent<GeometryComponent>();

    if (type == PRIMITIVE_TYPE::CAPSULE_TYPE)
    {
        this->AddComponent<GeometryComponent>(std::make_shared<CapsuleMesh>(CapsuleMesh()));
    }
    else
    {
        this->AddComponent<GeometryComponent>(std::make_shared<PrimitiveMesh>(PrimitiveMesh(type)));
    }

    this->_meshImportedType = MeshImportedType::PRIMITIVE_GEO;

    if (type != PRIMITIVE_TYPE::GRID_TYPE)
    {
        auto matptr = this->materialManager->GetMaterial(this->bindMaterialName);

        if (matptr == nullptr)
        {
            if (this->isMeshShading)
            {
                auto mat = this->materialManager->GetMaterial("defaultMeshPrimitiveMat");
                auto newMatInstance = mat->CreateMaterialInstance();
                this->materialManager->AddMaterial(newMatInstance);
                this->AddMaterial(newMatInstance);
            }
            else
            {
                auto mat = this->materialManager->GetMaterial("defaultPrimitiveMat");
                auto newMatInstance = mat->CreateMaterialInstance();
                this->materialManager->AddMaterial(newMatInstance);
                this->AddMaterial(newMatInstance);
            }
        }

        matptr = this->materialManager->GetMaterial(this->bindMaterialName);
        matptr->InitializeMaterialDataUBO();
    }
    else
    {
        this->_meshImportedType = MeshImportedType::EDITOR_GEO;
    }

    this->InitializeComponents();

    if (type != PRIMITIVE_TYPE::GRID_TYPE)
    {
        auto transform = this->GetComponent<Transform>();
        auto mesh = this->GetComponent<GeometryComponent>();
        auto downcastedPtr = std::dynamic_pointer_cast<PrimitiveMesh>(mesh);
        this->aabbculling = this->cullingSceneManager->GenerateAABB(downcastedPtr->aabbData, transform);
    }

    this->vkCmdDrawMeshTasksEXT =
        (PFN_vkCmdDrawMeshTasksEXT)vkGetDeviceProcAddr(this->deviceModule->device, "vkCmdDrawMeshTasksEXT");
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

bool QEGameObject::CreateChildsGameObject(std::string pathfile)
{
    MeshImporter importer = {};
    importer.EnableMeshShaderMaterials = this->isMeshShading;
    std::vector<MeshData> data = importer.LoadMesh(pathfile);

    if (data.empty())
        return false;

    this->_meshImportedType = importer.HasAnimation() ? MeshImportedType::ANIMATED_GEO : MeshImportedType::COMMON_GEO;

    if (data.size() > 1)
    {
        this->AddComponent<Transform>(std::make_shared<Transform>());
        auto transform = this->GetComponent<Transform>();
        this->childs.resize(data.size());

        glm::mat4 parentModel = transform->GetModel();

        for (size_t id = 0; id < data.size(); id++)
        {
            this->childs[id] = std::make_shared<QEGameObject>();
            this->childs[id]->AddComponent<GeometryComponent>(std::make_shared<Mesh>(Mesh(data[id])));
            this->childs[id]->_meshImportedType = this->_meshImportedType;
            this->childs[id]->isMeshShading = this->isMeshShading;
            this->childs[id]->AddComponent<Transform>(std::make_shared<Transform>(Transform(parentModel * data[id].model)));
            this->childs[id]->AddMaterial(this->materialManager->GetMaterial(data[id].materialID));
            transform->AddChild(this->childs[id]->GetComponent<Transform>());
            this->childs[id]->parent = this;
        }
    }
    else
    {
        this->AddComponent<GeometryComponent>(std::make_shared<Mesh>(Mesh(data[0])));

        auto transform = this->GetComponent<Transform>();
        this->AddComponent<Transform>(std::make_shared<Transform>(Transform(data[0].model)));

        auto matptr = this->materialManager->GetMaterial(this->bindMaterialName);
        this->AddMaterial(this->materialManager->GetMaterial(data[0].materialID));
    }

    if (this->_meshImportedType == MeshImportedType::ANIMATED_GEO)
    {
        this->AddComponent(std::make_shared<SkeletalComponent>());
        auto skeletalComponent = this->GetComponent<SkeletalComponent>();
        skeletalComponent->numBones = importer.GetBoneCount();
        skeletalComponent->m_BoneInfoMap = importer.GetBoneInfoMap();

        auto animData = AnimationImporter::ImportAnimation(pathfile, skeletalComponent->m_BoneInfoMap, skeletalComponent->numBones);

        if (!animData.empty())
        {
            for (auto anim : animData)
            {
                if (anim.m_Duration > 0.0f)
                {
                    this->AddAnimation(std::make_shared<Animation>(anim));
                }
            }
        }
    }

    std::pair<glm::vec3, glm::vec3> aabbData = importer.GetAABBData();
    auto transform = this->GetComponent<Transform>();
    this->aabbculling = this->cullingSceneManager->GenerateAABB(aabbData, transform);

    for (unsigned int i = 0; i < childs.size(); i++)
    {
        childs.at(i)->aabbculling = this->aabbculling;
    }

    return true;
}

void QEGameObject::SetDrawCommand(VkCommandBuffer& commandBuffer, uint32_t idx, std::shared_ptr<Animator> animator_ptr)
{
    if (this->IsRenderEnable())
    {
        auto mat = this->GetComponent<Material>();
        auto mesh = this->GetComponent<GeometryComponent>();
        auto pipelineModule = mat->shader->PipelineModule;
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineModule->pipeline);

        vkCmdSetDepthTestEnable(commandBuffer, true);
        vkCmdSetDepthWriteEnable(commandBuffer, true);

        if (this->_meshImportedType == EDITOR_GEO)
        {
            vkCmdSetCullMode(commandBuffer, false);
        }
        else
        {
            vkCmdSetCullMode(commandBuffer, true);
            vkCmdSetFrontFace(commandBuffer, VK_FRONT_FACE_CLOCKWISE);
        }

        if (!this->isMeshShading)
        {
            VkDeviceSize offsets[] = { 0 };

            if (animator_ptr != nullptr)
            {
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, &animator_ptr->GetComputeNode(this->id)->computeDescriptor->ssboData[2]->uniformBuffers.at(idx), offsets);
            }
            else
            {
                VkBuffer vertexBuffers[] = { mesh->vertexBuffer };
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
            }
            vkCmdBindIndexBuffer(commandBuffer, mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        }

        mat->BindDescriptors(commandBuffer, idx);

        auto transform = this->GetComponent<Transform>();
        vkCmdPushConstants(commandBuffer, pipelineModule->pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(PushConstantStruct), &transform->GetModel());

        if (this->isMeshShading)
        {
            this->vkCmdDrawMeshTasksEXT(commandBuffer, 32, 1, 1);
        }
        else
        {
            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mesh->indices.size()), 1, 0, 0, 0);
        }
    }
}

void QEGameObject::SetDrawShadowCommand(VkCommandBuffer& commandBuffer, uint32_t idx, VkPipelineLayout pipelineLayout, std::shared_ptr<Animator> animator_ptr)
{
    if (this->IsRenderEnable())
    {
        auto mesh = this->GetComponent<GeometryComponent>();

        if (!this->isMeshShading)
        {
            VkDeviceSize offsets[] = { 0 };

            if (animator_ptr != nullptr)
            {
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, &animator_ptr->GetComputeNode(this->id)->computeDescriptor->ssboData[2]->uniformBuffers.at(idx), offsets);
            }
            else
            {
                VkBuffer vertexBuffers[] = { mesh->vertexBuffer };
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
            }
            vkCmdBindIndexBuffer(commandBuffer, mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        }

        if (this->isMeshShading)
        {
            this->vkCmdDrawMeshTasksEXT(commandBuffer, 32, 1, 1);
        }
        else
        {
            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mesh->indices.size()), 1, 0, 0, 0);
        }
    }
}
