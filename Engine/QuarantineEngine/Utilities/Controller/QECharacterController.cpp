#include <QECharacterController.h>
#include <PhysicsModule.h>
#include <Timer.h>

void QECharacterController::Initialize()
{
    this->physicBodyPtr->body->setAngularFactor(0);     // No permitir que rote
    this->physicBodyPtr->body->setDamping(0.9f, 0.9f);  // Linear y angular damping
    this->physicBodyPtr->body->setFriction(1.0f);       // buena fricción
    this->physicBodyPtr->body->setRestitution(0.0f);    // sin rebote
    this->physicBodyPtr->body->setActivationState(DISABLE_DEACTIVATION);
    this->physicBodyPtr->body->setCollisionFlags(btCollisionObject::CF_DYNAMIC_OBJECT);
}

void QECharacterController::BindGameObjectProperties(std::shared_ptr<PhysicBody> physicBodyPtr)
{
    this->physicBodyPtr = physicBodyPtr;
}

void QECharacterController::CheckIfGrounded()
{
    btTransform trans;
    this->physicBodyPtr->body->getMotionState()->getWorldTransform(trans);
    btVector3 start = trans.getOrigin();
    btVector3 end = start - btVector3(0, 0.51f, 0);

    btCollisionWorld::ClosestRayResultCallback rayCallback(start, end);
    rayCallback.m_collisionFilterGroup = CollisionFlag::COL_PLAYER;
    rayCallback.m_collisionFilterMask = CollisionFlag::COL_SCENE;
    PhysicsModule::getInstance()->dynamicsWorld->rayTest(start, end, rayCallback);

    if (rayCallback.hasHit())
    {
        this->isGrounded = true;
        this->groundNormal = rayCallback.m_hitNormalWorld.normalized();

        // Calculamos si es una superficie caminable
        float maxSlopeRadians = glm::radians(45.0f); // puedes exponerlo como parámetro
        this->canWalkOnGround = this->groundNormal.dot(btVector3(0, 1, 0)) > cos(maxSlopeRadians);
    }
    else
    {
        this->isGrounded = false;
        this->canWalkOnGround = false;
    }

    isGrounded = rayCallback.hasHit();
}

void QECharacterController::Move(const btVector3 & direction, float speed)
{
    btVector3 currentVel = this->physicBodyPtr->body->getLinearVelocity();
    float airControl = this->isGrounded ? 1.0f : this->airControlFactor;
    float stepDistance = speed * Timer::DeltaTime;

    if (direction.length2() > 0.001f)
    {
        if (this->CanMove(direction, stepDistance) && (this->isGrounded == false || this->canWalkOnGround))
        {
            btVector3 vel = direction.normalized() * speed;

            // Ajuste de dirección si está en suelo
            if (this->isGrounded && this->canWalkOnGround)
            {
                btVector3 moveDir = direction.normalized();
                btVector3 right = moveDir.cross(this->groundNormal).normalized();
                btVector3 adjustedDir = this->groundNormal.cross(right).normalized();
                vel = adjustedDir * speed;
            }

            btVector3 desiredVelocity = btVector3(vel.x() * airControl, currentVel.y(), vel.z() * airControl);

            this->physicBodyPtr->body->setLinearVelocity(desiredVelocity);
        }
        else
        {
            this->physicBodyPtr->body->setLinearVelocity(btVector3(0, currentVel.y(), 0));
        }
    }
    else
    {
        this->physicBodyPtr->body->setLinearVelocity(btVector3(0, currentVel.y(), 0));
    }
}

void QECharacterController::Jump()
{
    if (this->isGrounded)
    {
        btVector3 velocity = this->physicBodyPtr->body->getLinearVelocity();
        this->physicBodyPtr->body->setLinearVelocity(btVector3(velocity.x(), this->jumpForce, velocity.z()));
    }
}

bool QECharacterController::CanMove(const btVector3& direction, float distance)
{
    btTransform from = this->physicBodyPtr->body->getWorldTransform();
    btTransform to = from;
    to.setOrigin(from.getOrigin() + direction.normalized() * distance);

    btConvexShape* shape = static_cast<btConvexShape*>(this->physicBodyPtr->body->getCollisionShape());

    IgnoreSelfCallback callback(
        from.getOrigin(),
        to.getOrigin(),
        this->physicBodyPtr->body);

    // Ajusta los grupos según tu sistema
    callback.m_collisionFilterGroup = this->physicBodyPtr->CollisionGroup;
    callback.m_collisionFilterMask = this->physicBodyPtr->CollisionMask;

    // Ignorar el propio cuerpo
    callback.m_hitCollisionObject = this->physicBodyPtr->body;

    PhysicsModule::getInstance()->dynamicsWorld->convexSweepTest(shape, from, to, callback);

    return !callback.hasHit();
}


// Actualizar la física (para simular la física del personaje)
void QECharacterController::Update()
{
    this->CheckIfGrounded();
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

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) != GLFW_PRESS)
    {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) dir += btVector3(0, 0, -1);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) dir += btVector3(0, 0, 1);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) dir += btVector3(-1, 0, 0);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) dir += btVector3(1, 0, 0);
    }

    player->Move(dir, moveSpeed);

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
    {
        player->Jump();
    }
}
