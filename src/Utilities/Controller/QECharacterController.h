#pragma once
#ifndef QE_CHARACTER_CONTROLLER
#define QE_CHARACTER_CONTROLLER

#include <GameComponent.h>
#include <Transform.h>
#include <PhysicBody.h>
#include <Collider.h>
#include <btBulletDynamicsCommon.h>
#include <GLFW/glfw3.h>
#include <memory>

struct IgnoreSelfCallback : public btCollisionWorld::ClosestConvexResultCallback
{
    const btCollisionObject* ignoreObject;
    btVector3 hitNormal;  // Aquí guardamos la normal del impacto

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
            return 1.0f; // Ignorar colisión con uno mismo

        // Guarda la normal
        if (normalInWorldSpace)
            hitNormal = convexResult.m_hitNormalLocal;
        else
        {
            // Si no está en espacio mundial, transformarla
            hitNormal = convexResult.m_hitCollisionObject->getWorldTransform().getBasis() * convexResult.m_hitNormalLocal;
        }

        return ClosestConvexResultCallback::addSingleResult(convexResult, normalInWorldSpace);
    }
};

class QECharacterController : public GameComponent
{
    private:
        std::shared_ptr<Collider> colliderPtr;
        std::shared_ptr<PhysicBody> physicBodyPtr;
        bool isGrounded = false;
        bool canWalkOnGround = false;
        btVector3 groundNormal = btVector3(0, 1, 0);
        float airControlFactor = 0.8f;
        float jumpForce = 8.0f;
        float maxStepAngle = 35.0f;

    private:
        void CheckIfGrounded();
        void Jump();
        void Move(const btVector3& direction, float speed);
        bool CanMove(const btVector3& direction, float distance, btVector3& outAdjustedDir);

    public:
        void Initialize();
        void BindGameObjectProperties(std::shared_ptr<PhysicBody> physicBody, std::shared_ptr<Collider> collider);
        void Update();
        btVector3 GetPosition();
        static void ProcessInput(GLFWwindow* window, std::shared_ptr<QECharacterController> player);
        void SetAirControlFactor(float factor) { airControlFactor = factor; }
        void SetJumpForce(float force) { jumpForce = force; }
};

#endif // !QE_CHARACTER_CONTROLLER

