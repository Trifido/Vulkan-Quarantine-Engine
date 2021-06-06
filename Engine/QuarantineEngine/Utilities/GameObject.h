#pragma once
#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include "Mesh.h"
#include "Transform.h"
#include "Material.h"

class GameObject
{
public:
    std::shared_ptr<Mesh> mesh;
    std::shared_ptr<Transform> transform;
    std::shared_ptr<Material> material;

public:
    GameObject() {}
    GameObject(std::string meshPath, std::string albedoPath);
};

#endif
