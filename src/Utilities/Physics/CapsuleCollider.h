#pragma once
#ifndef CAPSULE_COLLIDER_H
#define CAPSULE_COLLIDER_H

#include "Collider.h"
class CapsuleCollider : public Collider
{
private:
    float radius;
    float height;

public:
    CapsuleCollider();
    CapsuleCollider(float newRadius, float newHeight);

    float GetRadius() const { return radius; }
    float GetHeight() const { return height; }

    void SetSize(float newRadius, float newHeight);
};

#endif // !CAPSULE_COLLIDER_H



