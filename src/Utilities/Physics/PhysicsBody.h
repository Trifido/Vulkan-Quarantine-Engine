#pragma once
#ifndef PHYSICS_BODY_H
#define PHYSICS_BODY_H

#include <Transform.h>
#include <Collider.h>
#include <glm/matrix.hpp>
#include <btBulletDynamicsCommon.h>

enum PhysicBodyType
{
    STATIC_BODY,
    RIGID_BODY
};

enum CollisionFlag
{
    COL_NOTHING = 0,
    COL_DEFAULT = 1 << 0,
    COL_PLAYER = 1 << 1,
    COL_SCENE = 1 << 2,
    COL_ENEMY = 1 << 3,
    COL_TRIGGER = 1 << 4,
    COL_ALL = -1
};

class PhysicsBody : public QEGameComponent
{
private:
    btVector3 localInertia;
    std::shared_ptr<Transform> transform;
    std::shared_ptr<QECollider> collider;
public:
    btRigidBody* body;
    PhysicBodyType Type;
    float Mass;
    glm::vec3 Inertia;
    CollisionFlag CollisionGroup;
    CollisionFlag CollisionMask;
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

    glm::mat4 bulletToGlm(const btTransform& t);
    glm::vec3 bulletToGlm(const btVector3& v);
    glm::quat bulletToGlm(const btQuaternion& q);

    btTransform glmToBullet(const glm::mat4& m);
    btVector3 glmToBullet(const glm::vec3& v);
    btMatrix3x3 glmToBullet(const glm::mat3& m);
    btQuaternion glmToBullet(const glm::quat& q);
};

#endif
