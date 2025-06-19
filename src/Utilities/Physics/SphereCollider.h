#pragma once
#ifndef SPHERE_COLLIDER_H
#define SPHERE_COLLIDER_H

#include <Collider.h>

class SphereCollider : public QECollider
{
    REFLECTABLE_DERIVED_COMPONENT(SphereCollider, QECollider)
protected:
    REFLECT_PROPERTY(float, Radius)

public:
    SphereCollider();
    SphereCollider(const float& radius);
    const float GetRadius();
    void SetRadius(const float& value);
};

#endif 
