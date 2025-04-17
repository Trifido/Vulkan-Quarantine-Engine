#include "GameObjectManager.h"
#include <iostream>
#include <GameObjectDto.h>

std::string GameObjectManager::CheckName(std::string nameGameObject)
{
    std::unordered_map<std::string, std::shared_ptr<GameObject>>::const_iterator got;
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

void GameObjectManager::AddGameObject(std::shared_ptr<GameObject> object_ptr, std::string name)
{
    if (object_ptr->IsValidRender())
    {
        name = CheckName(name);

        if (object_ptr->_Material == nullptr)
        {
            if (!object_ptr->childs.empty())
            {
                unsigned int childLayer = object_ptr->childs[0]->_Material->layer;
                this->_objects[childLayer][name] = object_ptr;
            }
        }
        else
        {
            this->_objects[object_ptr->_Material->layer][name] = object_ptr;
        }

        if (object_ptr->physicBody != nullptr)
        {
            this->_physicObjects[name] = object_ptr;
        }
    }
}

void GameObjectManager::DrawCommand(VkCommandBuffer& commandBuffer, uint32_t idx)
{
    for (unsigned int idl = 0; idl < this->renderLayers.GetCount(); idl++)
    {
        for (auto model : this->_objects[this->renderLayers.GetLayer(idl)])
        {
            model.second->CreateDrawCommand(commandBuffer, idx);
        }
    }
}

void GameObjectManager::CSMCommand(VkCommandBuffer& commandBuffer, uint32_t idx, VkPipelineLayout pipelineLayout, uint32_t cascadeIndex)
{
    for (auto model : this->_objects[(unsigned int)RenderLayer::SOLID])
    {
        PushConstantCSMStruct shadowParameters = {};
        shadowParameters.model = model.second->_Transform->GetModel();
        shadowParameters.cascadeIndex = cascadeIndex;

        vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(PushConstantCSMStruct), &shadowParameters);
        model.second->CreateShadowCommand(commandBuffer, idx, pipelineLayout);
    }
}

void GameObjectManager::ShadowCommand(VkCommandBuffer& commandBuffer, uint32_t idx, VkPipelineLayout pipelineLayout)
{
    for (auto model : this->_objects[(unsigned int)RenderLayer::SOLID])
    {
        //model.second->drawShadowCommand(commandBuffer, idx, pipelineLayout);
    }
}

void GameObjectManager::OmniShadowCommand(VkCommandBuffer& commandBuffer, uint32_t idx, VkPipelineLayout pipelineLayout, glm::mat4 viewParameter, glm::vec3 lightPosition)
{
    for (auto model : this->_objects[(unsigned int)RenderLayer::SOLID])
    {
        PushConstantOmniShadowStruct shadowParameters = {};
        shadowParameters.view = viewParameter;

        glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), -lightPosition);
        glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), model.second->_Transform->Scale);
        glm::mat4 rotationMatrix = glm::toMat4(model.second->_Transform->Orientation);
        shadowParameters.lightModel = translationMatrix * rotationMatrix * scaleMatrix;
        shadowParameters.model = model.second->_Transform->GetModel();

        vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(PushConstantOmniShadowStruct), &shadowParameters);
        model.second->CreateShadowCommand(commandBuffer, idx, pipelineLayout);
    }
}

void GameObjectManager::InitializePhysics()
{
    for (auto model : this->_physicObjects)
    {
        model.second->InitializePhysics();
    }
}

void GameObjectManager::UpdatePhysicTransforms()
{
    for (auto model : this->_physicObjects)
    {
        model.second->UpdatePhysicTransform();
    }
}

void GameObjectManager::Cleanup()
{
    for (unsigned int idl = 0; idl < this->renderLayers.GetCount(); idl++)
    {
        for (auto model : this->_objects[this->renderLayers.GetLayer(idl)])
        {
            model.second->Cleanup();
        }
    }
}

void GameObjectManager::CleanLastResources()
{
    this->_objects.clear();
    this->_physicObjects.clear();
}

std::shared_ptr<GameObject> GameObjectManager::GetGameObject(std::string name)
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
            if (model.second->_Material != nullptr)
            {
                matPath = model.second->_Material->Name;
            }

            GameObjectDto gameObjectDto(
                model.second->ID(),
                model.first,
                model.second->_Transform->GetModel(),
                model.second->_meshImportedType,
                model.second->_primitiveMeshType,
                model.second->MeshFilePath,
                matPath);
            gameObjectDtos.push_back(gameObjectDto);
        }
    }

    int numGameObjects = gameObjectDtos.size();
    file.write(reinterpret_cast<const char*>(&numGameObjects), sizeof(int));

    size_t idLength = sizeof(char) * Numbered::ID_LENGTH;
    size_t nameLength;
    for (int i = 0; i < gameObjectDtos.size(); i++)
    {
        file.write(reinterpret_cast<const char*>(&idLength), sizeof(idLength));
        file.write(gameObjectDtos[i].Id.c_str(), idLength);

        nameLength = gameObjectDtos[i].Name.length();
        file.write(reinterpret_cast<const char*>(&nameLength), sizeof(nameLength));
        file.write(gameObjectDtos[i].Name.c_str(), nameLength);

        file.write(reinterpret_cast<const char*>(&gameObjectDtos[i].WorldTransform), sizeof(glm::mat4));

        file.write(reinterpret_cast<const char*>(&gameObjectDtos[i].MeshImportedType), sizeof(int));

        file.write(reinterpret_cast<const char*>(&gameObjectDtos[i].MeshPrimitiveType), sizeof(int));

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
    for (size_t i = 0; i < gameObjectDtos.size(); i++)
    {
        std::shared_ptr<GameObject> gameObject = std::make_shared<GameObject>(gameObjectDtos[i]);
        this->AddGameObject(gameObject, gameObjectDtos[i].Name);
    }
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

        file.read(reinterpret_cast<char*>(&gameObjectDtos[i].MeshImportedType), sizeof(int));

        file.read(reinterpret_cast<char*>(&gameObjectDtos[i].MeshPrimitiveType), sizeof(int));

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
