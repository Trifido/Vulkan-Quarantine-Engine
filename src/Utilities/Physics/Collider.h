#pragma once
#ifndef COLLIDER_H
#define COLLIDER_H

#include "QEGameComponent.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>

class QECollider : public QEGameComponent
{
protected:
    REFLECTABLE_DERIVED_COMPONENT(QECollider, QEGameComponent)
public:
    JPH::Ref<JPH::Shape> colShape = nullptr;

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
