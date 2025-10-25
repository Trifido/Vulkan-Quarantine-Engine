#include <QECharacterController.h>
#include <GUIWindow.h>
#include <PhysicsModule.h>
#include <Timer.h>
#include <QEGameObject.h>
#include <CameraHelper.h>

void QECharacterController::Initialize()
{
    this->physicBodyPtr->body->setAngularFactor(0);     // No permitir que rote
    this->physicBodyPtr->body->setDamping(0.9f, 0.9f);  // Linear y angular damping
    this->physicBodyPtr->body->setFriction(1.0f);       // buena fricción
    this->physicBodyPtr->body->setRestitution(0.0f);    // sin rebote
    this->physicBodyPtr->body->setActivationState(DISABLE_DEACTIVATION);
    this->physicBodyPtr->body->setCollisionFlags(btCollisionObject::CF_DYNAMIC_OBJECT);
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

QECharacterController::QECharacterController()
{
    isGrounded = false;
    canWalkOnGround = false;
    airControlFactor = 0.8f;
    jumpForce = 8.0f;
    maxStepAngle = 35.0f;
    groundNormal = btVector3(0, 1, 0);
}

btVector3 QECharacterController::GetPosition()
{
    btTransform trans;
    this->physicBodyPtr->body->getMotionState()->getWorldTransform(trans);
    return trans.getOrigin();
}

void QECharacterController::KeyInputTrigger(int key, int action, bool& lastAction, string animationState)
{
    bool isPressed = glfwGetKey(window, key) == action;
    if (isPressed && !lastAction)
    {
        this->animationComponentPtr->SetTrigger(animationState);
    }
    lastAction = isPressed;
}

void QECharacterController::ProcessInput()
{
    if (window == nullptr) return;

    if (!ImGui::GetIO().WantCaptureKeyboard)
    {
        this->KeyInputTrigger(GLFW_KEY_F, GLFW_PRESS, this->isPressedAttackButton, "attack");
        this->KeyInputTrigger(GLFW_KEY_SPACE, GLFW_PRESS, this->isPressedJumpButton, "jump");
    }

    if (ImGui::GetIO().WantCaptureKeyboard) return;

    // 0) Lee WASD como intención en 2D (x = lateral, y = adelante)
    float ix = 0.0f, iz = 0.0f;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) ix -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) ix += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) iz += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) iz -= 1.0f;

    // Sin intención -> frena lateral (conserva Y de la física)
    if (ix == 0.0f && iz == 0.0f)
    {
        btVector3 currentVel = this->physicBodyPtr->body->getLinearVelocity();
        this->physicBodyPtr->body->setLinearVelocity(btVector3(0, currentVel.y(), 0));
        return;
    }

    // 1) Toma la rotación de cámara (desde el spring arm)
    glm::quat camRot = glm::quat(1, 0, 0, 0);
    if (springArmPtr) {
        camRot = springArmPtr->GetCameraWorldRotation();
    }
    else {
        // fallback: si no hay springArm, usa rot propia o del GO
        auto tr = Owner->GetComponent<QETransform>();
        camRot = tr ? tr->GetWorldRotation() : glm::quat(1, 0, 0, 0);
    }

    // 2) Basis de cámara y dirección en mundo desde input
    glm::vec3 fwd = CamForward(camRot);
    glm::vec3 right = CamRight(camRot);

    // Ignora el componente vertical de cámara; mueve en plano
    fwd.y = 0.0f; right.y = 0.0f;
    if (glm::length2(fwd) < 1e-6f) fwd = glm::vec3(0, 0, -1);
    if (glm::length2(right) < 1e-6f) right = glm::vec3(1, 0, 0);
    fwd = glm::normalize(fwd);
    right = glm::normalize(right);

    glm::vec3 wishDir = glm::normalize(right * ix + fwd * iz);

    // 3) Proyecta sobre el plano del suelo si estás en pendiente
    glm::vec3 groundN(0, 1, 0);
    if (isGrounded) {
        groundN = glm::vec3(groundNormal.x(), groundNormal.y(), groundNormal.z());
        if (glm::length2(groundN) > 1e-6f) groundN = glm::normalize(groundN);
        else groundN = glm::vec3(0, 1, 0);
        wishDir = ProjectOnPlane(wishDir, groundN);
        if (glm::length2(wishDir) > 1e-6f) wishDir = glm::normalize(wishDir);
    }

    // 4) Mueve usando tu pipeline actual
    const float moveSpeed = 1.0f;
    btVector3 dirBT(wishDir.x, wishDir.y, wishDir.z);
    Move(dirBT, moveSpeed);

    if (visualTr && glm::length2(wishDir) > 1e-6f)
    {
        float targetYaw = std::atan2(wishDir.x, -wishDir.z);
        glm::quat targetRot = glm::angleAxis(targetYaw, glm::vec3(0, 1, 0));
        glm::quat newQ = glm::slerp(visualTr->localRotation, targetRot, 1.0f - std::exp(-10.0f * Timer::DeltaTime));
        visualTr->SetLocalRotation(newQ);
    }
}

void QECharacterController::QEStart()
{
    QEGameComponent::QEStart();
    this->AddGLFWWindow(GUIWindow::getInstance()->window);
}

void QECharacterController::QEInit()
{
    auto physicsBody = this->Owner->GetComponent<PhysicsBody>();
    auto collider = this->Owner->GetComponent<QECollider>();
    auto animationComponent = this->Owner->GetComponentInChildren<AnimationComponent>(true);
    this->springArmPtr = this->Owner->GetComponentInChildren<QESpringArmComponent>(false);
    this->visualTr = Owner->GetComponentInChildren<QETransform>(false);

    if (animationComponent != NULL)
    {
        this->animationComponentPtr = animationComponent;
    }

    if (collider != NULL && physicsBody != NULL)
    {
        this->physicBodyPtr = physicsBody;
        this->colliderPtr = collider;
        this->physicBodyID = physicBodyPtr->id;
        this->colliderID = collider->id;

        this->Initialize();

        QEGameComponent::QEInit();
    }
}

void QECharacterController::QEUpdate()
{
    CheckIfGrounded();
    ProcessInput();
}

void QECharacterController::QEDestroy()
{
    QEGameComponent::QEDestroy();
}
