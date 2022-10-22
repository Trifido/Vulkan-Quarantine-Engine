#pragma once
#ifndef COLLIDER_H
#define COLLIDER_H

#include "GameComponent.h"
#include "btBulletDynamicsCommon.h"

class Collider : public GameComponent
{
public:
    btCollisionShape* colShape = nullptr;
};

#endif 



