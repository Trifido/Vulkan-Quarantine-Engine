#pragma once
#ifndef PLANE_COLLIDER_H
#define PLANE_COLLIDER_H

#include <Collider.h>

class PlaneCollider : public QECollider
{
    REFLECTABLE_DERIVED_COMPONENT(PlaneCollider, QECollider)
protected:
    REFLECT_PROPERTY(float, Size)
    REFLECT_PROPERTY(glm::vec3, Orientation)

public:
    void QEStart() override;

    PlaneCollider();
    PlaneCollider(const float& newSize, const glm::vec3& newOrientation);
    const float GetSize();
    const glm::vec3 GetOrientation();
    void SetPlane(const float& newSize, const glm::vec3& newOrientation);
};

#endif 
