#pragma once
#ifndef FRUSTUM_COMPONENT_H
#define FRUSTUM_COMPONENT_H

#include <glm/glm.hpp>
#include <AABBObject.h>

class FrustumComponent
{
private:
    glm::vec4 initCorners[8];

public:
    glm::vec4 frustumPlanes[6];
    glm::vec4 corners[8];

private:
    void RecreateFrustumCorners(glm::mat4 viewProjection);

public:
    FrustumComponent();
    void RecreateFrustum(glm::mat4 viewProjection);
    bool isAABBInside(const AABBObject& box);
};

#endif // !FRUSTUM_COMPONENT_H
