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
    std::shared_ptr<QECollider> previousCollider;

    // Estado previo/actual para interpolación
    glm::vec3 prevPos{}, currPos{};
    glm::quat prevRot{}, currRot{};
    bool hasCurr{ false };
    bool applyingPhysicsTransform{ false };
    uint32_t lastTransformVersion{ 0 };

    glm::vec3 lastWorldScale{ 1.0f };
    glm::vec3 lastColliderVecA{ 0.0f };
    float lastColliderFloatA{ 0.0f };
    float lastColliderMargin{ 0.0f };
    float lastFriction{ 0.2f };
    float lastRestitution{ 0.0f };
    float lastLinearDamping{ 0.05f };
    float lastAngularDamping{ 0.05f };
    PhysicBodyType lastBodyType{ STATIC_BODY };
    CollisionFlag lastCollisionGroup{ COL_SCENE };
    CollisionFlag lastCollisionMask{ COL_ALL };
    float lastMass{ 0.0f };
    int lastColliderKind{ 0 };

    // Epsilons
    float posEps = 0.0005f;
    float angEps = 0.0015f;

public:
    JPH::BodyID body = JPH::BodyID();
    REFLECT_PROPERTY(PhysicBodyType, Type)
    REFLECT_PROPERTY(float, Mass)
    REFLECT_PROPERTY(glm::vec3, Inertia)
    REFLECT_PROPERTY(CollisionFlag, CollisionGroup)
    REFLECT_PROPERTY(CollisionFlag, CollisionMask)
    REFLECT_PROPERTY(float, Friction)
    REFLECT_PROPERTY(float, Restitution)
    REFLECT_PROPERTY(float, LinearDamping)
    REFLECT_PROPERTY(float, AngularDamping)

public:
    PhysicsBody();
    PhysicsBody(const PhysicBodyType& type);
    void UpdateType(const PhysicBodyType& type);
    void UpdateInertia(const glm::vec3& localInertia);

    void QEStart() override;
    void QEInit() override;
    void QEUpdate() override;
    void QEDestroy() override;
    void RefreshEditorState();
private:
    void Initialize();
    void copyTransformtoGLM();
    void RebuildScaledShapeFromCollider();
    void UpdateTransform();
    void PushTransformToPhysics(bool resetVelocities);
    void SyncShapeAndBodyToEditorState();
    void CaptureStateSnapshot();
    bool HasColliderDefinitionChanged() const;
    bool HasBodyRecreationConfigurationChanged() const;
    bool HasBodyMaterialConfigurationChanged() const;
    void ApplyRuntimeMaterialProperties();
    void DestroyRuntimeBody();
    void RecreateBody();
    JPH::ObjectLayer ResolveObjectLayer(const PhysicBodyType type, const CollisionFlag group);
    static CollisionFlag SanitizeCollisionGroup(CollisionFlag group);
    static unsigned int SanitizeCollisionMask(CollisionFlag mask);

    static inline JPH::Vec3  toJPH(const glm::vec3& v);
    static inline JPH::Quat  toJPH(const glm::quat& q);
    static inline glm::vec3  toGLM(const JPH::Vec3& v);
    static inline glm::quat  toGLM(const JPH::Quat& q);
};



namespace QE
{
    using ::PhysicsBody;
} // namespace QE
// QE namespace aliases
#endif

