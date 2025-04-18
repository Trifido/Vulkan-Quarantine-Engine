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

class PhysicBody : public GameComponent
{
private:
    btVector3 localInertia;
    std::shared_ptr<Transform> transform;
    std::shared_ptr<Collider> collider;
public:
    btRigidBody* body;
    PhysicBodyType Type;
    float Mass;
    glm::vec3 Inertia;
public:
    PhysicBody();
    PhysicBody(const PhysicBodyType& type);
    void UpdateType(const PhysicBodyType& type);
    void UpdateInertia(const glm::vec3& localInertia);
    void Initialize(std::shared_ptr<Transform> transform_ptr, std::shared_ptr<Collider> collider_ptr);

    void UpdateTransform();
private:
    void copyTransformtoGLM();

    glm::mat4 bulletToGlm(const btTransform& t);
    glm::vec3 bulletToGlm(const btVector3& v);
    glm::quat bulletToGlm(const btQuaternion& q);

    btTransform glmToBullet(const glm::mat4& m);
    btVector3 glmToBullet(const glm::vec3& v);
    btMatrix3x3 glmToBullet(const glm::mat3& m);
    btQuaternion glmToBullet(const glm::quat& q);

};

#endif
