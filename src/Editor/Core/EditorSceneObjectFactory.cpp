#include "EditorSceneObjectFactory.h"

#include <GameObjectManager.h>
#include <MaterialManager.h>
#include <Editor/Core/EditorSelectionManager.h>
#include <QESessionManager.h>

#include <QEGameObject.h>
#include <QETransform.h>
#include <QECamera.h>
#include <QEGeometryComponent.h>
#include <QEMeshRenderer.h>
#include <QEMeshGenerator.h>

#include <LightManager.h>
#include <PointLight.h>
#include <DirectionalLight.h>
#include <SpotLight.h>

#include <filesystem>
#include <glm/geometric.hpp>

EditorSceneObjectFactory::EditorSceneObjectFactory(
    GameObjectManager* gameObjectManager,
    MaterialManager* materialManager,
    EditorSelectionManager* selectionManager,
    QESessionManager* sessionManager)
    : gameObjectManager(gameObjectManager)
    , materialManager(materialManager)
    , selectionManager(selectionManager)
    , sessionManager(sessionManager)
{
}

std::shared_ptr<QEGameObject> EditorSceneObjectFactory::CreatePrimitive(QEPrimitiveType type, float spawnDistance)
{
    if (!gameObjectManager || !materialManager)
        return nullptr;

    const std::string objectName = GetPrimitiveName(type);

    auto generator = CreatePrimitiveGenerator(type);
    if (!generator)
        return nullptr;

    auto newObject = std::make_shared<QEGameObject>(objectName);

    newObject->AddComponent(std::make_shared<QEGeometryComponent>(std::move(generator)));
    newObject->AddComponent(std::make_shared<QEMeshRenderer>());

    if (auto defaultMaterial = materialManager->GetMaterial("defaultPrimitiveMat"))
    {
        newObject->AddComponent(defaultMaterial);
    }

    gameObjectManager->AddGameObject(newObject);
    FinalizeCreatedObject(newObject, spawnDistance);

    return newObject;
}

std::shared_ptr<QEGameObject> EditorSceneObjectFactory::CreateDroppedMeshObject(const std::string& assetPath, float spawnDistance)
{
    if (!gameObjectManager)
        return nullptr;

    if (assetPath.empty())
        return nullptr;

    std::filesystem::path path(assetPath);
    if (!std::filesystem::exists(path))
        return nullptr;

    const std::string objectName = path.stem().string();

    auto newObject = std::make_shared<QEGameObject>(objectName);
    newObject->AddComponent(std::make_shared<QEGeometryComponent>(
        std::make_unique<QEMeshGenerator>(path.generic_string())));
    newObject->AddComponent(std::make_shared<QEMeshRenderer>());

    gameObjectManager->AddGameObject(newObject);
    FinalizeCreatedObject(newObject, spawnDistance);

    return newObject;
}

std::string EditorSceneObjectFactory::GetPrimitiveName(QEPrimitiveType type) const
{
    switch (type)
    {
    case QEPrimitiveType::Cube:     return "Cube";
    case QEPrimitiveType::Plane:    return "Plane";
    case QEPrimitiveType::Sphere:   return "Sphere";
    case QEPrimitiveType::Cylinder: return "Cylinder";
    case QEPrimitiveType::Cone:     return "Cone";
    case QEPrimitiveType::Pyramid:  return "Pyramid";
    case QEPrimitiveType::Capsule:  return "Capsule";
    case QEPrimitiveType::Torus:    return "Torus";
    default:                        return "Primitive";
    }
}

std::unique_ptr<IQEMeshGenerator> EditorSceneObjectFactory::CreatePrimitiveGenerator(QEPrimitiveType type) const
{
    switch (type)
    {
    case QEPrimitiveType::Cube:     return std::make_unique<CubeGenerator>();
    case QEPrimitiveType::Plane:    return std::make_unique<FloorGenerator>();
    case QEPrimitiveType::Sphere:   return std::make_unique<SphereGenerator>();
    case QEPrimitiveType::Cylinder: return std::make_unique<CylinderGenerator>();
    case QEPrimitiveType::Cone:     return std::make_unique<ConeGenerator>();
    case QEPrimitiveType::Pyramid:  return std::make_unique<PyramidGenerator>();
    case QEPrimitiveType::Capsule:  return std::make_unique<CapsuleGenerator>();
    case QEPrimitiveType::Torus:    return std::make_unique<TorusGenerator>();
    default:                        return nullptr;
    }
}

glm::vec3 EditorSceneObjectFactory::GetSpawnPositionInFrontOfEditorCamera(float distance) const
{
    auto editorCamera = sessionManager->EditorCamera();
    if (!editorCamera)
        return glm::vec3(0.0f);

    auto cameraOwner = editorCamera->Owner;
    if (!cameraOwner)
        return glm::vec3(0.0f);

    auto cameraTransform = cameraOwner->GetComponent<QETransform>();
    if (!cameraTransform)
        return glm::vec3(0.0f);

    const glm::vec3 cameraPos = cameraTransform->GetWorldPosition();
    const glm::vec3 forward = glm::normalize(cameraTransform->Forward());

    return cameraPos + forward * distance;
}

void EditorSceneObjectFactory::FinalizeCreatedObject(const std::shared_ptr<QEGameObject>& gameObject, float spawnDistance) const
{
    if (!gameObject)
        return;

    if (auto transform = gameObject->GetComponent<QETransform>())
    {
        transform->SetLocalPosition(GetSpawnPositionInFrontOfEditorCamera(spawnDistance));
        transform->SetLocalScale(glm::vec3(1.0f));
    }

    if (selectionManager)
    {
        selectionManager->SelectGameObject(gameObject);
    }
}

std::shared_ptr<QEGameObject> EditorSceneObjectFactory::CreateLight(LightType type, float spawnDistance)
{
    if (!gameObjectManager)
        return nullptr;

    auto lightManager = LightManager::getInstance();
    if (!lightManager)
        return nullptr;

    // 1. Nombre base
    std::string baseName;
    switch (type)
    {
    case LightType::POINT_LIGHT:       baseName = "Point Light"; break;
    case LightType::DIRECTIONAL_LIGHT: baseName = "Directional Light"; break;
    case LightType::SPOT_LIGHT:        baseName = "Spot Light"; break;
    default:                           baseName = "Light"; break;
    }

    // 2. Crear GameObject
    auto newObject = std::make_shared<QEGameObject>(baseName);

    // 3. Crear luz
    auto light = lightManager->CreateLight(type, baseName);
    if (!light)
        return nullptr;

    // 4. Añadir componente
    newObject->AddComponent(light);

    // 5. Añadir a escena
    gameObjectManager->AddGameObject(newObject);

    // 6. Inicializar valores por defecto
    switch (type)
    {
    case LightType::POINT_LIGHT:
    {
        light->diffuse = glm::vec3(1.0f);
        light->specular = glm::vec3(1.0f);
        light->SetDistanceEffect(10.0f);
    }
    break;

    case LightType::DIRECTIONAL_LIGHT:
    {
        light->diffuse = glm::vec3(1.0f);
        light->specular = glm::vec3(1.0f);

        if (auto transform = newObject->GetComponent<QETransform>())
        {
            transform->SetLocalEulerDegrees(glm::vec3(50.0f, -30.0f, 0.0f));
        }
    }
    break;

    case LightType::SPOT_LIGHT:
    {
        light->diffuse = glm::vec3(1.0f);
        light->specular = glm::vec3(1.0f);
        light->SetDistanceEffect(20.0f);

        if (auto transform = newObject->GetComponent<QETransform>())
        {
            transform->SetLocalEulerDegrees(glm::vec3(90.0f, 0.0f, 0.0f));
        }
    }
    break;
    }

    // 7. Registrar en LightManager (IMPORTANTE)
    std::string finalName = baseName;
    lightManager->AddNewLight(light, finalName);

    newObject->Name = finalName;

    // 8. Posición + selección (igual que primitivas)
    FinalizeCreatedObject(newObject, spawnDistance);

    return newObject;
}
