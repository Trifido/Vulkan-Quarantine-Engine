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

void QECharacterController::BindGameObjectProperties(std::shared_ptr<PhysicsBody> physicBody, std::shared_ptr<Collider> collider)
{
    this->physicBodyPtr = physicBody;
    this->colliderPtr = collider;
}

void QECharacterController::CheckIfGrounded()
{
    this->isGrounded = false;
    this->canWalkOnGround = false;

    btTransform trans;
    this->physicBodyPtr->body->getMotionState()->getWorldTransform(trans);

    btVector3 min, max;
    this->colliderPtr->colShape->getAabb(
        this->physicBodyPtr->body->getWorldTransform(),
        min,
        max);

    // Check raycast from the character's position to the ground
    btVector3 groundCheckRays = (min + max) * 0.5f;
    float rayEndY = max.y() - groundCheckRays.y() + this->colliderPtr->CollisionMargin;
    btVector3 rayEnd = groundCheckRays - btVector3(0.0f, rayEndY, 0.0f);

    btCollisionWorld::ClosestRayResultCallback rayCallback(groundCheckRays, rayEnd);
    rayCallback.m_collisionFilterGroup = CollisionFlag::COL_PLAYER;
    rayCallback.m_collisionFilterMask = CollisionFlag::COL_SCENE;
    PhysicsModule::getInstance()->dynamicsWorld->rayTest(groundCheckRays, rayEnd, rayCallback);

    if (rayCallback.hasHit())
    {
        this->isGrounded = true;
        this->groundNormal = rayCallback.m_hitNormalWorld.normalized();

        // Calculamos si es una superficie caminable
        float maxSlopeRadians = glm::radians<float>(maxStepAngle);
        this->canWalkOnGround = this->groundNormal.dot(btVector3(0, 1, 0)) > cos(maxSlopeRadians);
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

            // Ajuste de dirección si está en suelo
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

    // Segundo intento con dirección deslizada
    btVector3 slideEnd = start + slideDir.normalized() * distance;

    IgnoreSelfCallback slideCallback(start, slideEnd, this->physicBodyPtr->body);
    slideCallback.m_collisionFilterGroup = this->physicBodyPtr->CollisionGroup;
    slideCallback.m_collisionFilterMask = this->physicBodyPtr->CollisionMask;

    PhysicsModule::getInstance()->dynamicsWorld->convexSweepTest(shape, from, btTransform(btQuaternion::getIdentity(), slideEnd), slideCallback);

    if (!slideCallback.hasHit())
    {
        outAdjustedDir = slideDir.normalized();
        return true;
    }

    return false;
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

void QECharacterController::ProcessInput()
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

    Move(dir, moveSpeed);

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
    {
        Jump();
    }
}

void QECharacterController::QEStart()
{
}

void QECharacterController::QEUpdate()
{
    CheckIfGrounded();
    ProcessInput();
}

void QECharacterController::QERelease()
{
}
