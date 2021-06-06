#pragma once
#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include "GameComponent.h"
#include <memory>

class GameObject
{
public:
    std::unique_ptr<GameComponent> mesh;
    std::unique_ptr<GameComponent> transform;
    std::unique_ptr<GameComponent> material;

public:
    GameObject() {}
};

#endif
