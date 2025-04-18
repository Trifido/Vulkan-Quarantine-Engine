#include <QECharacterController.h>

void QECharacterController::BindGameObjectProperties(std::shared_ptr<PhysicBody> physicBodyPtr)
{
    this->physicBodyPtr = physicBodyPtr;
}

// Mover el personaje hacia adelante
void QECharacterController::MoveForward()
{
    this->physicBodyPtr->body->applyCentralForce(btVector3(0, 0, -10.0f));  // Aplicar fuerza en Z negativa
}

// Mover el personaje hacia atrás
void QECharacterController::MoveBackward()
{
    this->physicBodyPtr->body->applyCentralForce(btVector3(0, 0, 10.0f));  // Aplicar fuerza en Z positiva
}

// Mover el personaje a la izquierda
void QECharacterController::MoveLeft()
{
    this->physicBodyPtr->body->applyCentralForce(btVector3(-10.0f, 0, 0));  // Aplicar fuerza en X negativa
}

// Mover el personaje a la derecha
void QECharacterController::MoveRight()
{
    this->physicBodyPtr->body->applyCentralForce(btVector3(10.0f, 0, 0));  // Aplicar fuerza en X positiva
}

// Saltar (aplicar fuerza hacia arriba)
void QECharacterController::Jump()
{
    this->physicBodyPtr->body->applyCentralForce(btVector3(0, 20.0f, 0)); // Fuerza hacia arriba en el eje Y
}

// Actualizar la física (para simular la física del personaje)
void QECharacterController::Update(btDiscreteDynamicsWorld* world)
{
    world->stepSimulation(1.f / 60.f, 10);  // Avanzar la simulación de Bullet
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
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
        player->MoveForward();
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
        player->MoveBackward();
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    {
        player->MoveLeft();
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    {
        player->MoveRight();
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
    {
        player->Jump();
    }
}
