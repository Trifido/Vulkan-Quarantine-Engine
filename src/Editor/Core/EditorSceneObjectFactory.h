#pragma once

#include <memory>
#include <string>
#include <glm/vec3.hpp>

class GameObjectManager;
class MaterialManager;
class EditorSelectionManager;
class QESessionManager;
class QEGameObject;
class IQEMeshGenerator;

enum class QEPrimitiveType
{
    Cube,
    Plane,
    Sphere,
    Cylinder,
    Cone,
    Pyramid,
    Capsule,
    Torus
};

class EditorSceneObjectFactory
{
public:
    EditorSceneObjectFactory(
        GameObjectManager* gameObjectManager,
        MaterialManager* materialManager,
        EditorSelectionManager* selectionManager,
        QESessionManager* sessionManager);

    std::shared_ptr<QEGameObject> CreatePrimitive(QEPrimitiveType type, float spawnDistance = 5.0f);
    std::shared_ptr<QEGameObject> CreateDroppedMeshObject(const std::string& assetPath, float spawnDistance = 5.0f);

private:
    std::string GetPrimitiveName(QEPrimitiveType type) const;
    std::unique_ptr<IQEMeshGenerator> CreatePrimitiveGenerator(QEPrimitiveType type) const;
    glm::vec3 GetSpawnPositionInFrontOfEditorCamera(float distance) const;
    void FinalizeCreatedObject(const std::shared_ptr<QEGameObject>& gameObject, float spawnDistance) const;

private:
    GameObjectManager* gameObjectManager = nullptr;
    MaterialManager* materialManager = nullptr;
    EditorSelectionManager* selectionManager = nullptr;
    QESessionManager* sessionManager = nullptr;
};
