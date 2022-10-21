#pragma once
#ifndef PLANE_COLLIDER_H
#define PLANE_COLLIDER_H

#include <Collider.h>

class PlaneCollider : public Collider
{
protected:
    float Size;
    glm::vec3 Orientation;
public:
    PlaneCollider();
    PlaneCollider(const float& newSize, const glm::vec3& newOrientation);
    const float GetSize();
    const glm::vec3 GetOrientation();
    void SetPlane(const float& newSize, const glm::vec3& newOrientation);
};

#endif 
