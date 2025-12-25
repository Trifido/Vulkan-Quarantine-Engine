#pragma once

#ifndef MATH_HELPERS
#define MATH_HELPERS

#include <glm/glm.hpp>

namespace QEHelper
{
    static glm::vec3 DirectionToEulerDegrees(glm::vec3 dir)
    {
        const float eps = 1e-6f;
        float len = glm::length(dir);
        if (len < eps) return glm::vec3(-45.f, 45.f, 0.f); // fallback
        dir /= len;

        // Forward base = (0,0,-1)
        float yaw = std::atan2(dir.x, -dir.z);                                      // Y
        float pitch = std::atan2(dir.y, std::sqrt(dir.x * dir.x + dir.z * dir.z));  // X

        return glm::degrees(glm::vec3(pitch, yaw, 0.0f));
    }
}

#endif // !MATH_HELPERS
