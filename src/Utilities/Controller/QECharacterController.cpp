#include <QECharacterController.h>
#include <GUIWindow.h>
#include <PhysicsModule.h>
#include <Timer.h>
#include <QEGameObject.h>
#include <CameraHelper.h>
#include <CapsuleCollider.h>

QECharacterController::QECharacterController()
{
    _moveSpeed = 5.0f;   
    _sprintSpeed = 8.0f;
    _gravity = -9.81f;   
    _jumpSpeed = 6.5f;
    _maxSlopeCos = std::cos(glm::radians(50.0f));
    _vY = 0.0f;
    _grounded = false;
    _capsuleRadius = 0.35f;
    _capsuleHalfHeight = 0.90f;
}

btTransform QECharacterController::GlmToBt(const glm::vec3& p, const glm::quat& q) const
{
    btTransform t;
    t.setIdentity();
    t.setOrigin(btVector3(p.x, p.y, p.z));
    t.setRotation(btQuaternion(q.x, q.y, q.z, q.w));
    return t;
}

void QECharacterController::EnsureKinematicFlags()
{
    if (!physicBodyPtr || !physicBodyPtr->body) return;

    auto* rb = physicBodyPtr->body;

    int flags = rb->getCollisionFlags();
    if ((flags & btCollisionObject::CF_KINEMATIC_OBJECT) == 0)
    {
        rb->setCollisionFlags(flags | btCollisionObject::CF_KINEMATIC_OBJECT);
        rb->setActivationState(DISABLE_DEACTIVATION);
        rb->setSleepingThresholds(0.f, 0.f);
        rb->setMassProps(0.f, btVector3(0, 0, 0));
    }
}

void QECharacterController::SyncTransformToBullet() const
{
    if (!physicBodyPtr || !physicBodyPtr->body) return;

    glm::mat4 wm = transform->GetWorldMatrix();
    glm::vec3 p = glm::vec3(wm[3]);
    glm::quat q = transform->GetWorldRotation();
    btTransform t = GlmToBt(p, q);

    if (auto* ms = physicBodyPtr->body->getMotionState()) ms->setWorldTransform(t);
    physicBodyPtr->body->setWorldTransform(t);
}

void QECharacterController::QEStart()
{
    QEGameComponent::QEStart();
}

void QECharacterController::QEInit()
{
    if (QEInitialized()) return;

    transform = Owner->GetComponent<QETransform>();
    colliderPtr = Owner->GetComponentInChildren<QECollider>(true);
    physicBodyPtr = Owner->GetComponent<PhysicsBody>();

    if (auto caps = std::dynamic_pointer_cast<CapsuleCollider>(colliderPtr))
    {
        _capsuleRadius = caps->GetRadius();
        _capsuleHalfHeight = caps->GetHeight() * 0.5f;
    }

    EnsureKinematicFlags();

    QEGameComponent::QEInit();
}

// ----------------- Input horizontal (WASD) -----------------
glm::vec3 QECharacterController::ReadInputHorizontal(float dt) const
{
    glm::vec3 dir(0);
    if (ImGui::IsKeyDown(ImGuiKey_W)) dir += glm::vec3(0, 0, -1);
    if (ImGui::IsKeyDown(ImGuiKey_S)) dir += glm::vec3(0, 0, 1);
    if (ImGui::IsKeyDown(ImGuiKey_A)) dir += glm::vec3(-1, 0, 0);
    if (ImGui::IsKeyDown(ImGuiKey_D)) dir += glm::vec3(1, 0, 0);
    if (glm::length2(dir) > 0) dir = glm::normalize(dir);

    float spd = ImGui::IsKeyDown(ImGuiKey_ModShift) ? _sprintSpeed : _moveSpeed;
    return dir * spd * dt;
}

bool QECharacterController::CheckGrounded(bool* outOnSlope)
{
    const float snap = 0.2f;
    glm::vec3 pos = transform->GetWorldPosition();
    glm::vec3 down = pos - glm::vec3(0, snap, 0);

    glm::vec3 p, n; float f;
    bool hit = CapsuleSweep(pos, down, p, n, f);

    if (outOnSlope && hit) {
        n = glm::normalize(n);
        *outOnSlope = (n.y >= _maxSlopeCos);
    }
    return _grounded = hit;
}

bool QECharacterController::CapsuleSweep(
    const glm::vec3& start,
    const glm::vec3& end,
    glm::vec3& hitPoint,
    glm::vec3& hitNormal,
    float& hitFrac) const
{
    auto* world = PhysicsModule::getInstance()->dynamicsWorld;
    if (!world || !colliderPtr || !colliderPtr->colShape)
        return false;

    // 1) Shape del collider (tu cápsula real)
    btConvexShape* convex = static_cast<btConvexShape*>(colliderPtr->colShape);
    if (!convex)
        return false; // solo convexos sirven para sweeps

    // 2) Prepara el sweep
    btTransform from, to;
    from.setIdentity();
    from.setOrigin(btVector3(start.x, start.y, start.z));
    to.setIdentity();
    to.setOrigin(btVector3(end.x, end.y, end.z));

    const btCollisionObject* selfObj = (physicBodyPtr && physicBodyPtr->body)
        ? static_cast<const btCollisionObject*>(physicBodyPtr->body)
        : nullptr;

    IgnoreSelfConvexResult cb(from.getOrigin(), to.getOrigin(), selfObj);

    cb.m_collisionFilterGroup = physicBodyPtr->CollisionGroup;
    cb.m_collisionFilterMask = physicBodyPtr->CollisionMask;

    // 3) Ejecutar el sweep
    world->convexSweepTest(convex, from, to, cb);

    if (!cb.hasHit())
        return false;

    // 4) Resultado
    hitFrac = static_cast<float>(cb.m_closestHitFraction);
    const btVector3& p = cb.m_hitPointWorld;
    btVector3 n = cb.m_hitNormalWorld;
    n.normalize();

    hitPoint = { p.x(), p.y(), p.z() };
    hitNormal = { n.x(), n.y(), n.z() };
    return true;
}

// ----------------- Movimiento kinematic -----------------
void QECharacterController::ApplyKinematicMove(const glm::vec3& desiredDelta)
{
    glm::vec3 start = transform->GetWorldPosition();
    glm::vec3 delta = desiredDelta;
    const float backoff = 0.001f;

    for (int it = 0; it < 2; ++it) { // 2 iteraciones suele bastar
        glm::vec3 end = start + delta;
        glm::vec3 hitP, hitN; float frac;

        if (CapsuleSweep(start, end, hitP, hitN, frac)) {
            float useFrac = glm::max(0.0f, frac - backoff);
            glm::vec3 reached = start + delta * useFrac;

            glm::vec3 n = glm::normalize(hitN);
            glm::vec3 remaining = delta * (1.0f - useFrac);
            glm::vec3 slide = remaining - n * glm::dot(remaining, n);

            transform->TranslateWorld(reached - start);
            start = transform->GetWorldPosition();
            delta = slide;
        }
        else {
            transform->TranslateWorld(delta);
            break;
        }
    }
    SyncTransformToBullet();
}

void QECharacterController::QEUpdate()
{
    float dt = Timer::DeltaTime;
    if (!transform || dt <= 0.0f) return;

    // 1) Horizontal
    glm::vec3 horiz = ReadInputHorizontal(dt);

    // 2) Ground & salto
    bool onSlope = false;
    bool wasGrounded = _grounded;
    _grounded = CheckGrounded(&onSlope);

    if (_grounded)
    {
        if (ImGui::IsKeyPressed(ImGuiKey_Space))
        {
            _vY = _jumpSpeed;       // impulso
            _grounded = false;
        }
        else {
            _vY = std::min(_vY, 0.0f); // pegado al suelo
        }
    }
    else {
        _vY += _gravity * dt;       // gravedad manual
    }

    // 3) Delta total
    glm::vec3 delta = horiz;
    delta.y += _vY * dt;

    // 4) Aplicar con sweep (bloquea paredes y hace slide)
    ApplyKinematicMove(delta);

    if (_grounded && _vY <= 0.0f) {
        glm::vec3 pos = transform->GetWorldPosition();
        glm::vec3 p, n; float f;
        if (CapsuleSweep(pos, pos - glm::vec3(0, 0.2f, 0), p, n, f)) {
            transform->TranslateWorld(glm::vec3(0, -(0.2f * f), 0)); // encaja suave
            _vY = 0.0f;
            SyncTransformToBullet();
        }
    }
}

void QECharacterController::QEDestroy()
{
    QEGameComponent::QEDestroy();
}
