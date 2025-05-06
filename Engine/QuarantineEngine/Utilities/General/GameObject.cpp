#include "GameObject.h"
#include <PrimitiveMesh.h>
#include <MeshImporter.h>

#include "PrimitiveTypes.h"
#include <AnimationImporter.h>
#include <CapsuleMesh.h>

GameObject::GameObject()
{
    this->childs.resize(0);
    this->InitializeResources();
    this->_meshImportedType = MeshImportedType::NONE_GEO;

    this->InitializeComponents();

    this->vkCmdDrawMeshTasksEXT =
        (PFN_vkCmdDrawMeshTasksEXT)vkGetDeviceProcAddr(this->deviceModule->device, "vkCmdDrawMeshTasksEXT");
}

GameObject::GameObject(PRIMITIVE_TYPE type, bool isMeshShading)
{
    this->InitializeGameObject(type, isMeshShading);
}

GameObject::GameObject(std::string meshPath, bool isMeshShading)
{
    this->InitializeGameObject(meshPath, isMeshShading);
}

GameObject::GameObject(const GameObjectDto& gameObjectDto) : Numbered(gameObjectDto.Id)
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
    this->_Transform->SetModel(gameObjectDto.WorldTransform);
}

void GameObject::InitializeResources()
{
    this->deviceModule = DeviceModule::getInstance();
    this->queueModule = QueueModule::getInstance();
    this->materialManager = MaterialManager::getInstance();
    this->cullingSceneManager = CullingSceneManager::getInstance();
}

void GameObject::Cleanup()
{
    if (_Mesh != nullptr)
    {
        _Mesh->cleanup();
    }

    if (this->animationComponent != nullptr)
    {
        this->animationComponent->CleanLastResources();
    }

    if (!this->childs.empty())
    {
        for (auto& child : this->childs)
        {
            child->_Mesh->cleanup();

            if (child->animationComponent != nullptr)
            {
                child->animationComponent->CleanLastResources();
            }
        }
    }    
}

void GameObject::CreateDrawCommand(VkCommandBuffer& commandBuffer, uint32_t idx)
{
    bool isAnimationPipeline = this->_meshImportedType == ANIMATED_GEO;
    auto animatorPtr = isAnimationPipeline ? this->animationComponent->animator : nullptr;

    this->SetDrawCommand(commandBuffer, idx, animatorPtr);
    for (auto child : childs)
    {
        child->SetDrawCommand(commandBuffer, idx, animatorPtr);
    }
}

void GameObject::CreateShadowCommand(VkCommandBuffer& commandBuffer, uint32_t idx, VkPipelineLayout pipelineLayout)
{
    bool isAnimationPipeline = this->_meshImportedType == ANIMATED_GEO;
    auto animatorPtr = isAnimationPipeline ? this->animationComponent->animator : nullptr;

    this->SetDrawShadowCommand(commandBuffer, idx, pipelineLayout, animatorPtr);
    for (auto child : childs)
    {
        child->SetDrawShadowCommand(commandBuffer, idx, pipelineLayout, animatorPtr);
    }
}

void GameObject::AddMaterial(std::shared_ptr<Material> material_ptr)
{
    if (material_ptr == nullptr)
        return;

    this->_Material = material_ptr;
    this->bindMaterialName = material_ptr->Name;

    if (this->_Mesh != nullptr)
    {
        this->_Material->bindingMesh(this->_Mesh);
    }
}

void GameObject::AddPhysicBody(std::shared_ptr<PhysicBody> physicBody_ptr)
{
    this->physicBody = physicBody_ptr;
}

void GameObject::AddCollider(std::shared_ptr<Collider> collider_ptr)
{
    this->collider = collider_ptr;
}

void GameObject::AddAnimation(std::shared_ptr<Animation> animation_ptr)
{
    if (this->animationComponent == nullptr)
    {
        this->animationComponent = std::make_shared<AnimationComponent>();
    }

    this->animationComponent->AddAnimation(animation_ptr);

    if (this->animationManager == nullptr)
    {
        this->animationManager = AnimationManager::getInstance();
        this->animationManager->AddAnimationComponent(this->id, this->animationComponent);
    }
}

void GameObject::AddCharacterController(std::shared_ptr<QECharacterController> characterController_ptr)
{
    this->characterController = characterController_ptr;
    this->characterController->BindGameObjectProperties(this->physicBody, this->collider);
}

void GameObject::InitializeComponents()
{
    if (this->_Transform == nullptr)
    {
        this->_Transform = std::make_shared<Transform>();
    }

    if (this->_Mesh != nullptr)
    {
        this->_Mesh->InitializeMesh();

        if (this->isMeshShading)
        {
            this->_Material->descriptor->SetMeshletBuffers(this->_Mesh->meshlets_ptr);
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

void GameObject::InitializeAnimationComponent()
{
    if (this->animationComponent != nullptr)
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

        this->animationComponent->animator->InitializeComputeNodes(idChilds);

        if (!this->childs.empty())
        {
            for (int idChild = 0; idChild < this->childs.size(); idChild++)
            {
                uint32_t numVertices = this->childs[idChild]->_Mesh->numVertices;
                this->animationComponent->animator->SetVertexBufferInComputeNode(this->childs[idChild]->id, this->childs[idChild]->_Mesh->vertexBuffer, this->childs[idChild]->_Mesh->animationBuffer, numVertices);
            }
        }
        else
        {
            this->animationComponent->animator->SetVertexBufferInComputeNode(0, this->_Mesh->vertexBuffer, this->_Mesh->animationBuffer, this->_Mesh->numVertices);
        }
    }
}

void GameObject::InitializePhysics()
{
    if (this->physicBody != nullptr && this->collider != nullptr)
    {
        this->physicBody->Initialize(this->_Transform, this->collider);
    }

    if (this->characterController != nullptr)
    {
        this->characterController->Initialize();
    }
}

bool GameObject::IsValidRender()
{
    if (this->_Transform == nullptr)
        return false;

    if (this->_Mesh != nullptr && this->_Material != nullptr)
        return true;

    for (auto child : childs)
    {
        if (child->_Mesh != nullptr && child->_Material != nullptr)
            return true;
    }

    return false;
}

bool GameObject::IsRenderEnable()
{
    bool isRender = true;
    isRender = isRender && this->_Material != nullptr;
    isRender = isRender && this->_Mesh != nullptr;
    isRender = isRender && this->aabbculling->isGameObjectVisible;

    return isRender;
}

void GameObject::InitializeGameObject(std::string meshPath, bool isMeshShading)
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

void GameObject::InitializeGameObject(PRIMITIVE_TYPE type, bool isMeshShading)
{
    this->InitializeResources();
    this->isMeshShading = isMeshShading;
    this->_primitiveMeshType = type;

    if (type == PRIMITIVE_TYPE::CAPSULE_TYPE)
    {
        this->_Mesh = std::make_shared<CapsuleMesh>(CapsuleMesh());
    }
    else
    {
        this->_Mesh = std::make_shared<PrimitiveMesh>(PrimitiveMesh(type));
    }

    this->_meshImportedType = MeshImportedType::PRIMITIVE_GEO;

    if (type != PRIMITIVE_TYPE::GRID_TYPE)
    {
        this->_Material = this->materialManager->GetMaterial(this->bindMaterialName);

        if (this->_Material == nullptr)
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

        this->_Material->InitializeMaterialDataUBO();
    }
    else
    {
        this->_meshImportedType = MeshImportedType::EDITOR_GEO;
    }

    this->InitializeComponents();

    if (type != PRIMITIVE_TYPE::GRID_TYPE)
    {
        auto downcastedPtr = std::dynamic_pointer_cast<PrimitiveMesh>(this->_Mesh);
        this->aabbculling = this->cullingSceneManager->GenerateAABB(downcastedPtr->aabbData, this->_Transform);
    }

    this->vkCmdDrawMeshTasksEXT =
        (PFN_vkCmdDrawMeshTasksEXT)vkGetDeviceProcAddr(this->deviceModule->device, "vkCmdDrawMeshTasksEXT");
}

bool GameObject::IsValidGameObject()
{
    return this->_Transform != nullptr;
}

void GameObject::UpdatePhysicTransform()
{
    if (this->physicBody != nullptr && this->collider != nullptr)
    {
        this->physicBody->UpdateTransform();
    }
}

bool GameObject::CreateChildsGameObject(std::string pathfile)
{
    MeshImporter importer = {};
    importer.EnableMeshShaderMaterials = this->isMeshShading;
    std::vector<MeshData> data = importer.LoadMesh(pathfile);

    if (data.empty())
        return false;

    this->_meshImportedType = importer.HasAnimation() ? MeshImportedType::ANIMATED_GEO : MeshImportedType::COMMON_GEO;

    if (data.size() > 1)
    {
        this->_Transform = std::make_shared<Transform>();
        this->childs.resize(data.size());

        glm::mat4 parentModel = this->_Transform->GetModel();

        for (size_t id = 0; id < data.size(); id++)
        {
            this->childs[id] = std::make_shared<GameObject>();
            this->childs[id]->_Mesh = std::make_shared<Mesh>(Mesh(data[id]));
            this->childs[id]->_meshImportedType = this->_meshImportedType;
            this->childs[id]->isMeshShading = this->isMeshShading;
            this->childs[id]->_Transform = std::make_shared<Transform>(Transform(parentModel * data[id].model));
            this->childs[id]->AddMaterial(this->materialManager->GetMaterial(data[id].materialID));
            this->_Transform->AddChild(this->childs[id]->_Transform);
            this->childs[id]->parent = this;
        }
    }
    else
    {
        this->_Mesh = std::make_shared<Mesh>(Mesh(data[0]));
        this->_Transform = std::make_shared<Transform>(Transform(data[0].model));
        this->_Material = this->materialManager->GetMaterial(this->bindMaterialName);
        if (this->_Material == nullptr)
        {
            this->AddMaterial(this->materialManager->GetMaterial(data[0].materialID));
        }
    }

    if (this->_meshImportedType == MeshImportedType::ANIMATED_GEO)
    {
        skeletalComponent = std::make_shared<SkeletalComponent>();
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
    this->aabbculling = this->cullingSceneManager->GenerateAABB(aabbData, this->_Transform);

    for (unsigned int i = 0; i < childs.size(); i++)
    {
        childs.at(i)->aabbculling = this->aabbculling;
    }

    return true;
}

void GameObject::SetDrawCommand(VkCommandBuffer& commandBuffer, uint32_t idx, std::shared_ptr<Animator> animator_ptr)
{
    if (this->IsRenderEnable())
    {
        auto pipelineModule = this->_Material->shader->PipelineModule;
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
                VkBuffer vertexBuffers[] = { this->_Mesh->vertexBuffer };
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
            }
            vkCmdBindIndexBuffer(commandBuffer, this->_Mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        }

        this->_Material->BindDescriptors(commandBuffer, idx);

        vkCmdPushConstants(commandBuffer, pipelineModule->pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(PushConstantStruct), &this->_Transform->GetModel());

        if (this->isMeshShading)
        {
            this->vkCmdDrawMeshTasksEXT(commandBuffer, 32, 1, 1);
        }
        else
        {
            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(this->_Mesh->indices.size()), 1, 0, 0, 0);
        }
    }
}

void GameObject::SetDrawShadowCommand(VkCommandBuffer& commandBuffer, uint32_t idx, VkPipelineLayout pipelineLayout, std::shared_ptr<Animator> animator_ptr)
{
    if (this->IsRenderEnable())
    {
        if (!this->isMeshShading)
        {
            VkDeviceSize offsets[] = { 0 };

            if (animator_ptr != nullptr)
            {
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, &animator_ptr->GetComputeNode(this->id)->computeDescriptor->ssboData[2]->uniformBuffers.at(idx), offsets);
            }
            else
            {
                VkBuffer vertexBuffers[] = { this->_Mesh->vertexBuffer };
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
            }
            vkCmdBindIndexBuffer(commandBuffer, this->_Mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        }

        if (this->isMeshShading)
        {
            this->vkCmdDrawMeshTasksEXT(commandBuffer, 32, 1, 1);
        }
        else
        {
            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(this->_Mesh->indices.size()), 1, 0, 0, 0);
        }
    }
}
