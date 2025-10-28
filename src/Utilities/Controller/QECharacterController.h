#pragma once
#ifndef QE_CHARACTER_CONTROLLER
#define QE_CHARACTER_CONTROLLER

#include <QEGameComponent.h>
#include "QETransform.h"
#include <PhysicsBody.h>
#include <Collider.h>
#include <btBulletDynamicsCommon.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <AnimationComponent.h>
#include <QESpringArmComponent.h>


// Callback para ignorar tu propio cuerpo
struct IgnoreSelfConvexResult : public btCollisionWorld::ClosestConvexResultCallback {
    const btCollisionObject* ignore = nullptr;
    IgnoreSelfConvexResult(const btVector3& from, const btVector3& to, const btCollisionObject* ig = nullptr)
        : btCollisionWorld::ClosestConvexResultCallback(from, to), ignore(ig) {}

    bool needsCollision(btBroadphaseProxy* proxy0) const override {
        if (!btCollisionWorld::ClosestConvexResultCallback::needsCollision(proxy0))
            return false;
        return proxy0->m_clientObject != ignore;
    }
};

class QECharacterController : public QEGameComponent
{
    REFLECTABLE_DERIVED_COMPONENT(QECharacterController, QEGameComponent)
    private:
        REFLECT_PROPERTY(std::string, colliderID)
        REFLECT_PROPERTY(std::string, physicBodyID)

        REFLECT_PROPERTY(float, _moveSpeed)
        REFLECT_PROPERTY(float, _sprintSpeed)
        REFLECT_PROPERTY(float, _gravity)
        REFLECT_PROPERTY(float, _jumpSpeed)
        REFLECT_PROPERTY(float, _maxSlopeCos)

        REFLECT_PROPERTY(bool, _grounded)
        float _vY;
        float _capsuleRadius;
        float _capsuleHalfHeight;

        std::shared_ptr<QETransform> transform;
        std::shared_ptr<QECollider> colliderPtr;
        std::shared_ptr<PhysicsBody> physicBodyPtr;
        std::shared_ptr<AnimationComponent> animationComponentPtr;

    private:
        // Movimiento
        glm::vec3 ReadInputHorizontal(float dt) const;
        void      ApplyKinematicMove(const glm::vec3& desiredDelta);

        // Colisiones
        bool      CapsuleSweep(const glm::vec3& start, const glm::vec3& end,
            glm::vec3& hitPoint, glm::vec3& hitNormal, float& hitFrac) const;
        bool      CheckGrounded(bool* outOnSlope = nullptr);

        // Utilidades
        void      EnsureKinematicFlags();
        void      SyncTransformToBullet() const;
        btTransform GlmToBt(const glm::vec3& p, const glm::quat& q) const;

    public:
        QECharacterController();

        void QEStart() override;
        void QEInit() override;
        void QEUpdate() override;
        void QEDestroy() override;
};

#endif // !QE_CHARACTER_CONTROLLER

