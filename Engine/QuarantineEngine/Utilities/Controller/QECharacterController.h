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

    IgnoreSelfCallback(const btVector3& from, const btVector3& to, const btCollisionObject* ignoreObj)
        : btCollisionWorld::ClosestConvexResultCallback(from, to), ignoreObject(ignoreObj) {}

    bool needsCollision(btBroadphaseProxy* proxy0) const override
    {
        // Filtrado estándar
        if (!btCollisionWorld::ClosestConvexResultCallback::needsCollision(proxy0))
            return false;

        // Ignorar el propio cuerpo
        return proxy0->m_clientObject != ignoreObject;
    }
};


class QECharacterController : public GameComponent
{
    private:
        std::shared_ptr<PhysicBody> physicBodyPtr;
        bool isGrounded = false;
        bool canWalkOnGround = false;
        btVector3 groundNormal = btVector3(0, 1, 0);
        float airControlFactor = 0.8f;
        float jumpForce = 8.0f;

    private:
        void CheckIfGrounded();
        void Jump();
        void Move(const btVector3& direction, float speed);
        bool CanMove(const btVector3& direction, float distance);

    public:
        void Initialize();
        void BindGameObjectProperties(std::shared_ptr<PhysicBody> physicBodyPtr);
        void Update();
        btVector3 GetPosition();
        static void ProcessInput(GLFWwindow* window, std::shared_ptr<QECharacterController> player);
        void SetAirControlFactor(float factor) { airControlFactor = factor; }
        void SetJumpForce(float force) { jumpForce = force; }
};

#endif // !QE_CHARACTER_CONTROLLER

