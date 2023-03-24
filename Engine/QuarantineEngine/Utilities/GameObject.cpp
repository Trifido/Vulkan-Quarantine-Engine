#include "GameObject.h"
#include <PrimitiveMesh.h>
#include <MeshImporter.h>

#include "PrimitiveTypes.h"
#include <AnimationImporter.h>

GameObject::GameObject()
{
    this->CreateGameObjectID(12);
    this->deviceModule = DeviceModule::getInstance();
    this->queueModule = QueueModule::getInstance();
    this->materialManager = MaterialManager::getInstance();
    this->meshImportedType = MeshImportedType::NONE_GEO;

    size_t numMeshAttributes = this->CheckNumAttributes();
    this->InitializeComponents(numMeshAttributes);
}

GameObject::GameObject(PRIMITIVE_TYPE type)
{
    this->CreateGameObjectID(12);
    this->deviceModule = DeviceModule::getInstance();
    this->queueModule = QueueModule::getInstance();
    this->materialManager = MaterialManager::getInstance();

    this->parent = nullptr;
    this->mesh = std::make_shared<PrimitiveMesh>(PrimitiveMesh(type));
    this->meshImportedType = MeshImportedType::PRIMITIVE_GEO;

    this->addMaterial(this->materialManager->GetMaterial("defaultPrimitiveMat"));

    size_t numMeshAttributes = this->CheckNumAttributes();
    this->InitializeComponents(numMeshAttributes);
}

GameObject::GameObject(std::string meshPath)
{
    this->CreateGameObjectID(12);
    this->deviceModule = DeviceModule::getInstance();
    this->queueModule = QueueModule::getInstance();
    this->materialManager = MaterialManager::getInstance();

    this->parent = nullptr;
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

    if (!this->childs.empty())
    {
        for (auto& it : this->childs)
        {
            it->mesh->cleanup();
        }
    }    
}

void GameObject::drawCommand(VkCommandBuffer& commandBuffer, uint32_t idx)
{
    this->CreateDrawCommand(commandBuffer, idx);
}

void GameObject::addMaterial(std::shared_ptr<Material> material_ptr)
{
    if (material_ptr == nullptr)
        return;

    this->material = material_ptr;
    this->material->InitializeDescriptor();

    if (this->meshImportedType == MeshImportedType::ANIMATED_GEO)
    {
        this->material->descriptor->InitializeAnimationProperties();
    }

    if (this->mesh != nullptr)
    {
        this->material->bindingMesh(this->mesh);
    }

    //if (!this->childs.empty())
    //{
    //    for (auto& it : this->childs)
    //    {
    //        it->material = material_ptr;
    //        it->material->InitializeDescriptor();// it->meshImportedType == MeshImportedType::ANIMATED_GEO);
    //        if (it->meshImportedType == MeshImportedType::ANIMATED_GEO)
    //            it->material->descriptor->InitializeAnimationProperties();
    //        it->material->bindingMesh(it->mesh);
    //    }
    //}
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
        for (auto& it : this->childs)
        {
            it->InitializeComponents(numMeshAttributes);
        }
    }
}

void GameObject::InitializeAnimationComponent()
{
    if (this->animationComponent != nullptr)
    {
        if (this->material != nullptr)
        {
            this->animationComponent->animator->AddDescriptor(this->material->descriptor);
        }

        if (!this->childs.empty())
        {
            for (auto& it : this->childs)
            {
                this->animationComponent->animator->AddDescriptor(it->material->descriptor);
            }
        }
    }
}

void GameObject::CreateGameObjectID(size_t length)
{
    auto randchar = []() -> char
    {
        const char charset[] =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";
        const size_t max_index = (sizeof(charset) - 1);
        return charset[rand() % max_index];
    };
    std::string str(length, 0);
    std::generate_n(str.begin(), length, randchar);

    this->id = str;
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

void GameObject::DrawChilds(VkCommandBuffer& commandBuffer, uint32_t idx)
{
    if (!this->childs.empty())
    {
        for (auto& it : this->childs)
        {
            it->CreateDrawCommand(commandBuffer, idx);
        }
    }
}

void GameObject::CreateDrawCommand(VkCommandBuffer& commandBuffer, uint32_t idx)
{
    if (this->material != nullptr && this->mesh != nullptr)
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->material->pipeline);
        VkBuffer vertexBuffers[] = { this->mesh->vertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, this->mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->material->pipelineLayout, 0, 1, this->material->GetDescrìptor()->getDescriptorSet(idx), 0, nullptr);

        TransformUniform ubo;
        ubo.model = this->transform->GetModel();

        if (this->parent != nullptr)
        {
            ubo.model = this->parent->transform->GetModel() * ubo.model;
        }

        vkCmdPushConstants(commandBuffer, this->material->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &ubo.model);

        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(this->mesh->indices.size()), 1, 0, 0, 0);
    }

    this->DrawChilds(commandBuffer, idx);
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
