#include <QECharacterController.h>
#include <PhysicsModule.h>
#include <Timer.h>

void QECharacterController::Initialize()
{
    this->physicBodyPtr->body->setAngularFactor(0);     // No permitir que rote
    this->physicBodyPtr->body->setDamping(0.9f, 0.9f);  // Linear y angular damping
    this->physicBodyPtr->body->setFriction(1.0f);       // buena fricci�n
    this->physicBodyPtr->body->setRestitution(0.0f);    // sin rebote
    this->physicBodyPtr->body->setActivationState(DISABLE_DEACTIVATION);
    this->physicBodyPtr->body->setCollisionFlags(btCollisionObject::CF_DYNAMIC_OBJECT);
}

void QECharacterController::BindGameObjectProperties(std::shared_ptr<PhysicBody> physicBody, std::shared_ptr<Collider> colliderPtr)
{
    this->physicBodyPtr = physicBody;
    this->colliderPtr = colliderPtr;
}

void QECharacterController::CheckIfGrounded()
{
    float margin = this->colliderPtr->CollisionMargin;

    btTransform trans;
    this->physicBodyPtr->body->getMotionState()->getWorldTransform(trans);

    btVector3 min, max;
    this->colliderPtr->colShape->getAabb(
        this->physicBodyPtr->body->getWorldTransform(),
        min,
        max);

    this->groundCheckRays[0] = btVector3(min.x() + margin, min.y() + margin, min.z() + margin);
    this->groundCheckRays[1] = btVector3(max.x() - margin, min.y() + margin, min.z() + margin);
    this->groundCheckRays[2] = btVector3(min.x() + margin, min.y() + margin, max.z() - margin);
    this->groundCheckRays[3] = btVector3(max.x() - margin, min.y() + margin, max.z() - margin);

    this->isGrounded = false;
    this->canWalkOnGround = false;

    // Check raycast from the character's position to the ground
    for (int i = 0; i < 4; i++)
    {
        btVector3 rayEnd = this->groundCheckRays[i] - btVector3(0.0f, margin * 2.0f, 0.0f);

        btCollisionWorld::ClosestRayResultCallback rayCallback(this->groundCheckRays[i], rayEnd);
        rayCallback.m_collisionFilterGroup = CollisionFlag::COL_PLAYER;
        rayCallback.m_collisionFilterMask = CollisionFlag::COL_SCENE;
        PhysicsModule::getInstance()->dynamicsWorld->rayTest(this->groundCheckRays[i], rayEnd, rayCallback);

        if (rayCallback.hasHit())
        {
            this->isGrounded = true;
            this->groundNormal = rayCallback.m_hitNormalWorld.normalized();

            // Calculamos si es una superficie caminable
            float maxSlopeRadians = glm::radians(35.0f);
            this->canWalkOnGround = this->groundNormal.dot(btVector3(0, 1, 0)) > cos(maxSlopeRadians);

            printf("Tierra!\n");

            break;
        }
    }

    if (this->isGrounded == false)
    {
        printf("Aire!\n");
    }
}

void QECharacterController::Move(const btVector3 & direction, float speed)
{
    btVector3 currentVel = this->physicBodyPtr->body->getLinearVelocity();
    btVector3 adjustedDir;
    float airControl = this->isGrounded ? 1.0f : this->airControlFactor;
    float stepDistance = speed * Timer::DeltaTime;

    if (direction.length2() > 0.001f)
    {
        if (this->CanMove(direction, stepDistance, adjustedDir) && (this->isGrounded == false || this->canWalkOnGround))
        {
            btVector3 vel = adjustedDir.normalized() * speed;

            // Ajuste de direcci�n si est� en suelo
            if (this->isGrounded && this->canWalkOnGround)
            {
                btVector3 right = adjustedDir.cross(this->groundNormal);

                if (right.length2() > 0.0001f)
                {
                    right = right.normalized();
                    adjustedDir = this->groundNormal.cross(right).normalized();
                }

                vel = adjustedDir * speed;
            }

            btVector3 desiredVelocity = btVector3(vel.x() * airControl, currentVel.y(), vel.z() * airControl);

            printf("Desired Velocity: %f %f %f\n", desiredVelocity.x(), desiredVelocity.y(), desiredVelocity.z());
            this->physicBodyPtr->body->setLinearVelocity(desiredVelocity);
        }
        else
        {
            printf("No me muevo\n");
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

bool QECharacterController::CanMove(const btVector3& direction, float distance, btVector3& outAdjustedDir)
{
    outAdjustedDir = btVector3(0, 0, 0);
    btTransform from = this->physicBodyPtr->body->getWorldTransform();
    btVector3 start = from.getOrigin();
    btVector3 desiredEnd = start + direction.normalized() * distance;

    btConvexShape* shape = static_cast<btConvexShape*>(this->physicBodyPtr->body->getCollisionShape());

    IgnoreSelfCallback callback(start, desiredEnd, this->physicBodyPtr->body);
    callback.m_collisionFilterGroup = this->physicBodyPtr->CollisionGroup;
    callback.m_collisionFilterMask = this->physicBodyPtr->CollisionMask;

    PhysicsModule::getInstance()->dynamicsWorld->convexSweepTest(shape, from, btTransform(btQuaternion::getIdentity(), desiredEnd), callback);

    if (!callback.hasHit()) {
        outAdjustedDir = direction.normalized();
        return true;
    }

    // Intentar deslizarse
    btVector3 normal = callback.hitNormal.normalized();
    btVector3 slideDir = direction - normal * direction.dot(normal);

    if (slideDir.fuzzyZero())
    {
        return false;
    }

    // Segundo intento con direcci�n deslizada
    btVector3 slideEnd = start + slideDir.normalized() * distance;

    IgnoreSelfCallback slideCallback(start, slideEnd, this->physicBodyPtr->body);
    slideCallback.m_collisionFilterGroup = this->physicBodyPtr->CollisionGroup;
    slideCallback.m_collisionFilterMask = this->physicBodyPtr->CollisionMask;

    PhysicsModule::getInstance()->dynamicsWorld->convexSweepTest(shape, from, btTransform(btQuaternion::getIdentity(), slideEnd), slideCallback);

    if (!slideCallback.hasHit()) {
        outAdjustedDir = slideDir.normalized();
        return true;
    }

    return false;
}


// Actualizar la f�sica (para simular la f�sica del personaje)
void QECharacterController::Update()
{
    this->CheckIfGrounded();
}

// Obtener la posici�n del personaje
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
