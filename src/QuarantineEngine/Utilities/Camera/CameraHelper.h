#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

inline glm::vec3 CamForward(const glm::quat& q) { return q * glm::vec3(0, 0, -1); }

inline glm::vec3 CamRight(const glm::quat& q) { return q * glm::vec3(1, 0, 0); }

inline glm::vec3 ProjectOnPlane(const glm::vec3& v, const glm::vec3& n)
{
    return v - n * glm::dot(v, n);
}
