#include "GameObjectManager.h"
#include <iostream>
#include <GameObjectDto.h>
#include <QEMeshRenderer.h>

std::string GameObjectManager::CheckName(std::string nameGameObject)
{
    std::unordered_map<std::string, std::shared_ptr<QEGameObject>>::const_iterator got;
    std::string newName = nameGameObject;
    unsigned int id = 0;

    for (unsigned int idl = 0; idl < this->renderLayers.GetCount(); idl++)
    {
        do
        {
            got = this->_objects[this->renderLayers.GetLayer(idl)].find(newName);

            if (got != this->_objects[this->renderLayers.GetLayer(idl)].end())
            {
                id++;
                newName = nameGameObject + "_" + std::to_string(id);
            }
        } while (got != this->_objects[this->renderLayers.GetLayer(idl)].end());
    }

    return newName;
}

unsigned int GameObjectManager::DecideRenderLayer(std::shared_ptr<QEGameObject> go, unsigned int defaultLayer)
{
    if (auto mat = go->GetMaterial()) return mat->layer;

    if (!go->childs.empty())
    {
        if (auto matChild = go->childs[0]->GetMaterial())
            return matChild->layer;
    }

    return defaultLayer;
}

void GameObjectManager::RegisterSingle(std::shared_ptr<QEGameObject> go, std::string name)
{
    name = CheckName(name);

    const unsigned int layer = DecideRenderLayer(go, (unsigned int)RenderLayer::SOLID);
    this->_objects[layer][name] = go;
}

void GameObjectManager::AddGameObject(std::shared_ptr<QEGameObject> object_ptr)
{
    RegisterSingle(object_ptr, object_ptr->Name);

    std::function<void(std::shared_ptr<QEGameObject>)> dfs =
        [&](std::shared_ptr<QEGameObject> node)
        {
            for (size_t i = 0; i < node->childs.size(); ++i) {
                auto& child = node->childs[i];
                if (!child) continue;

                RegisterSingle(child, child->Name);
                dfs(child);
            }
        };

    dfs(object_ptr);
}

void GameObjectManager::DrawCommand(VkCommandBuffer& commandBuffer, uint32_t idx)
{
    for (unsigned int idl = 0; idl < this->renderLayers.GetCount(); idl++)
    {
        for (auto model : this->_objects[this->renderLayers.GetLayer(idl)])
        {
            auto meshRenderer = model.second->GetComponent<QEMeshRenderer>();
            if (meshRenderer == nullptr)
                continue;
            meshRenderer->SetDrawCommand(commandBuffer, idx);
        }
    }
}

void GameObjectManager::CSMCommand(VkCommandBuffer& commandBuffer, uint32_t idx, VkPipelineLayout pipelineLayout, uint32_t cascadeIndex)
{
    for (auto model : this->_objects[(unsigned int)RenderLayer::SOLID])
    {
        auto meshRenderer = model.second->GetComponent<QEMeshRenderer>();
        if (meshRenderer == nullptr)
            continue;

        PushConstantCSMStruct shadowParameters = {};
        shadowParameters.model = model.second->GetComponent<QETransform>()->GetWorldMatrix();
        shadowParameters.cascadeIndex = cascadeIndex;

        vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(PushConstantCSMStruct), &shadowParameters);
        meshRenderer->SetDrawShadowCommand(commandBuffer, idx, pipelineLayout);
    }
}

void GameObjectManager::OmniShadowCommand(VkCommandBuffer& commandBuffer, uint32_t idx, VkPipelineLayout pipelineLayout, glm::mat4 viewParameter, glm::vec3 lightPosition)
{
    for (auto model : this->_objects[(unsigned int)RenderLayer::SOLID])
    {
        auto meshRenderer = model.second->GetComponent<QEMeshRenderer>();
        if (meshRenderer == nullptr)
            continue;

        auto transform = model.second->GetComponent<QETransform>();
        glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), -lightPosition);

        PushConstantOmniShadowStruct shadowParameters = {};
        shadowParameters.lightModel = translationMatrix * transform->GetWorldMatrix();
        shadowParameters.model = transform->GetWorldMatrix();
        shadowParameters.view = viewParameter;

        vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(PushConstantOmniShadowStruct), &shadowParameters);
        meshRenderer->SetDrawShadowCommand(commandBuffer, idx, pipelineLayout);
    }
}

void GameObjectManager::ReleaseAllGameObjects()
{
    for (unsigned int idl = 0; idl < this->renderLayers.GetCount(); idl++)
    {
        for (auto model : this->_objects[this->renderLayers.GetLayer(idl)])
        {
            model.second->QEDestroy();
        }
    }
}

void GameObjectManager::CleanLastResources()
{
    this->_objects.clear();
}

std::shared_ptr<QEGameObject> GameObjectManager::GetGameObject(std::string name)
{
    for (unsigned int idl = 0; idl < this->renderLayers.GetCount(); idl++)
    {
        unsigned int id = this->renderLayers.GetLayer(idl);
        auto it = this->_objects[id].find(name);

        if (it != this->_objects[id].end())
            return it->second;
    }

    return nullptr;
}

YAML::Node GameObjectManager::SerializeGameObjects()
{
    YAML::Node gameObjectsNode;
    std::unordered_set<std::string> emittedRoots;

    for (unsigned int idl = 0; idl < renderLayers.GetCount(); ++idl)
    {
        const unsigned int layerId = renderLayers.GetLayer(idl);

        for (auto& kv : _objects[layerId])
        {
            auto& go = kv.second;
            if (!go) continue;

            if (go->parent != nullptr)
                continue;

            if (!emittedRoots.insert(go->ID()).second)
                continue;

            gameObjectsNode.push_back(go->ToYaml());
        }
    }

    return gameObjectsNode;
}

void GameObjectManager::DeserializeGameObjects(YAML::Node gameObjects)
{
    for (auto gameObjectData : gameObjects)
    {
        std::shared_ptr<QEGameObject> root = QEGameObject::FromYaml(gameObjectData);
        this->AddGameObject(root);
    }
}

void GameObjectManager::StartQEGameObjects()
{
    for (unsigned int idl = 0; idl < this->renderLayers.GetCount(); idl++)
    {
        for (auto model : this->_objects[this->renderLayers.GetLayer(idl)])
        {
            model.second->QEStart();
        }
    }

    for (unsigned int idl = 0; idl < this->renderLayers.GetCount(); idl++)
    {
        for (auto model : this->_objects[this->renderLayers.GetLayer(idl)])
        {
            model.second->QEInit();
        }
    }
}

void GameObjectManager::UpdateQEGameObjects()
{
    for (unsigned int idl = 0; idl < this->renderLayers.GetCount(); idl++)
    {
        for (auto model : this->_objects[this->renderLayers.GetLayer(idl)])
        {
            model.second->QEUpdate();
        }
    }
}

void GameObjectManager::DestroyQEGameObjects()
{
    for (unsigned int idl = 0; idl < this->renderLayers.GetCount(); idl++)
    {
        for (auto model : this->_objects[this->renderLayers.GetLayer(idl)])
        {
            model.second->QEDestroy();
        }
    }
}

std::shared_ptr<QEGameComponent> GameObjectManager::FindGameComponentInScene(std::string id)
{
    for (unsigned int idl = 0; idl < this->renderLayers.GetCount(); idl++)
    {
        for (auto model : this->_objects[this->renderLayers.GetLayer(idl)])
        {
            for (auto component : model.second->components)
            {
                if (component->id == id)
                    return component;
            }
        }
    }

    return nullptr;
}
