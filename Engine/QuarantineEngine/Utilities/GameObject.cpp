#include "GameObject.h"
#include <PrimitiveMesh.h>
#include <MeshImporter.h>

#include "PrimitiveTypes.h"

GameObject::GameObject()
{
    this->deviceModule = DeviceModule::getInstance();
    this->queueModule = QueueModule::getInstance();
    this->materialManager = MaterialManager::getInstance();
    this->InitializeComponents();
}

GameObject::GameObject(PRIMITIVE_TYPE type)
{
    this->deviceModule = DeviceModule::getInstance();
    this->queueModule = QueueModule::getInstance();
    this->materialManager = MaterialManager::getInstance();

    this->parent = nullptr;
    mesh = std::make_shared<PrimitiveMesh>(PrimitiveMesh(type));
    this->InitializeComponents();
}

GameObject::GameObject(std::string meshPath)
{
    this->deviceModule = DeviceModule::getInstance();
    this->queueModule = QueueModule::getInstance();
    this->materialManager = MaterialManager::getInstance();

    this->parent = nullptr;
    this->CreateChildsGameObject(meshPath);
    this->InitializeComponents();
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

    if (this->mesh != nullptr)
    {
        this->material->bindingMesh(this->mesh);
    }

    if (!this->childs.empty())
    {
        for (auto& it : this->childs)
        {
            it->material = material_ptr;
            this->material->bindingMesh(it->mesh);
        }
    }
}

void GameObject::addEditorCamera(std::shared_ptr<Camera> camera_ptr)
{
    this->cameraEditor = camera_ptr;
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
    }

    if (!this->childs.empty())
    {
        for (auto& it : this->childs)
        {
            it->InitializeComponents();
        }
    }
}

void GameObject::CreateChildsGameObject(std::string pathfile)
{
    MeshImporter importer = {};
    std::vector<MeshData> data = importer.LoadMesh(pathfile);

    if (data.size() > 1)
    {
        this->childs.resize(data.size());

        for (size_t id = 0; id < data.size(); id++)
        {
            this->childs[id] = std::make_shared<GameObject>();
            this->childs[id]->parent = std::make_shared<GameObject>(*this);
            this->childs[id]->mesh = std::make_shared<Mesh>(Mesh(data[id]));
            this->childs[id]->transform = std::make_shared<Transform>(Transform(data[id].model));
            this->childs[id]->addMaterial(this->materialManager->GetMaterial(data[id].materialID));
        }
    }
    else
    {
        this->mesh = std::make_shared<Mesh>(Mesh(data[0]));
        this->transform = std::make_shared<Transform>(Transform(data[0].model));
        this->addMaterial(this->materialManager->GetMaterial(data[0].materialID));
    }
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

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->material->pipelineLayout, 0, 1, this->material->descriptor->getDescriptorSet(idx), 0, nullptr);

        vkCmdPushConstants(commandBuffer, this->material->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(this->transform->ubo.model), &this->transform->ubo.model);

        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(this->mesh->indices.size()), 1, 0, 0, 0);
    }

    this->DrawChilds(commandBuffer, idx);
}
