#include "GameObject.h"

GameObject::GameObject(std::string meshPath, std::string albedoPath)
{
    material = std::make_shared<Material>();
    material->addAlbedo(albedoPath);

    mesh = std::make_shared<Mesh>();
    mesh->loadMesh(meshPath);

    transform = std::make_shared<Transform>();
}
