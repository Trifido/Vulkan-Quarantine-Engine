#pragma once
#ifndef COLLIDER_H
#define COLLIDER_H

#include "GameComponent.h"
#include "btBulletDynamicsCommon.h"

class Collider : public GameComponent
{
public:
    btCollisionShape* colShape = nullptr;
    float CollisionMargin = 0.04f;
    glm::vec3 LocalDisplacement = glm::vec3(0.0f);
};

#endif 



