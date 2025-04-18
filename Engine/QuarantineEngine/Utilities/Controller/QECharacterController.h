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

class QECharacterController : public GameComponent
{
    private:
        std::shared_ptr<PhysicBody> physicBodyPtr;
        bool isGrounded = false;
        float airControlFactor = 0.8f;

    private:
        void CheckIfGrounded(btDiscreteDynamicsWorld* world);
        void Jump(float force = 8.0f);
        void Move(const btVector3& direction, float speed);

    public:
        void Initialize();
        void BindGameObjectProperties(std::shared_ptr<PhysicBody> physicBodyPtr);
        void Update(btDiscreteDynamicsWorld* world);
        btVector3 GetPosition();
        static void ProcessInput(GLFWwindow* window, std::shared_ptr<QECharacterController> player);
        void SetAirControlFactor(float factor) { airControlFactor = factor; }
};

#endif // !QE_CHARACTER_CONTROLLER

