#pragma once

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

inline btVector3 ToBt(const glm::vec3& v)
{
    return btVector3(v.x, v.y, v.z);
}

inline glm::vec3 ToGlm(const btVector3& v)
{
    return glm::vec3(v.x(), v.y(), v.z());
}

inline btQuaternion ToBt(const glm::quat& q)
{
    return btQuaternion(q.x, q.y, q.z, q.w);
}

inline glm::quat ToGlm(const btQuaternion& q)
{
    return glm::quat(q.w(), q.x(), q.y(), q.z());
}

inline btTransform ToBt(const glm::vec3& p, const glm::quat& r)
{
    btTransform t; t.setIdentity(); t.setOrigin(ToBt(p)); t.setRotation(ToBt(r)); return t;
}
