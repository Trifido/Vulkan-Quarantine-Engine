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

void GameObjectManager::SaveGameObjects(std::ofstream& file)
{
    std::vector<GameObjectDto> gameObjectDtos;
    for (unsigned int idl = 0; idl < this->renderLayers.GetCount(); idl++)
    {
        unsigned int id = this->renderLayers.GetLayer(idl);
        for (auto model : this->_objects[id])
        {
            std::string matPath = "NULL_MATERIAL";
            auto mat = model.second->GetMaterial();
            if (mat != nullptr)
            {
                matPath = mat->Name;
            }

            auto transform = model.second->GetComponent<QETransform>();
            auto mesh = model.second->GetComponent<QEGeometryComponent>();

            GameObjectDto gameObjectDto(
                model.second->ID(),
                model.first,
                transform->GetWorldMatrix(),
                "",
                matPath);
            gameObjectDtos.push_back(gameObjectDto);
        }
    }

    int numGameObjects = static_cast<int>(gameObjectDtos.size());
    file.write(reinterpret_cast<const char*>(&numGameObjects), sizeof(int));

    size_t idLength = sizeof(char) * Numbered::ID_LENGTH;
    size_t nameLength;
    for (int i = 0; i < gameObjectDtos.size(); i++)
    {
        file.write(reinterpret_cast<const char*>(&idLength), sizeof(idLength));
        file.write(gameObjectDtos[i].Id.c_str(), static_cast<int>(idLength));

        nameLength = gameObjectDtos[i].Name.length();
        file.write(reinterpret_cast<const char*>(&nameLength), sizeof(nameLength));
        file.write(gameObjectDtos[i].Name.c_str(), nameLength);

        file.write(reinterpret_cast<const char*>(&gameObjectDtos[i].WorldTransform), sizeof(glm::mat4));

        size_t meshPathLength = gameObjectDtos[i].MeshPath.length();
        file.write(reinterpret_cast<const char*>(&meshPathLength), sizeof(meshPathLength));
        file.write(gameObjectDtos[i].MeshPath.c_str(), meshPathLength);


        size_t matPathLength = gameObjectDtos[i].BindMaterialName.length();
        file.write(reinterpret_cast<const char*>(&matPathLength), sizeof(matPathLength));

        if (matPathLength > 0)
        {
            file.write(gameObjectDtos[i].BindMaterialName.c_str(), matPathLength);
        }
    }
}

void GameObjectManager::LoadGameObjectDtos(std::vector<GameObjectDto>& gameObjectDtos)
{
    // Load the GameObjects
    //for (size_t i = 0; i < gameObjectDtos.size(); i++)
    //{
    //    std::shared_ptr<QEGameObject> gameObject = std::make_shared<QEGameObject>(gameObjectDtos[i]);
    //    this->AddGameObject(gameObject, gameObjectDtos[i].Name);
    //}
}

YAML::Node GameObjectManager::SerializeGameObjects()
{
    YAML::Node gameObjectsNode;

    for (unsigned int idl = 0; idl < this->renderLayers.GetCount(); idl++)
    {
        unsigned int id = this->renderLayers.GetLayer(idl);
        for (auto model : this->_objects[id])
        {
            gameObjectsNode.push_back(model.second->ToYaml());
        }
    }

    return gameObjectsNode;
}

void GameObjectManager::DeserializeGameObjects(YAML::Node gameObjects)
{
    for (auto gameObjectData : gameObjects)
    {
        std::shared_ptr<QEGameObject> gameObject = QEGameObject::FromYaml(gameObjectData);
        this->AddGameObject(gameObject);
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

std::vector<GameObjectDto> GameObjectManager::GetGameObjectDtos(std::ifstream& file)
{
    // Read the game objects
    int numGameObjects;
    file.read(reinterpret_cast<char*>(&numGameObjects), sizeof(int));

    size_t idLength;
    size_t meshPathLength;
    size_t matPathLength;
    size_t nameLength;

    std::vector<GameObjectDto> gameObjectDtos(numGameObjects);
    for (int i = 0; i < numGameObjects; i++)
    {
        file.read(reinterpret_cast<char*>(&idLength), sizeof(idLength));
        gameObjectDtos[i].Id.resize(idLength);
        file.read(&gameObjectDtos[i].Id[0], idLength);

        file.read(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));
        gameObjectDtos[i].Id.resize(nameLength);
        file.read(&gameObjectDtos[i].Name[0], nameLength);

        file.read(reinterpret_cast<char*>(&gameObjectDtos[i].WorldTransform), sizeof(glm::mat4));

        file.read(reinterpret_cast<char*>(&meshPathLength), sizeof(meshPathLength));
        gameObjectDtos[i].MeshPath.resize(meshPathLength);
        file.read(&gameObjectDtos[i].MeshPath[0], meshPathLength);

        file.read(reinterpret_cast<char*>(&matPathLength), sizeof(matPathLength));
        gameObjectDtos[i].BindMaterialName.resize(matPathLength);
        if (matPathLength > 0)
        {
            file.read(&gameObjectDtos[i].BindMaterialName[0], matPathLength);
        }
        else
        {
            gameObjectDtos[i].BindMaterialName = "NULL_MATERIAL";
        }
    }

    return gameObjectDtos;
}
