#pragma once
#ifndef COLLIDER_H
#define COLLIDER_H

#include "QEGameComponent.h"
#include "btBulletDynamicsCommon.h"

class QECollider : public QEGameComponent
{
protected:
    REFLECTABLE_DERIVED_COMPONENT(QECollider, QEGameComponent)
public:
    btCollisionShape* colShape = nullptr;

    REFLECT_PROPERTY(float, CollisionMargin)
    REFLECT_PROPERTY(glm::vec3, LocalDisplacement)

    QECollider();
    void QEStart() override;
    void QEInit() override;
    void QEUpdate() override;
    void QEDestroy() override;
};

#endif 
