#include "GameObject.h"
#include <PrimitiveMesh.h>
#include <MeshImporter.h>

#include "PrimitiveTypes.h"
#include <AnimationImporter.h>

GameObject::GameObject()
{
    this->deviceModule = DeviceModule::getInstance();
    this->queueModule = QueueModule::getInstance();
    this->materialManager = MaterialManager::getInstance();
    this->meshImportedType = MeshImportedType::NONE_GEO;

    size_t numMeshAttributes = this->CheckNumAttributes();
    this->InitializeComponents(numMeshAttributes);
}

GameObject::GameObject(PRIMITIVE_TYPE type)
{
    this->deviceModule = DeviceModule::getInstance();
    this->queueModule = QueueModule::getInstance();
    this->materialManager = MaterialManager::getInstance();
    this->mesh = std::make_shared<PrimitiveMesh>(PrimitiveMesh(type));
    this->meshImportedType = MeshImportedType::PRIMITIVE_GEO;

    if (type != PRIMITIVE_TYPE::GRID_TYPE)
    {
        auto mat = this->materialManager->GetMaterial("defaultPrimitiveMat");
        auto newMatInstance = mat->CreateMaterialInstance();
        this->materialManager->AddMaterial("defaultPrimitiveMat", newMatInstance);

        this->addMaterial(newMatInstance);
        this->material->InitializeMaterialDataUBO();
    }
    else if (type == PRIMITIVE_TYPE::GRID_TYPE)
    {
        this->meshImportedType = MeshImportedType::EDITOR_GEO;
    }

    size_t numMeshAttributes = this->CheckNumAttributes();
    this->InitializeComponents(numMeshAttributes);
}

GameObject::GameObject(std::string meshPath)
{
    this->deviceModule = DeviceModule::getInstance();
    this->queueModule = QueueModule::getInstance();
    this->materialManager = MaterialManager::getInstance();

    bool loadResult = this->CreateChildsGameObject(meshPath);

    if (loadResult)
    {
        size_t numMeshAttributes = this->CheckNumAttributes();
        this->InitializeComponents(numMeshAttributes);
        this->InitializeAnimationComponent();
    }
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
    if (this->meshImportedType != ANIMATED_GEO)
    {
        this->CreateDrawCommand(commandBuffer, idx);
        for each (auto child in childs)
        {
            child->CreateDrawCommand(commandBuffer, idx);
        }
    }
    else
    {
        this->CreateAnimationDrawCommand(commandBuffer, idx, this->animationComponent->animator);
        for each (auto child in childs)
        {
            child->CreateAnimationDrawCommand(commandBuffer, idx, this->animationComponent->animator);
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

void GameObject::InitializeComponents(size_t numMeshAttributes)
{
    if (this->transform == nullptr)
    {
        this->transform = std::make_shared<Transform>();
    }

    if (this->mesh != nullptr)
    {
        this->mesh->InitializeMesh(numMeshAttributes);
    }

    if (!this->childs.empty())
    {
        for (auto& child : this->childs)
        {
            child->InitializeComponents(numMeshAttributes);
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
            for each (auto child in this->childs)
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
                this->animationComponent->animator->SetVertexBufferInComputeNode(this->childs[idChild]->id, this->childs[idChild]->mesh->vertexBuffer, numVertices);
            }
        }
        else
        {
            this->animationComponent->animator->SetVertexBufferInComputeNode(0, this->mesh->vertexBuffer, this->mesh->numVertices);
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

    return true;
}

void GameObject::CreateAnimationDrawCommand(VkCommandBuffer& commandBuffer, uint32_t idx, std::shared_ptr<Animator> animator)
{
    if (this->material != nullptr && this->mesh != nullptr)
    {
        auto pipelineModule = this->material->shader->PipelineModule;
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineModule->pipeline);

        vkCmdSetDepthTestEnable(commandBuffer, true);
        if (this->meshImportedType == EDITOR_GEO)
        {
            vkCmdSetCullMode(commandBuffer, false);
        }
        else
        {
            vkCmdSetCullMode(commandBuffer, true);
            vkCmdSetFrontFace(commandBuffer, VK_FRONT_FACE_CLOCKWISE);
        }

        VkDeviceSize animOffsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &animator->GetComputeNode(this->id)->computeDescriptor->ssboData[1]->uniformBuffers.at(idx), animOffsets);
        vkCmdBindIndexBuffer(commandBuffer, this->mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        if (this->material->HasDescriptorBuffer())
        {
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineModule->pipelineLayout, 0, 1, this->material->descriptor->getDescriptorSet(idx), 0, nullptr);
        }

        this->pushConstant.model = this->transform->GetModel();
        if (this->parent != nullptr)
        {
            pushConstant.model = this->parent->transform->GetModel() * pushConstant.model;
        }
        vkCmdPushConstants(commandBuffer, pipelineModule->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantStruct), &pushConstant);

        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(this->mesh->indices.size()), 1, 0, 0, 0);
    }
}

void GameObject::CreateDrawCommand(VkCommandBuffer& commandBuffer, uint32_t idx)
{
    if (this->material != nullptr && this->mesh != nullptr)
    {
        auto pipelineModule = this->material->shader->PipelineModule;
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineModule->pipeline);

        vkCmdSetDepthTestEnable(commandBuffer, true);
        if (this->meshImportedType == EDITOR_GEO)
        {
            vkCmdSetCullMode(commandBuffer, false);
        }
        else
        {
            vkCmdSetCullMode(commandBuffer, true);
            vkCmdSetFrontFace(commandBuffer, VK_FRONT_FACE_CLOCKWISE);
        }

        VkDeviceSize offsets[] = { 0 };
        VkBuffer vertexBuffers[] = { this->mesh->vertexBuffer };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, this->mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        if (this->material->HasDescriptorBuffer())
        {
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineModule->pipelineLayout, 0, 1, this->material->descriptor->getDescriptorSet(idx), 0, nullptr);
        }

        this->pushConstant.model = this->transform->GetModel();
        if (this->parent != nullptr)
        {
            pushConstant.model = this->parent->transform->GetModel() * pushConstant.model;
        }
        vkCmdPushConstants(commandBuffer, pipelineModule->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantStruct), &pushConstant);

        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(this->mesh->indices.size()), 1, 0, 0, 0);
    }
}

size_t GameObject::CheckNumAttributes()
{
    size_t numAttributes = 0;

    switch (this->meshImportedType)
    {
    case MeshImportedType::ANIMATED_GEO:
        numAttributes = 7;
        break;
    case MeshImportedType::PRIMITIVE_GEO:
        numAttributes = 5;
        break;
    case MeshImportedType::NONE_GEO:
        numAttributes = 0;
        break;
    case MeshImportedType::COMMON_GEO:
        numAttributes = 5;
    default:
        break;
    }

    return numAttributes;
}
