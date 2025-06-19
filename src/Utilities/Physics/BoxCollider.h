#pragma once
#ifndef BOX_COLLIDER_H
#define BOX_COLLIDER_H

#include <Collider.h>

class BoxCollider : public QECollider
{
    REFLECTABLE_DERIVED_COMPONENT(BoxCollider, QECollider)
protected:
    REFLECT_PROPERTY(glm::vec3, Size)
public:
    BoxCollider();
    BoxCollider(const glm::vec3& newSize);
    const glm::vec3 GetSize();
    void SetSize(const glm::vec3& value);
};

#endif 
