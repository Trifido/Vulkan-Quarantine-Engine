#pragma once
#ifndef PHYSICS_BODY_H
#define PHYSICS_BODY_H

#include "QETransform.h"
#include <Collider.h>
#include <glm/matrix.hpp>
#include "PhysicsTypes.h"

// Jolt
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Body/BodyLockInterface.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Math/Quat.h>
#include <Jolt/Math/Vec3.h>
#include <Jolt/Math/Mat44.h>

class PhysicsBody : public QEGameComponent
{
    REFLECTABLE_DERIVED_COMPONENT(PhysicsBody, QEGameComponent)
private:
    std::shared_ptr<QETransform> transform;
    std::shared_ptr<QECollider> collider;

    // Estado previo/actual para interpolación
    glm::vec3 prevPos{}, currPos{};
    glm::quat prevRot{}, currRot{};
    bool hasCurr{ false };

    // Epsilons
    float posEps = 0.0005f;
    float angEps = 0.0015f;

public:
    JPH::BodyID body = JPH::BodyID();
    REFLECT_PROPERTY(PhysicBodyType, Type)
    REFLECT_PROPERTY(float, Mass)
    REFLECT_PROPERTY(glm::vec3, Inertia)
    REFLECT_PROPERTY(CollisionFlag, CollisionGroup)

public:
    PhysicsBody();
    PhysicsBody(const PhysicBodyType& type);
    void UpdateType(const PhysicBodyType& type);
    void UpdateInertia(const glm::vec3& localInertia);

    void QEStart() override;
    void QEInit() override;
    void QEUpdate() override;
    void QEDestroy() override;
private:
    void Initialize();
    void copyTransformtoGLM();
    void RebuildScaledShapeFromCollider();
    void UpdateTransform();
    JPH::ObjectLayer ResolveObjectLayer(const PhysicBodyType type, const CollisionFlag group);

    static inline JPH::Vec3  toJPH(const glm::vec3& v);
    static inline JPH::Quat  toJPH(const glm::quat& q);
    static inline glm::vec3  toGLM(const JPH::Vec3& v);
    static inline glm::quat  toGLM(const JPH::Quat& q);
};

#endif
