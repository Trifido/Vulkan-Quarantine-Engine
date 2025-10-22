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

struct IgnoreSelfCallback : public btCollisionWorld::ClosestConvexResultCallback
{
    const btCollisionObject* ignoreObject;
    btVector3 hitNormal;

    IgnoreSelfCallback(const btVector3& from, const btVector3& to, const btCollisionObject* ignoreObj)
        : btCollisionWorld::ClosestConvexResultCallback(from, to), ignoreObject(ignoreObj), hitNormal(btVector3(0, 0, 0)) {}

    bool needsCollision(btBroadphaseProxy* proxy0) const override
    {
        if (!btCollisionWorld::ClosestConvexResultCallback::needsCollision(proxy0))
            return false;

        return proxy0->m_clientObject != ignoreObject;
    }

    btScalar addSingleResult(btCollisionWorld::LocalConvexResult& convexResult, bool normalInWorldSpace) override
    {
        if (convexResult.m_hitCollisionObject == ignoreObject)
            return 1.0f;

        if (normalInWorldSpace)
            hitNormal = convexResult.m_hitNormalLocal;
        else
        {
            hitNormal = convexResult.m_hitCollisionObject->getWorldTransform().getBasis() * convexResult.m_hitNormalLocal;
        }

        return ClosestConvexResultCallback::addSingleResult(convexResult, normalInWorldSpace);
    }
};

class QECharacterController : public QEGameComponent
{
    REFLECTABLE_DERIVED_COMPONENT(QECharacterController, QEGameComponent)
    private:

        REFLECT_PROPERTY(std::string, colliderID)
        REFLECT_PROPERTY(std::string, physicBodyID)
        REFLECT_PROPERTY(bool, isGrounded)
        REFLECT_PROPERTY(bool, canWalkOnGround)
        REFLECT_PROPERTY(float, airControlFactor)
        REFLECT_PROPERTY(float, jumpForce)
        REFLECT_PROPERTY(float, maxStepAngle)
        REFLECT_PROPERTY(btVector3, groundNormal)
        GLFWwindow* window = nullptr;
        std::shared_ptr<QECollider> colliderPtr;
        std::shared_ptr<PhysicsBody> physicBodyPtr;
        std::shared_ptr<AnimationComponent> animationComponentPtr;
        std::shared_ptr<QESpringArmComponent> springArmPtr;

    private:
        //Buttons
        bool isPressedAttackButton = false;
        bool isPressedJumpButton = false;
        std::shared_ptr<QETransform> visualTr;

    private:
        void CheckIfGrounded();
        void Jump();
        void Move(const btVector3& direction, float speed);
        bool CanMove(const btVector3& direction, float distance, btVector3& outAdjustedDir);
        void Initialize();
        void KeyInputTrigger(int key, int action, bool& lastAction, string animationState);

    public:
        QECharacterController();
        void AddGLFWWindow(GLFWwindow* window) { this->window = window; }
        void Update();
        btVector3 GetPosition();
        void ProcessInput();
        void SetAirControlFactor(float factor) { airControlFactor = factor; }
        void SetJumpForce(float force) { jumpForce = force; }

        void QEStart() override;
        void QEInit() override;
        void QEUpdate() override;
        void QEDestroy() override;
};

#endif // !QE_CHARACTER_CONTROLLER

