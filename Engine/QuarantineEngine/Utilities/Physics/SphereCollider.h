#pragma once
#ifndef SPHERE_COLLIDER_H
#define SPHERE_COLLIDER_H

#include <Collider.h>

class SphereCollider : public Collider
{
protected:
    float Radius;
public:
    SphereCollider();
    SphereCollider(const float& radius);
    const float GetRadius();
    void SetRadius(const float& value);
};

#endif 
