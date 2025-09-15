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
    btCompoundShape* compound = nullptr;

    REFLECT_PROPERTY(float, CollisionMargin)

    QECollider();
    void QEStart() override;
    void QEInit() override;
    void QEUpdate() override;
    void QEDestroy() override;

protected:
    void SetColliderPivot(glm::vec3 displacement);
};

#endif 
