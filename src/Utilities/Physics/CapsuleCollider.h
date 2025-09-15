#pragma once
#ifndef CAPSULE_COLLIDER_H
#define CAPSULE_COLLIDER_H

#include "Collider.h"
class CapsuleCollider : public QECollider
{
    REFLECTABLE_DERIVED_COMPONENT(CapsuleCollider, QECollider)
protected:
    REFLECT_PROPERTY(float, radius)
    REFLECT_PROPERTY(float, height)

public:
    void QEStart() override;

    CapsuleCollider();
    CapsuleCollider(float newRadius, float newHeight);

    float GetRadius() const { return radius; }
    float GetHeight() const { return height; }

private:
    void SetSize(float newRadius, float newHeight);
};

#endif // !CAPSULE_COLLIDER_H



