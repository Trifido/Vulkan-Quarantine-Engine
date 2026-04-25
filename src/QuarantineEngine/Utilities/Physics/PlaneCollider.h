#pragma once
#ifndef PLANE_COLLIDER_H
#define PLANE_COLLIDER_H

#include <Collider.h>
#include <glm/vec2.hpp>

class PlaneCollider : public QECollider
{
    REFLECTABLE_DERIVED_COMPONENT(PlaneCollider, QECollider)
protected:
    REFLECT_PROPERTY(float, Size)
    REFLECT_PROPERTY(glm::vec2, Extents)
    REFLECT_PROPERTY(glm::vec3, Orientation)

public:
    void QEStart() override;
    void QEInit() override;

    PlaneCollider();
    PlaneCollider(const float& newSize, const glm::vec3& newOrientation);
    const float GetSize();
    const glm::vec2 GetExtents();
    const glm::vec3 GetOrientation();
    void SetPlane(const float& newSize, const glm::vec3& newOrientation);

private:
    bool TryAutoFit();
};



namespace QE
{
    using ::PlaneCollider;
} // namespace QE
// QE namespace aliases
#endif 
