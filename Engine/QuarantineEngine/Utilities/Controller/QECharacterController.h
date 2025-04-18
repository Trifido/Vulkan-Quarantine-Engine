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

    public:
        void BindGameObjectProperties(std::shared_ptr<PhysicBody> physicBodyPtr);
        void MoveForward();
        void MoveBackward();
        void MoveLeft();
        void MoveRight();
        void Jump();
        void Update(btDiscreteDynamicsWorld* world);
        btVector3 GetPosition();
        static void ProcessInput(GLFWwindow* window, std::shared_ptr<QECharacterController> player);
    };

#endif // !QE_CHARACTER_CONTROLLER

