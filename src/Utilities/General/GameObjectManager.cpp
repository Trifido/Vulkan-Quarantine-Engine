#include "GameObjectManager.h"
#include <iostream>
#include <GameObjectDto.h>
#include <QEMeshRenderer.h>
#include <unordered_set>
#include <LightManager.h>
#include <Light.h>
#include <PointLight.h>
#include <DirectionalLight.h>
#include <SpotLight.h>
#include <SunLight.h>
#include <QECamera.h>
#include <QESessionManager.h>
#include <QETransform.h>

std::string GameObjectManager::CheckName(std::string nameGameObject)
{
    std::string newName = nameGameObject;
    unsigned int id = 0;

    bool exists = false;
    do
    {
        exists = false;

        for (const auto& bucketPair : _objectsByUpdateOrder)
        {
            const auto& bucket = bucketPair.second;
            if (bucket.find(newName) != bucket.end())
            {
                exists = true;
                ++id;
                newName = nameGameObject + "_" + std::to_string(id);
                break;
            }
        }
    } while (exists);

    return newName;
}

unsigned int GameObjectManager::DecideUpdateBucket(std::shared_ptr<QEGameObject> go, unsigned int defaultOrder)
{
    if (!go)
        return defaultOrder;

    return go->UpdateOrder;
}

void GameObjectManager::RegisterSingle(std::shared_ptr<QEGameObject> go, std::string name)
{
    if (!go)
        return;

    name = CheckName(name);
    go->Name = name;

    const unsigned int bucket = DecideUpdateBucket(go, 0u);
    _objectsByUpdateOrder[bucket][name] = go;
}

void GameObjectManager::UnregisterSingle(const std::shared_ptr<QEGameObject>& go)
{
    if (!go)
        return;

    for (auto& bucketPair : _objectsByUpdateOrder)
    {
        auto& bucket = bucketPair.second;

        for (auto it = bucket.begin(); it != bucket.end();)
        {
            if (it->second == go)
                it = bucket.erase(it);
            else
                ++it;
        }
    }
}

void GameObjectManager::UnregisterHierarchy(const std::shared_ptr<QEGameObject>& go)
{
    if (!go)
        return;

    for (auto& child : go->childs)
    {
        UnregisterHierarchy(child);
    }

    UnregisterSingle(go);
}

void GameObjectManager::DestroyHierarchy(const std::shared_ptr<QEGameObject>& go)
{
    if (!go)
        return;

    for (auto& child : go->childs)
    {
        DestroyHierarchy(child);
    }

    go->QEDestroy();
}

void GameObjectManager::AddGameObject(std::shared_ptr<QEGameObject> object_ptr)
{
    RegisterSingle(object_ptr, object_ptr->Name);

    std::function<void(std::shared_ptr<QEGameObject>)> dfs =
        [&](std::shared_ptr<QEGameObject> node)
        {
            for (size_t i = 0; i < node->childs.size(); ++i)
            {
                auto& child = node->childs[i];
                if (!child)
                    continue;

                RegisterSingle(child, child->Name);
                dfs(child);
            }
        };

    dfs(object_ptr);
}

std::shared_ptr<QEGameObject> GameObjectManager::CreateEmptyGameObject(const std::string& baseName)
{
    const std::string finalName = CheckName(baseName.empty() ? "Empty GameObject" : baseName);
    auto gameObject = std::make_shared<QEGameObject>(finalName);
    AddGameObject(gameObject);
    return gameObject;
}

bool GameObjectManager::RemoveGameObject(const std::shared_ptr<QEGameObject>& object_ptr)
{
    if (!object_ptr)
        return false;

    if (object_ptr->Name == "QECameraEditor")
        return false;

    if (object_ptr->parent != nullptr)
    {
        object_ptr->parent->RemoveChild(object_ptr);
    }

    RemoveLightsFromHierarchy(object_ptr);

    DestroyHierarchy(object_ptr);
    UnregisterHierarchy(object_ptr);

    ReindexLightShadowMaps();

    return true;
}

bool GameObjectManager::SetGameObjectUpdateOrder(const std::shared_ptr<QEGameObject>& objectPtr, unsigned int newOrder)
{
    if (!objectPtr)
        return false;

    if (objectPtr->UpdateOrder == newOrder)
        return false;

    UnregisterSingle(objectPtr);
    objectPtr->UpdateOrder = newOrder;
    RegisterSingle(objectPtr, objectPtr->Name);

    return true;
}

void GameObjectManager::DrawCommand(VkCommandBuffer& commandBuffer, uint32_t idx)
{
    const auto renderItems = BuildRenderItems();

    for (const auto& item : renderItems)
    {
        if (!item.GameObject || !item.MeshRenderer || !item.Material)
            continue;

        item.MeshRenderer->SetDrawCommand(commandBuffer, idx);
    }
}

void GameObjectManager::CSMCommand(VkCommandBuffer& commandBuffer, uint32_t idx, VkPipelineLayout pipelineLayout, uint32_t cascadeIndex)
{
    const auto shadowItems = BuildShadowRenderItems();

    for (const auto& item : shadowItems)
    {
        if (!item.GameObject || !item.MeshRenderer || !item.Material)
            continue;

        auto transform = item.GameObject->GetComponent<QETransform>();
        if (!transform)
            continue;

        PushConstantCSMStruct shadowParameters = {};
        shadowParameters.model = transform->GetWorldMatrix();
        shadowParameters.cascadeIndex = cascadeIndex;

        vkCmdPushConstants(
            commandBuffer,
            pipelineLayout,
            VK_SHADER_STAGE_ALL,
            0,
            sizeof(PushConstantCSMStruct),
            &shadowParameters);

        item.MeshRenderer->SetDrawShadowCommand(commandBuffer, idx, pipelineLayout);
    }
}

void GameObjectManager::OmniShadowCommand(VkCommandBuffer& commandBuffer, uint32_t idx, VkPipelineLayout pipelineLayout, glm::mat4 viewParameter, glm::vec3 lightPosition)
{
    const auto shadowItems = BuildShadowRenderItems();

    for (const auto& item : shadowItems)
    {
        if (!item.GameObject || !item.MeshRenderer || !item.Material)
            continue;

        auto transform = item.GameObject->GetComponent<QETransform>();
        if (!transform)
            continue;

        glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), -lightPosition);

        PushConstantOmniShadowStruct shadowParameters = {};
        shadowParameters.lightModel = translationMatrix * transform->GetWorldMatrix();
        shadowParameters.model = transform->GetWorldMatrix();
        shadowParameters.view = viewParameter;

        vkCmdPushConstants(
            commandBuffer,
            pipelineLayout,
            VK_SHADER_STAGE_ALL,
            0,
            sizeof(PushConstantOmniShadowStruct),
            &shadowParameters);

        item.MeshRenderer->SetDrawShadowCommand(commandBuffer, idx, pipelineLayout);
    }
}

void GameObjectManager::ReleaseAllGameObjects()
{
    for (const auto& bucketPair : _objectsByUpdateOrder)
    {
        const auto& bucket = bucketPair.second;

        for (const auto& model : bucket)
        {
            if (model.second)
            {
                model.second->QEDestroy();
            }
        }
    }
}

void GameObjectManager::CleanLastResources()
{
    _objectsByUpdateOrder.clear();
}

std::shared_ptr<QEGameObject> GameObjectManager::GetGameObject(const std::string& name)
{
    for (const auto& bucketPair : _objectsByUpdateOrder)
    {
        const auto& bucket = bucketPair.second;
        auto it = bucket.find(name);

        if (it != bucket.end())
            return it->second;
    }

    return nullptr;
}

YAML::Node GameObjectManager::SerializeGameObjects()
{
    YAML::Node gameObjectsNode;
    std::unordered_set<std::string> emittedRoots;

    for (const auto& bucketPair : _objectsByUpdateOrder)
    {
        const auto& bucket = bucketPair.second;

        for (const auto& kv : bucket)
        {
            const auto& go = kv.second;
            if (!go)
                continue;

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
        AddGameObject(root);
    }
}

void GameObjectManager::StartQEGameObjects()
{
    for (const auto& bucketPair : _objectsByUpdateOrder)
    {
        const auto& bucket = bucketPair.second;

        for (const auto& model : bucket)
        {
            model.second->QEStart();
        }
    }

    for (const auto& bucketPair : _objectsByUpdateOrder)
    {
        const auto& bucket = bucketPair.second;

        for (const auto& model : bucket)
        {
            model.second->QEInit();
        }
    }
}

void GameObjectManager::UpdateQEGameObjects()
{
    for (const auto& bucketPair : _objectsByUpdateOrder)
    {
        const auto& bucket = bucketPair.second;

        for (const auto& model : bucket)
        {
            model.second->QEUpdate();
        }
    }
}

void GameObjectManager::DestroyQEGameObjects()
{
    for (const auto& bucketPair : _objectsByUpdateOrder)
    {
        const auto& bucket = bucketPair.second;

        for (const auto& model : bucket)
        {
            model.second->QEDestroy();
        }
    }
}

std::shared_ptr<QEGameComponent> GameObjectManager::FindGameComponentInScene(const std::string& id)
{
    for (const auto& bucketPair : _objectsByUpdateOrder)
    {
        const auto& bucket = bucketPair.second;

        for (const auto& model : bucket)
        {
            for (const auto& component : model.second->components)
            {
                if (component->id == id)
                    return component;
            }
        }
    }

    return nullptr;
}

std::vector<std::shared_ptr<QEGameObject>> GameObjectManager::GetRootGameObjects() const
{
    std::vector<std::shared_ptr<QEGameObject>> roots;
    std::unordered_set<std::string> emittedRoots;

    for (const auto& bucketPair : _objectsByUpdateOrder)
    {
        const auto& bucket = bucketPair.second;

        for (const auto& kv : bucket)
        {
            const auto& go = kv.second;
            if (!go)
                continue;

            if (go->parent != nullptr)
                continue;

            if (!emittedRoots.insert(go->ID()).second)
                continue;

            roots.push_back(go);
        }
    }

    return roots;
}

std::shared_ptr<QEGameObject> GameObjectManager::GetGameObjectById(const std::string& id) const
{
    for (const auto& bucketPair : _objectsByUpdateOrder)
    {
        const auto& bucket = bucketPair.second;

        for (const auto& kv : bucket)
        {
            const auto& go = kv.second;
            if (!go)
                continue;

            if (go->ID() == id)
                return go;
        }
    }

    return nullptr;
}

bool GameObjectManager::RenameGameObject(const std::shared_ptr<QEGameObject>& objectPtr, const std::string& newName)
{
    if (!objectPtr)
        return false;

    if (newName.empty())
        return false;

    if (objectPtr->Name == newName)
        return false;

    UnregisterSingle(objectPtr);

    const std::string uniqueName = CheckName(newName);
    objectPtr->Name = uniqueName;

    RegisterSingle(objectPtr, objectPtr->Name);
    return true;
}

std::vector<QEOrderRenderItem> GameObjectManager::BuildRenderItems() const
{
    std::vector<QEOrderRenderItem> items;
    glm::vec3 cameraPosition(0.0f);
    if (auto activeCamera = QESessionManager::getInstance()->ActiveCamera())
    {
        cameraPosition = glm::vec3(activeCamera->CameraData->Position);
    }

    for (const auto& bucketPair : _objectsByUpdateOrder)
    {
        const auto& bucket = bucketPair.second;

        for (const auto& kv : bucket)
        {
            const auto& go = kv.second;
            if (!go)
                continue;

            auto meshRenderer = go->GetComponent<QEMeshRenderer>();
            if (!meshRenderer)
                continue;

            auto material = go->GetMaterial();
            if (!material)
                continue;

            QEOrderRenderItem item;
            item.GameObject = go;
            item.MeshRenderer = meshRenderer;
            item.Material = material;
            item.RenderQueue = material->renderQueue;
            if (auto transform = go->GetComponent<QETransform>())
            {
                const glm::vec3 delta = transform->GetWorldPosition() - cameraPosition;
                item.CameraDistanceSq = glm::dot(delta, delta);
            }

            items.push_back(item);
        }
    }

    std::sort(items.begin(), items.end(),
        [](const QEOrderRenderItem& a, const QEOrderRenderItem& b)
        {
            if (a.RenderQueue != b.RenderQueue)
                return a.RenderQueue < b.RenderQueue;

            const bool aTransparent = a.RenderQueue >= static_cast<unsigned int>(RenderQueue::Transparent);
            const bool bTransparent = b.RenderQueue >= static_cast<unsigned int>(RenderQueue::Transparent);
            if (aTransparent && bTransparent && a.CameraDistanceSq != b.CameraDistanceSq)
                return a.CameraDistanceSq > b.CameraDistanceSq;

            return a.GameObject->ID() < b.GameObject->ID();
        });

    return items;
}

std::vector<QEOrderRenderItem> GameObjectManager::BuildShadowRenderItems() const
{
    std::vector<QEOrderRenderItem> items;

    for (const auto& bucketPair : _objectsByUpdateOrder)
    {
        const auto& bucket = bucketPair.second;

        for (const auto& kv : bucket)
        {
            const auto& go = kv.second;
            if (!go)
                continue;

            auto meshRenderer = go->GetComponent<QEMeshRenderer>();
            if (!meshRenderer)
                continue;

            auto material = go->GetMaterial();
            if (!material)
                continue;

            if (material->renderQueue >= static_cast<unsigned int>(RenderQueue::Transparent))
                continue;

            QEOrderRenderItem item;
            item.GameObject = go;
            item.MeshRenderer = meshRenderer;
            item.Material = material;
            item.RenderQueue = material->renderQueue;

            items.push_back(item);
        }
    }

    std::sort(items.begin(), items.end(),
        [](const QEOrderRenderItem& a, const QEOrderRenderItem& b)
        {
            if (a.RenderQueue != b.RenderQueue)
                return a.RenderQueue < b.RenderQueue;

            return a.GameObject->ID() < b.GameObject->ID();
        });

    return items;
}

void GameObjectManager::ResetSceneState()
{
    ReleaseAllGameObjects();
    CleanLastResources();
}

void GameObjectManager::RemoveLightsFromHierarchy(const std::shared_ptr<QEGameObject>& go)
{
    if (!go)
        return;

    for (const auto& child : go->childs)
    {
        RemoveLightsFromHierarchy(child);
    }

    auto light = go->GetComponent<QELight>();
    if (!light)
        return;

    auto* lightManager = LightManager::getInstance();
    if (!lightManager)
        return;

    std::string lightName = light->Name;
    lightManager->DeleteLight(light, lightName);
}

void GameObjectManager::ReindexLightShadowMaps()
{
    auto* lightManager = LightManager::getInstance();
    if (!lightManager)
        return;

    for (uint32_t i = 0; i < lightManager->PointLights.size(); ++i)
    {
        if (lightManager->PointLights[i])
        {
            lightManager->PointLights[i]->idxShadowMap = i;
        }
    }

    for (uint32_t i = 0; i < lightManager->DirLights.size(); ++i)
    {
        if (lightManager->DirLights[i])
        {
            lightManager->DirLights[i]->idxShadowMap = i;
        }
    }

    for (uint32_t i = 0; i < lightManager->SpotLights.size(); ++i)
    {
        if (lightManager->SpotLights[i])
        {
            lightManager->SpotLights[i]->idxShadowMap = i;
        }
    }
}

void GameObjectManager::RegisterSceneLights()
{
    auto* lightManager = LightManager::getInstance();
    if (!lightManager)
        return;

    for (const auto& bucketPair : _objectsByUpdateOrder)
    {
        const auto& bucket = bucketPair.second;

        for (const auto& kv : bucket)
        {
            const auto& go = kv.second;
            if (!go)
                continue;

            auto light = go->GetComponent<QELight>();
            if (!light)
                continue;

            if (light->ResourcesInitialized)
                continue;

            std::string lightName = light->Name.empty() ? go->Name : light->Name;
            lightManager->AddNewLight(light, lightName);

            go->Name = lightName;
            light->Name = lightName;
        }
    }
}
