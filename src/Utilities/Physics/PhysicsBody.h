#pragma once
#ifndef PHYSICS_BODY_H
#define PHYSICS_BODY_H

#include "QETransform.h"
#include <Collider.h>
#include <glm/matrix.hpp>
#include <btBulletDynamicsCommon.h>
#include "PhysicsTypes.h"

class PhysicsBody : public QEGameComponent
{
    REFLECTABLE_DERIVED_COMPONENT(PhysicsBody, QEGameComponent)
private:
    std::shared_ptr<QETransform> transform;
    std::shared_ptr<QECollider> collider;
    btVector3 localInertia;
    btTransform lastTrans;
    bool hasLastTrans = false;

    // Epsilons para comparación
    float posEps = 0.0005f;    // ~0.5 mm
    float angEps = 0.0015f;    // ~0.086 rad

public:
    btRigidBody* body;
    REFLECT_PROPERTY(PhysicBodyType, Type)
    REFLECT_PROPERTY(float, Mass)
    REFLECT_PROPERTY(glm::vec3, Inertia)
    REFLECT_PROPERTY(CollisionFlag, CollisionGroup)
    REFLECT_PROPERTY(CollisionFlag, CollisionMask)
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
    void UpdateTransform();

    static inline bool closePos(const btVector3& a, const btVector3& b, btScalar eps);
    static inline bool closeRot(const btQuaternion& a, btQuaternion b, btScalar angleEpsRad);

    glm::mat4 bulletToGlm(const btTransform& t);
    glm::vec3 bulletToGlm(const btVector3& v);
    glm::quat bulletToGlm(const btQuaternion& q);

    btTransform glmToBullet(const glm::mat4& m);
    btVector3 glmToBullet(const glm::vec3& v);
    btMatrix3x3 glmToBullet(const glm::mat3& m);
    btQuaternion glmToBullet(const glm::quat& q);
};

#endif
