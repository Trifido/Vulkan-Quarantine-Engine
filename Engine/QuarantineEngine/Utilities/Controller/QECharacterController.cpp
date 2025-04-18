#include <QECharacterController.h>

void QECharacterController::Initialize()
{
    this->physicBodyPtr->body->setAngularFactor(0);     // No permitir que rote
    this->physicBodyPtr->body->setDamping(0.9f, 0.9f);  // Linear y angular damping
    this->physicBodyPtr->body->setFriction(1.0f);       // buena fricción
    this->physicBodyPtr->body->setRestitution(0.0f);    // sin rebote
    this->physicBodyPtr->body->setActivationState(DISABLE_DEACTIVATION);
}

void QECharacterController::BindGameObjectProperties(std::shared_ptr<PhysicBody> physicBodyPtr)
{
    this->physicBodyPtr = physicBodyPtr;
}

void QECharacterController::CheckIfGrounded(btDiscreteDynamicsWorld* world)
{
    btTransform trans;
    this->physicBodyPtr->body->getMotionState()->getWorldTransform(trans);
    btVector3 start = trans.getOrigin();
    btVector3 end = start - btVector3(0, 1.2f, 0);

    btCollisionWorld::ClosestRayResultCallback rayCallback(start, end);
    world->rayTest(start, end, rayCallback);
    isGrounded = rayCallback.hasHit();
}

void QECharacterController::Move(const btVector3 & direction, float speed)
{
    btVector3 currentVel = this->physicBodyPtr->body->getLinearVelocity();
    float airControl = this->isGrounded ? 1.0f : this->airControlFactor;

    if (direction.length2() > 0.001f)
    {
        btVector3 vel = direction.normalized() * speed;
        btVector3 desiredVelocity = btVector3(vel.x() * airControl, currentVel.y(), vel.z() * airControl);

        this->physicBodyPtr->body->setLinearVelocity(desiredVelocity);
    }
    else
    {
        this->physicBodyPtr->body->setLinearVelocity(btVector3(0, currentVel.y(), 0));
    }
}

void QECharacterController::Jump(float force)
{
    if (this->isGrounded)
    {
        btVector3 velocity = this->physicBodyPtr->body->getLinearVelocity();
        this->physicBodyPtr->body->setLinearVelocity(btVector3(velocity.x(), force, velocity.z()));
    }
}

// Actualizar la física (para simular la física del personaje)
void QECharacterController::Update(btDiscreteDynamicsWorld* world)
{
    this->CheckIfGrounded(world);
}

// Obtener la posición del personaje
btVector3 QECharacterController::GetPosition()
{
    btTransform trans;
    this->physicBodyPtr->body->getMotionState()->getWorldTransform(trans);
    return trans.getOrigin();
}

void QECharacterController::ProcessInput(GLFWwindow* window, std::shared_ptr<QECharacterController> player)
{
    btVector3 dir(0, 0, 0);
    const float moveSpeed = 5.0f;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) dir += btVector3(0, 0, -1);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) dir += btVector3(0, 0, 1);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) dir += btVector3(-1, 0, 0);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) dir += btVector3(1, 0, 0);

    player->Move(dir, moveSpeed);

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
    {
        player->Jump();
    }
}
