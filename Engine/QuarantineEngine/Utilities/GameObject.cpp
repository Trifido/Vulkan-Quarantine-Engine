#include "GameObject.h"
#include <PrimitiveMesh.h>
#include <MeshImporter.h>

#include "PrimitiveTypes.h"
#include <AnimationImporter.h>

void GameObject::InitializeResources()
{
    this->deviceModule = DeviceModule::getInstance();
    this->queueModule = QueueModule::getInstance();
    this->materialManager = MaterialManager::getInstance();
    this->cullingSceneManager = CullingSceneManager::getInstance();
}

bool GameObject::isRenderEnable()
{
    bool isRender = true;
    isRender = isRender && this->material != nullptr;
    isRender = isRender && this->mesh != nullptr;
    isRender = isRender && this->aabbculling->isGameObjectVisible;

    return isRender;
}

GameObject::GameObject()
{
    this->InitializeResources();
    this->meshImportedType = MeshImportedType::NONE_GEO;

    this->InitializeComponents();

    this->vkCmdDrawMeshTasksEXT =
        (PFN_vkCmdDrawMeshTasksEXT)vkGetDeviceProcAddr(this->deviceModule->device, "vkCmdDrawMeshTasksEXT");
}

GameObject::GameObject(PRIMITIVE_TYPE type, bool isMeshShading)
{
    this->InitializeResources();
    this->isMeshShading = isMeshShading;
    this->mesh = std::make_shared<PrimitiveMesh>(PrimitiveMesh(type));
    this->meshImportedType = MeshImportedType::PRIMITIVE_GEO;

    if (type != PRIMITIVE_TYPE::GRID_TYPE)
    {
        if (this->isMeshShading)
        {
            auto mat = this->materialManager->GetMaterial("defaultMeshPrimitiveMat");
            auto newMatInstance = mat->CreateMaterialInstance();
            this->materialManager->AddMaterial("defaultMeshPrimitiveMat", newMatInstance);
            this->addMaterial(newMatInstance);
        }
        else
        {
            auto mat = this->materialManager->GetMaterial("defaultPrimitiveMat");
            auto newMatInstance = mat->CreateMaterialInstance();
            this->materialManager->AddMaterial("defaultPrimitiveMat", newMatInstance);
            this->addMaterial(newMatInstance);
        }

        this->material->InitializeMaterialDataUBO();
    }
    else
    {
        this->meshImportedType = MeshImportedType::EDITOR_GEO;
    }

    this->InitializeComponents();

    if (type != PRIMITIVE_TYPE::GRID_TYPE)
    {
        auto downcastedPtr = std::dynamic_pointer_cast<PrimitiveMesh>(this->mesh);
        this->aabbculling = this->cullingSceneManager->GenerateAABB(downcastedPtr->aabbData, this->transform);
    }

    this->vkCmdDrawMeshTasksEXT =
        (PFN_vkCmdDrawMeshTasksEXT)vkGetDeviceProcAddr(this->deviceModule->device, "vkCmdDrawMeshTasksEXT");
}

GameObject::GameObject(std::string meshPath, bool isMeshShading)
{
    this->InitializeResources();
    this->isMeshShading = isMeshShading;

    bool loadResult = this->CreateChildsGameObject(meshPath);

    if (loadResult)
    {
        this->InitializeComponents();
        this->InitializeAnimationComponent();
    }

    this->vkCmdDrawMeshTasksEXT =
        (PFN_vkCmdDrawMeshTasksEXT)vkGetDeviceProcAddr(this->deviceModule->device, "vkCmdDrawMeshTasksEXT");
}

void GameObject::cleanup()
{
    if (mesh != nullptr)
    {
        mesh->cleanup();
    }

    if (this->animationComponent != nullptr)
    {
        this->animationComponent->CleanLastResources();
    }

    if (!this->childs.empty())
    {
        for (auto& child : this->childs)
        {
            child->mesh->cleanup();

            if (child->animationComponent != nullptr)
            {
                child->animationComponent->CleanLastResources();
            }

            child->parent = nullptr;
        }
    }    
}


void GameObject::drawCommand(VkCommandBuffer& commandBuffer, uint32_t idx)
{
    bool isAnimationPipeline = this->meshImportedType == ANIMATED_GEO;

    if (isAnimationPipeline)
    {
        this->CreateDrawCommand(commandBuffer, idx, this->animationComponent->animator);
        for (auto child : childs)
        {
            child->CreateDrawCommand(commandBuffer, idx, this->animationComponent->animator);
        }
    }
    else
    {
        this->CreateDrawCommand(commandBuffer, idx, nullptr);
        for (auto child : childs)
        {
            child->CreateDrawCommand(commandBuffer, idx, nullptr);
        }
    }
}

void GameObject::drawShadowCommand(VkCommandBuffer& commandBuffer, uint32_t idx, VkPipelineLayout pipelineLayout)
{
    bool isAnimationPipeline = this->meshImportedType == ANIMATED_GEO;

    if (isAnimationPipeline)
    {
        this->CreateDrawShadowCommand(commandBuffer, idx, pipelineLayout, this->animationComponent->animator);
        for (auto child : childs)
        {
            child->CreateDrawShadowCommand(commandBuffer, idx, pipelineLayout, this->animationComponent->animator);
        }
    }
    else
    {
        this->CreateDrawShadowCommand(commandBuffer, idx, pipelineLayout, nullptr);
        for (auto child : childs)
        {
            child->CreateDrawShadowCommand(commandBuffer, idx, pipelineLayout, nullptr);
        }
    }
}

void GameObject::addMaterial(std::shared_ptr<Material> material_ptr)
{
    if (material_ptr == nullptr)
        return;

    this->material = material_ptr;

    if (this->mesh != nullptr)
    {
        this->material->bindingMesh(this->mesh);
    }
}

void GameObject::addPhysicBody(std::shared_ptr<PhysicBody> physicBody_ptr)
{
    this->physicBody = physicBody_ptr;
}

void GameObject::addCollider(std::shared_ptr<Collider> collider_ptr)
{
    this->collider = collider_ptr;
}

void GameObject::addAnimation(std::shared_ptr<Animation> animation_ptr)
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

void GameObject::InitializeComponents()
{
    if (this->transform == nullptr)
    {
        this->transform = std::make_shared<Transform>();
    }

    if (this->mesh != nullptr)
    {
        this->mesh->InitializeMesh();

        if (this->isMeshShading)
        {
            this->material->descriptor->SetMeshletBuffers(this->mesh->meshlets_ptr);
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
                uint32_t numVertices = this->childs[idChild]->mesh->numVertices;
                this->animationComponent->animator->SetVertexBufferInComputeNode(this->childs[idChild]->id, this->childs[idChild]->mesh->vertexBuffer, this->childs[idChild]->mesh->animationBuffer, numVertices);
            }
        }
        else
        {
            this->animationComponent->animator->SetVertexBufferInComputeNode(0, this->mesh->vertexBuffer, this->mesh->animationBuffer, this->mesh->numVertices);
        }
    }
}

void GameObject::InitializePhysics()
{
    if (this->physicBody != nullptr && this->collider != nullptr)
    {
        this->physicBody->Initialize(this->transform, this->collider);
    }   
}

bool GameObject::IsValid()
{
    if (this->transform == nullptr)
        return false;

    if (this->mesh != nullptr && this->material != nullptr)
        return true;

    for (auto child : childs)
    {
        if (child->mesh != nullptr && child->material != nullptr)
            return true;
    }

    return false;
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

    this->meshImportedType = importer.HasAnimation() ? MeshImportedType::ANIMATED_GEO : MeshImportedType::COMMON_GEO;

    if (data.size() > 1)
    {
        this->transform = std::make_shared<Transform>();
        this->childs.resize(data.size());

        glm::mat4 parentModel = this->transform->GetModel();

        std::weak_ptr<GameObject> wp;

        for (size_t id = 0; id < data.size(); id++)
        {
            this->childs[id] = std::make_shared<GameObject>();
            this->childs[id]->parent = std::make_shared<GameObject>(*this);
            this->childs[id]->mesh = std::make_shared<Mesh>(Mesh(data[id]));
            this->childs[id]->meshImportedType = this->meshImportedType;
            this->childs[id]->isMeshShading = this->isMeshShading;
            this->childs[id]->transform = std::make_shared<Transform>(Transform(parentModel * data[id].model));
            this->childs[id]->addMaterial(this->materialManager->GetMaterial(data[id].materialID));
        }
    }
    else
    {
        this->mesh = std::make_shared<Mesh>(Mesh(data[0]));
        this->transform = std::make_shared<Transform>(Transform(data[0].model));
        this->addMaterial(this->materialManager->GetMaterial(data[0].materialID));
    }

    if (this->meshImportedType == MeshImportedType::ANIMATED_GEO)
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
                    this->addAnimation(std::make_shared<Animation>(anim));
                }
            }
        }
    }

    std::pair<glm::vec3, glm::vec3> aabbData = importer.GetAABBData();
    this->aabbculling = this->cullingSceneManager->GenerateAABB(aabbData, this->transform);

    for (unsigned int i = 0; i < childs.size(); i++)
    {
        childs.at(i)->aabbculling = this->aabbculling;
    }

    return true;
}

void GameObject::CreateDrawCommand(VkCommandBuffer& commandBuffer, uint32_t idx, std::shared_ptr<Animator> animator_ptr)
{
    if (this->isRenderEnable())
    {
        auto pipelineModule = this->material->shader->PipelineModule;
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineModule->pipeline);

        vkCmdSetDepthTestEnable(commandBuffer, true);
        vkCmdSetDepthWriteEnable(commandBuffer, true);

        if (this->meshImportedType == EDITOR_GEO)
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
                VkBuffer vertexBuffers[] = { this->mesh->vertexBuffer };
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
            }
            vkCmdBindIndexBuffer(commandBuffer, this->mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        }

        if (this->material->HasDescriptorBuffer())
        {
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineModule->pipelineLayout, 0, 1, this->material->descriptor->getDescriptorSet(idx), 0, nullptr);
        }

        this->pushConstant.model = this->transform->GetModel();
        if (this->parent != nullptr)
        {
            pushConstant.model = this->parent->transform->GetModel() * pushConstant.model;
        }

        VkShaderStageFlagBits stages = VK_SHADER_STAGE_ALL;
        vkCmdPushConstants(commandBuffer, pipelineModule->pipelineLayout, stages, 0, sizeof(PushConstantStruct), &pushConstant);

        if (this->isMeshShading)
        {
            this->vkCmdDrawMeshTasksEXT(commandBuffer, 32, 1, 1);
        }
        else
        {
            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(this->mesh->indices.size()), 1, 0, 0, 0);
        }
    }
}


void GameObject::CreateDrawShadowCommand(VkCommandBuffer& commandBuffer, uint32_t idx, VkPipelineLayout pipelineLayout, std::shared_ptr<Animator> animator_ptr)
{
    if (this->isRenderEnable())
    {
        vkCmdSetDepthTestEnable(commandBuffer, true);
        vkCmdSetDepthWriteEnable(commandBuffer, true);

        if(this->meshImportedType == EDITOR_GEO)
        {
            vkCmdSetFrontFace(commandBuffer, VK_FRONT_FACE_COUNTER_CLOCKWISE);
        }
        else
        {
            vkCmdSetFrontFace(commandBuffer, VK_FRONT_FACE_COUNTER_CLOCKWISE);
            //vkCmdSetFrontFace(commandBuffer, VK_FRONT_FACE_CLOCKWISE);
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
                VkBuffer vertexBuffers[] = { this->mesh->vertexBuffer };
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
            }
            vkCmdBindIndexBuffer(commandBuffer, this->mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        }

        this->pushConstant.model = this->transform->GetModel();
        if (this->parent != nullptr)
        {
            pushConstant.model = this->parent->transform->GetModel() * pushConstant.model;
        }

        VkShaderStageFlagBits stages = VK_SHADER_STAGE_ALL;
        vkCmdPushConstants(commandBuffer, pipelineLayout, stages, 0, sizeof(PushConstantStruct), &pushConstant);

        if (this->isMeshShading)
        {
            this->vkCmdDrawMeshTasksEXT(commandBuffer, 32, 1, 1);
        }
        else
        {
            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(this->mesh->indices.size()), 1, 0, 0, 0);
        }
    }
}
