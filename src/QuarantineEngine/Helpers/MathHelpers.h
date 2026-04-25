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

    static float WrapAnglePi(float a)
    {
        while (a > glm::pi<float>()) a -= glm::two_pi<float>();
        while (a < -glm::pi<float>()) a += glm::two_pi<float>();
        return a;
    }

    static float MoveTowardsAngle(float current, float target, float maxDelta)
    {
        float delta = WrapAnglePi(target - current);
        if (delta > maxDelta) delta = maxDelta;
        if (delta < -maxDelta) delta = -maxDelta;
        return current + delta;
    }

    static float WrapPi(float a)
    {
        while (a > glm::pi<float>()) a -= glm::two_pi<float>();
        while (a < -glm::pi<float>()) a += glm::two_pi<float>();
        return a;
    }

    static float YawFromQuat(const glm::quat& q)
    {
        glm::vec3 fwd = q * glm::vec3(0, 0, -1);
        fwd.y = 0.0f;
        if (glm::length2(fwd) > 1e-6f) fwd = glm::normalize(fwd);
        return std::atan2(fwd.x, -fwd.z);
    }
}

#endif // !MATH_HELPERS
