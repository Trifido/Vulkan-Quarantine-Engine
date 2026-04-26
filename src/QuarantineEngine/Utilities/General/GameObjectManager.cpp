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
#include <QECameraContext.h>
#include <QETransform.h>

namespace
{
    bool IsEditorOnlyGameObject(const std::shared_ptr<QEGameObject>& gameObject)
    {
        return gameObject && gameObject->Name == "QECameraEditor";
    }
}

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

    return go->GetUpdateOrder();
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

    for (auto& child : go->GetChildren())
    {
        UnregisterHierarchy(child);
    }

    UnregisterSingle(go);
}

void GameObjectManager::DestroyHierarchy(const std::shared_ptr<QEGameObject>& go)
{
    if (!go)
        return;

    for (auto& child : go->GetChildren())
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
            const auto& children = node->GetChildren();
            for (size_t i = 0; i < children.size(); ++i)
            {
                auto& child = children[i];
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

    if (object_ptr->GetParent() != nullptr)
    {
        object_ptr->GetParent()->RemoveChild(object_ptr);
    }

    // Scene editing can delete a hierarchy while its buffers are still
    // referenced by command buffers submitted in previous frames.
    // For this editor-side path, synchronize before releasing GPU resources.
    if (auto* deviceModule = DeviceModule::getInstance())
    {
        vkDeviceWaitIdle(deviceModule->device);
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

    if (objectPtr->GetUpdateOrder() == newOrder)
        return false;

    UnregisterSingle(objectPtr);
    objectPtr->SetUpdateOrder(newOrder);
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

        item.MeshRenderer->SetDrawCommand(commandBuffer, idx, item.SubMeshIndex);
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

        item.MeshRenderer->SetDrawShadowCommand(commandBuffer, idx, pipelineLayout, item.SubMeshIndex);
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

        item.MeshRenderer->SetDrawShadowCommand(commandBuffer, idx, pipelineLayout, item.SubMeshIndex);
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

std::shared_ptr<QEGameObject> GameObjectManager::GetGameObject(const std::string& name) const
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

YAML::Node GameObjectManager::SerializeGameObjects() const
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

            if (IsEditorOnlyGameObject(go))
                continue;

            if (go->GetParent() != nullptr)
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
        if (IsEditorOnlyGameObject(root))
            continue;
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

std::shared_ptr<QEGameComponent> GameObjectManager::FindGameComponentInScene(const std::string& id)
{
    for (const auto& bucketPair : _objectsByUpdateOrder)
    {
        const auto& bucket = bucketPair.second;

        for (const auto& model : bucket)
        {
            if (!model.second || !model.second->IsActiveInHierarchy())
                continue;

            for (const auto& component : model.second->GetComponents())
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

            if (go->GetParent() != nullptr)
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

void GameObjectManager::RemoveMaterialReferences(const std::string& materialName)
{
    if (materialName.empty())
        return;

    for (const auto& bucketPair : _objectsByUpdateOrder)
    {
        const auto& bucket = bucketPair.second;

        for (const auto& kv : bucket)
        {
            const auto& go = kv.second;
            if (!go)
                continue;

            bool removedAny = false;
            for (size_t materialIndex = 0; materialIndex < go->GetMaterialCount();)
            {
                const auto material = go->GetMaterialAt(materialIndex);
                if (material && material->Name == materialName)
                {
                    go->RemoveMaterialAt(materialIndex);
                    removedAny = true;
                    continue;
                }

                ++materialIndex;
            }

            if (!removedAny)
                continue;

            if (auto geometry = go->GetComponent<QEGeometryComponent>())
            {
                if (auto mesh = geometry->GetMesh())
                {
                    if (mesh->MaterialRel.empty() && go->GetMaterialCount() > 0)
                    {
                        if (auto firstMaterial = go->GetMaterialAt(0))
                        {
                            mesh->MaterialRel.resize(mesh->MeshData.size(), firstMaterial->Name);
                        }
                    }
                }
            }
        }
    }
}

bool GameObjectManager::RenameGameObject(const std::shared_ptr<QEGameObject>& objectPtr, const std::string& newName)
{
    if (!objectPtr)
        return false;

    if (newName.empty())
        return false;

    if (objectPtr->Name == newName)
        return false;

    const std::string previousObjectName = objectPtr->Name;
    auto light = objectPtr->GetComponent<QELight>();
    const std::string previousLightName = light ? light->Name : std::string{};

    UnregisterSingle(objectPtr);

    const std::string uniqueName = CheckName(newName);
    objectPtr->Name = uniqueName;
    if (light)
    {
        light->Name = uniqueName;

        if (auto* lightManager = LightManager::getInstance())
        {
            const std::string oldRegisteredName = previousLightName.empty() ? previousObjectName : previousLightName;
            lightManager->RenameLight(oldRegisteredName, uniqueName, light);
        }
    }

    RegisterSingle(objectPtr, objectPtr->Name);
    return true;
}

std::vector<QEOrderRenderItem> GameObjectManager::BuildRenderItems() const
{
    std::vector<QEOrderRenderItem> items;
    glm::vec3 cameraPosition(0.0f);
    if (auto activeCamera = QECameraContext::getInstance()->ActiveCamera())
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

            if (!go->IsActiveInHierarchy())
                continue;

            auto meshRenderer = go->GetComponent<QEMeshRenderer>();
            if (!meshRenderer)
                continue;

            auto geometry = go->GetComponent<QEGeometryComponent>();
            if (!geometry)
                continue;

            auto mesh = geometry->GetMesh();
            if (!mesh)
                continue;

            const auto subMeshCount = static_cast<uint32_t>(mesh->MeshData.size());
            for (uint32_t subMeshIndex = 0; subMeshIndex < subMeshCount; ++subMeshIndex)
            {
                std::shared_ptr<QEMaterial> material = nullptr;
                if (subMeshIndex < mesh->MaterialRel.size())
                {
                    material = go->GetMaterial(mesh->MaterialRel[subMeshIndex]);
                }
                if (!material)
                {
                    material = go->GetMaterial();
                }
                if (!material)
                    continue;

                QEOrderRenderItem item;
                item.GameObject = go;
                item.MeshRenderer = meshRenderer;
                item.Material = material;
                item.SubMeshIndex = subMeshIndex;
                item.RenderQueue = material->renderQueue;
                if (auto transform = go->GetComponent<QETransform>())
                {
                    const glm::vec3 delta = transform->GetWorldPosition() - cameraPosition;
                    item.CameraDistanceSq = glm::dot(delta, delta);
                }

                items.push_back(item);
            }
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

            if (!go->IsActiveInHierarchy())
                continue;

            auto meshRenderer = go->GetComponent<QEMeshRenderer>();
            if (!meshRenderer)
                continue;

            auto geometry = go->GetComponent<QEGeometryComponent>();
            if (!geometry)
                continue;

            auto mesh = geometry->GetMesh();
            if (!mesh)
                continue;

            const auto subMeshCount = static_cast<uint32_t>(mesh->MeshData.size());
            for (uint32_t subMeshIndex = 0; subMeshIndex < subMeshCount; ++subMeshIndex)
            {
                std::shared_ptr<QEMaterial> material = nullptr;
                if (subMeshIndex < mesh->MaterialRel.size())
                {
                    material = go->GetMaterial(mesh->MaterialRel[subMeshIndex]);
                }
                if (!material)
                {
                    material = go->GetMaterial();
                }
                if (!material)
                    continue;

                if (material->renderQueue >= static_cast<unsigned int>(RenderQueue::Transparent))
                    continue;

                QEOrderRenderItem item;
                item.GameObject = go;
                item.MeshRenderer = meshRenderer;
                item.Material = material;
                item.SubMeshIndex = subMeshIndex;
                item.RenderQueue = material->renderQueue;

                items.push_back(item);
            }
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
    if (auto* deviceModule = DeviceModule::getInstance())
    {
        vkDeviceWaitIdle(deviceModule->device);
    }

    ReleaseAllGameObjects();
    CleanLastResources();
}

void GameObjectManager::RemoveLightsFromHierarchy(const std::shared_ptr<QEGameObject>& go)
{
    if (!go)
        return;

    for (const auto& child : go->GetChildren())
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

    const auto& pointLights = lightManager->GetPointLights();
    for (uint32_t i = 0; i < pointLights.size(); ++i)
    {
        if (pointLights[i])
        {
            pointLights[i]->idxShadowMap = i;
        }
    }

    const auto& directionalLights = lightManager->GetDirectionalLights();
    for (uint32_t i = 0; i < directionalLights.size(); ++i)
    {
        if (directionalLights[i])
        {
            directionalLights[i]->idxShadowMap = i;
        }
    }

    const auto& spotLights = lightManager->GetSpotLights();
    for (uint32_t i = 0; i < spotLights.size(); ++i)
    {
        if (spotLights[i])
        {
            spotLights[i]->idxShadowMap = i;
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
