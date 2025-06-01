#pragma once
#ifndef COLLIDER_H
#define COLLIDER_H

#include "QEGameComponent.h"
#include "btBulletDynamicsCommon.h"

class QECollider : public QEGameComponent
{
public:
    btCollisionShape* colShape = nullptr;
    float CollisionMargin = 0.04f;
    glm::vec3 LocalDisplacement = glm::vec3(0.0f);

    void QEStart() override;
    void QEInit() override;
    void QEUpdate() override;
    void QEDestroy() override;
};

#endif 
