#include "QESpringArmComponent.h"
#include <Timer.h>
#include <QEGameObject.h>
#include <QECamera.h>
#include <QETransform.h>
#include <PhysicsModule.h>
#include <glm/common.hpp>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhase.h>
#include <Jolt/Physics/Collision/CollideShape.h>
#include <Jolt/Physics/Collision/ShapeCast.h>

namespace {
    // Colisiona sólo con la escena (ajusta a tus necesidades)
    struct SceneOnlyLayerFilter final : JPH::ObjectLayerFilter {
        bool ShouldCollide(JPH::ObjectLayer l) const override {
            return l == Layers::SCENE_STATIC || l == Layers::SCENE_MOVING;
        }
    };

    // Ignora al body del jugador/cámara si existe
    struct IgnoreBodyFilter final : JPH::BodyFilter {
        JPH::BodyID ignore;
        explicit IgnoreBodyFilter(JPH::BodyID id) : ignore(id) {}
        bool ShouldCollide(const JPH::BodyID& other) const override { return other != ignore; }
    };
}

QESpringArmComponent::QESpringArmComponent()
{
    this->TargetArmLength = 3.0f;
    this->MinArmLength = 1.0f;
    this->MaxArmLength = 6.0f;

    this->TargetOffset = { 0.0f, 1.6f, 0.0f };
    this->SocketOffset = { 0.0f, 0.3f, 0.0f };

    this->DoCollisionTest = false;
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
    glm::vec3 basePos; glm::quat baseRot;
    GetParentInterpolatedTRS(basePos, baseRot);

    glm::quat inherit = (InheritYaw || InheritPitch || InheritRoll) ? baseRot
        : glm::quat(1, 0, 0, 0);
    return inherit * YawPitchToQuat(Yaw, Pitch);
}

glm::vec3 QESpringArmComponent::ComputePivotWorldPos() const
{
    glm::vec3 basePos; glm::quat baseRot;
    GetParentInterpolatedTRS(basePos, baseRot);
    return basePos + (glm::mat3_cast(baseRot) * TargetOffset);
}

bool QESpringArmComponent::SphereSweep(
    const glm::vec3& start,
    const glm::vec3& end,
    float radius,
    SweepHit& outHit) const
{
    return false;
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
    glm::vec3 pivot = desiredPivot;
    glm::quat rotBase = desiredRot;
    m_currentArmLen = SmoothExp(m_currentArmLen, TargetArmLength, PosLagSpeed, dt);
    m_smoothedPivot = SmoothExpVec3(m_smoothedPivot, desiredPivot, PosLagSpeed, dt);

    // 3) Punto final deseado del brazo
    glm::vec3 fwd = rotBase * glm::vec3(0, 0, -1);
    glm::vec3 socket = m_smoothedPivot + SocketOffset;
    glm::vec3 desiredCamPos = socket - fwd * std::clamp(m_currentArmLen, MinArmLength, MaxArmLength);

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

    float kPos = 1.0f - std::exp(-PosLagSpeed * dt);
    float kRot = 1.0f - std::exp(-RotLagSpeed * dt);
    m_camPos = glm::mix(m_camPos, desiredCamPos, kPos);
    m_camRot = glm::slerp(m_camRot, rotBase, kRot);

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

void QESpringArmComponent::GetParentInterpolatedTRS(glm::vec3& pos, glm::quat& rot) const
{
    if (auto tr = Owner->GetComponent<QETransform>()) {
        if (auto parent = tr->GetParent()) {
            pos = parent->GetWorldPosition();
            rot = parent->GetWorldRotation();
            return;
        }
        pos = tr->GetWorldPosition();
        rot = tr->GetWorldRotation();
    }
    else {
        pos = glm::vec3(0);
        rot = glm::quat(1, 0, 0, 0);
    }
}
