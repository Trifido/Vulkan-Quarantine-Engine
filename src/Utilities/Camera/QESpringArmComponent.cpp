#include "QESpringArmComponent.h"
#include <Timer.h>
#include <QEGameObject.h>
#include <QECamera.h>
#include <QETransform.h>
#include <PhysicsModule.h>
#include <BulletGLM.h>

struct ClosestNotMeConvexResultCallback : public btCollisionWorld::ClosestConvexResultCallback {
    std::unordered_set<const btCollisionObject*> ignored;

    ClosestNotMeConvexResultCallback(const btVector3& from, const btVector3& to)
        : btCollisionWorld::ClosestConvexResultCallback(from, to) {}

    btScalar addSingleResult(btCollisionWorld::LocalConvexResult& convexResult, bool normalInWorldSpace) override {
        if (convexResult.m_hitCollisionObject &&
            ignored.count(convexResult.m_hitCollisionObject) > 0) {
            return 1.0f;
        }
        return btCollisionWorld::ClosestConvexResultCallback::addSingleResult(convexResult, normalInWorldSpace);
    }
};

QESpringArmComponent::QESpringArmComponent()
{
    this->TargetArmLength = 3.0f;
    this->MinArmLength = 1.0f;
    this->MaxArmLength = 6.0f;

    this->TargetOffset = { 0.0f, 1.6f, 0.0f };
    this->SocketOffset = { 0.0f, 0.3f, 0.0f };

    this->DoCollisionTest = true;
    this->ProbeRadius = 0.2f;

    this->PosLagSpeed = 12.0f;
    this->RotLagSpeed = 12.0f;

    this->Yaw = 0.0f;
    this->Pitch = -10.0f;
    this->MinPitch = -80.0f;
    this->MaxPitch = +60.0f;

    this->InheritYaw = false;
    this->InheritPitch = false;
    this->InheritRoll = false;

    this->m_currentArmLen = this->TargetArmLength;
}

float QESpringArmComponent::SmoothExp(float c, float t, float s, float dt)
{
    float a = 1.0f - std::exp(-s * dt);
    return c + (t - c) * a;
}
glm::vec3 QESpringArmComponent::SmoothExpVec3(glm::vec3 c, glm::vec3 t, float s, float dt)
{
    float a = 1.0f - std::exp(-s * dt);
    return c + (t - c) * a;
}
glm::quat QESpringArmComponent::SmoothExpQuat(glm::quat c, glm::quat t, float s, float dt)
{
    float a = 1.0f - std::exp(-s * dt);
    return glm::slerp(c, t, a);
}

static glm::quat YawPitchToQuat(float yawDeg, float pitchDeg)
{
    float yawRad = glm::radians(yawDeg);
    float pitchRad = glm::radians(pitchDeg);
    glm::quat qYaw = glm::angleAxis(yawRad, glm::vec3(0, 1, 0));
    glm::quat qPitch = glm::angleAxis(pitchRad, glm::vec3(1, 0, 0));
    return qYaw * qPitch;
}

glm::quat QESpringArmComponent::ComputeDesiredRotation() const
{
    glm::quat base = glm::quat(1, 0, 0, 0);
    if (InheritYaw || InheritPitch || InheritRoll)
    {
        auto tr = Owner->GetComponent<QETransform>();
        auto parent = tr ? tr->GetParent() : nullptr;
        base = parent ? parent->GetWorldRotation() : (tr ? tr->GetWorldRotation() : glm::quat(1, 0, 0, 0));
    }
    glm::quat orbit = YawPitchToQuat(Yaw, Pitch);
    return base * orbit;
}

glm::vec3 QESpringArmComponent::ComputePivotWorldPos() const
{
    auto tr = Owner->GetComponent<QETransform>();
    auto parent = tr ? tr->GetParent() : nullptr;
    glm::vec3 basePos = parent ? parent->GetWorldPosition() : (tr ? tr->GetWorldPosition() : glm::vec3(0));
    glm::quat baseRot = parent ? parent->GetWorldRotation() : (tr ? tr->GetWorldRotation() : glm::quat(1, 0, 0, 0));

    glm::vec3 worldOffset = glm::mat3_cast(baseRot) * TargetOffset;
    return basePos + worldOffset;
}

bool QESpringArmComponent::SphereSweep(const glm::vec3& start,
    const glm::vec3& end,
    float radius,
    SweepHit& outHit) const
{
    auto physicsSystem = PhysicsModule::getInstance();
    if (!physicsSystem->dynamicsWorld)
    {
        outHit = {};
        return false;
    }

    // 1) Shape y márgenes
    // Importante: pon margen a 0.0f para que el radio sea EXACTO.
    static thread_local btSphereShape sphere(1.0f);
    sphere.setUnscaledRadius(btScalar(radius));
    sphere.setMargin(0.0f);

    // 2) Transforms de inicio/fin (rot identidad: esfera no depende de rot)
    btTransform fromXf; fromXf.setIdentity(); fromXf.setOrigin(ToBt(start));
    btTransform toXf;   toXf.setIdentity();   toXf.setOrigin(ToBt(end));

    // 3) Callback de resultado con filtros
    ClosestNotMeConvexResultCallback cb(ToBt(start), ToBt(end));

    // Configura grupos/máscaras si usas flags de colisión (opcional)
    // cb.m_collisionFilterGroup = (short)CollisionFlag::CameraProbe;
    // cb.m_collisionFilterMask  = (short)CollisionFlag::WorldStatic | (short)CollisionFlag::WorldDynamic;

    // Ignora el rigidbody del player y el de la cámara si existen
    // (Sustituye estas llamadas por tu forma de acceder a los cuerpos)
    auto playerRB = Owner->GetComponent<PhysicsBody>();
    if (playerRB)
    {
        cb.ignored.insert(playerRB->body);
    }

    // 4) Lanza el sweep
    physicsSystem->dynamicsWorld->convexSweepTest(&sphere, fromXf, toXf, cb);

    if (!cb.hasHit()) { outHit = {}; return false; }

    // 5) Construye el resultado
    outHit.hit = true;
    outHit.distance = glm::clamp(float(cb.m_closestHitFraction) * glm::length(end - start), 0.0f, glm::length(end - start));
    // Punto de impacto en mundo
    glm::vec3 ray = (end - start);
    glm::vec3 hitPos = start + (ray * float(cb.m_closestHitFraction));
    outHit.point = hitPos;

    // Normal
    if (cb.m_hitNormalWorld.length2() > 0.0)
    {
        outHit.normal = ToGlm(cb.m_hitNormalWorld) * 1.0f;
    }
    else
    {
        outHit.normal = glm::normalize(start - end);
    }
    return true;
}

void QESpringArmComponent::AddYawInput(float dDeg)
{
    Yaw += dDeg;
}

void QESpringArmComponent::AddPitchInput(float dDeg)
{
    Pitch = std::clamp(Pitch + dDeg, MinPitch, MaxPitch);
}

void QESpringArmComponent::AddZoomInput(float dLen)
{
    TargetArmLength = std::clamp(TargetArmLength + dLen, MinArmLength, MaxArmLength);
}

void QESpringArmComponent::QEStart()
{
    if (QEStarted()) return;

    this->m_smoothedPivot = ComputePivotWorldPos();
    this->m_smoothedRot = ComputeDesiredRotation();
}

void QESpringArmComponent::QEUpdate()
{
    float dt = Timer::DeltaTime;

    ImGuiIO& io = ImGui::GetIO();
    ImVec2 d = io.MouseDelta;
    if (d.x != 0.0f || d.y != 0.0f)
    {
        AddYawInput(d.x * g_camMouse.sensX);
        AddPitchInput((g_camMouse.invertY ? 1.0f : -1.0f) * d.y * g_camMouse.sensY);
    }

    // 1) Objetivo deseado
    glm::vec3 desiredPivot = ComputePivotWorldPos();
    glm::quat desiredRot = ComputeDesiredRotation();

    // 2) Suavizados
    m_smoothedPivot = SmoothExpVec3(m_smoothedPivot, desiredPivot, PosLagSpeed, dt);
    m_smoothedRot = SmoothExpQuat(m_smoothedRot, desiredRot, RotLagSpeed, dt);
    m_currentArmLen = SmoothExp(m_currentArmLen, TargetArmLength, PosLagSpeed, dt);

    // 3) Punto final deseado del brazo
    glm::vec3 fwd = m_smoothedRot * glm::vec3(0, 0, -1); // cámara mira -Z local
    glm::vec3 desiredCamPos = m_smoothedPivot + SocketOffset - fwd * m_currentArmLen;

    // 4) Colisión del brazo (opcional)
    glm::vec3 start = m_smoothedPivot + SocketOffset;
    glm::vec3 end = desiredCamPos;
    if (DoCollisionTest)
    {
        SweepHit hit{};
        if (SphereSweep(start, end, ProbeRadius, hit) && hit.hit)
        {
            constexpr float kBackoff = 0.05f;
            float segLen = glm::length(end - start);
            glm::vec3 dir = (segLen > 1e-5f) ? (end - start) / segLen : -fwd;

            float hitLen = glm::max(hit.distance - kBackoff, MinArmLength);
            float safeLen = glm::clamp(hitLen, MinArmLength, segLen);

            desiredCamPos = start + dir * safeLen;
        }
    }

    m_camPos = desiredCamPos;
    m_camRot = m_smoothedRot;

    if (auto tr = Owner->GetComponent<QETransform>())
    {
        if (auto cam = Owner->GetComponentInChildren<QECamera>(false))
        {
            auto camTr = cam->Owner->GetComponent<QETransform>();
            if (camTr)
            {
                glm::mat4 T = glm::translate(glm::mat4(1), m_camPos);
                glm::mat4 R = glm::toMat4(m_camRot);
                glm::mat4 worldCam = T * R;

                // World del padre (SpringArmGO)
                auto armTr = Owner->GetComponent<QETransform>();
                glm::mat4 parentWorld = armTr ? armTr->GetWorldMatrix() : glm::mat4(1);

                // LOCAL = inv(parentWorld) * WORLD
                glm::mat4 localCam = glm::inverse(parentWorld) * worldCam;

                camTr->SetFromMatrix(localCam);
            }
        }
    }
}
