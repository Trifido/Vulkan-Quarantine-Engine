#include "QECharacterController.h"

#include <QEGameObject.h>
#include <QETransform.h>
#include <Collider.h>
#include <PhysicsBody.h>
#include <PhysicsModule.h>
#include <QESpringArmComponent.h>
#include <QEAnimationComponent.h>
#include <Timer.h>

#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <imgui.h>
#include <Helpers/MathHelpers.h>
#include <QESessionManager.h>

namespace
{
    static const struct AllLayersFilter final : JPH::ObjectLayerFilter {
        bool ShouldCollide(JPH::ObjectLayer) const override { return true; }
    } kAllLayersFilter;

    struct BodyFilterSelfIgnore final : JPH::BodyFilter {
        JPH::BodyID ignore;
        explicit BodyFilterSelfIgnore(JPH::BodyID id) : ignore(id) {}
        bool ShouldCollide(const JPH::BodyID& other) const override { return other != ignore; }
    };

    inline JPH::DefaultBroadPhaseLayerFilter MakeBPFilter() {
        auto* pm = PhysicsModule::getInstance();
        return JPH::DefaultBroadPhaseLayerFilter(
            pm->GetObjectVsBPLFilter(),
            Layers::PLAYER
        );
    }
}

QECharacterController::QECharacterController()
{
    MoveSpeed = 3.0f;
    SprintSpeed = 8.0f;
    JumpSpeed = 26.5f;
    GravityY = -9.81f;
    MaxSlopeDeg = 50.0f;
    TurnSpeedDeg = 540.0f;
    JumpAnimDelay = 0.55f;

    mSettings.mBackFaceMode = JPH::EBackFaceMode::IgnoreBackFaces;
    mSettings.mEnhancedInternalEdgeRemoval = true;
}

void QECharacterController::DebugDraw(JPH::DebugRenderer& renderer)
{
    if (!mCharacter)
        return;

    const JPH::Shape* shape = mCharacter->GetShape();
    if (!shape)
        return;

    JPH::RMat44 center_of_mass = mCharacter->GetCenterOfMassTransform();

    shape->Draw(
        &renderer,
        center_of_mass,
        JPH::Vec3::sReplicate(1.0f),
        JPH::Color::sGreen,
        false,
        true
    );
}

void QECharacterController::QEStart()
{
    QEGameComponent::QEStart();
}

void QECharacterController::QEInit()
{
    if (QEInitialized()) return;

    mTransform = Owner->GetComponent<QETransform>();
    mCollider = Owner->GetComponentInChildren<QECollider>(true);
    mPhysBody = Owner->GetComponent<PhysicsBody>();
    mSpringArm = Owner->GetComponentInChildren<QESpringArmComponent>(true);
    animationComponentPtr = Owner->GetComponentInChildren<QEAnimationComponent>(true);

    BuildOrUpdateCharacter();

    PhysicsModule::getInstance()->RegisterCharacter(this);

    QEGameComponent::QEInit();
}

void QECharacterController::QEDestroy()
{
    PhysicsModule::getInstance()->UnregisterCharacter(this);
    mCharacter = nullptr;
    QEGameComponent::QEDestroy();
}

void QECharacterController::BuildOrUpdateCharacter()
{
    if (!mCollider || mCollider->colShape == nullptr)
        return;

    JPH::ShapeRefC newShape = mCollider->colShape;

    if (newShape == mLastShape && mCharacter != nullptr)
        return;

    mLastShape = newShape;
    mSettings.mShape = newShape;
    mSettings.mMaxSlopeAngle = JPH::DegreesToRadians(MaxSlopeDeg);
    mSettings.mBackFaceMode = JPH::EBackFaceMode::IgnoreBackFaces;
    mSettings.mEnhancedInternalEdgeRemoval = true;

    if (mCharacter == nullptr)
    {
        const glm::vec3 p = mTransform ? mTransform->GetWorldPosition() : glm::vec3(0);
        const glm::quat q = mTransform ? mTransform->GetWorldRotation() : glm::quat(1, 0, 0, 0);

        JPH::PhysicsSystem& system = PhysicsModule::getInstance()->World();

        mCharacter = new JPH::CharacterVirtual(
            &mSettings,
            ToJPH(p),
            ToJPH(q),
            &system
        );
        return;
    }

    JPH::TempAllocatorImpl temp_alloc(4 * 1024 * 1024);
    auto bp_filter = MakeBPFilter();
    BodyFilterSelfIgnore body_filter(mPhysBody ? mPhysBody->body : JPH::BodyID());

    mCharacter->SetShape(
        mSettings.mShape.GetPtr(),
        0.05f,
        bp_filter,
        kAllLayersFilter,
        body_filter,
        JPH::ShapeFilter(),
        temp_alloc
    );
}

glm::vec3 QECharacterController::ReadMoveInput() const
{
    glm::vec2 move(0.0f);
    if (ImGui::IsKeyDown(ImGuiKey_W)) move.y += 1.0f;  // adelante
    if (ImGui::IsKeyDown(ImGuiKey_S)) move.y -= 1.0f;  // atrás
    if (ImGui::IsKeyDown(ImGuiKey_D)) move.x += 1.0f;  // derecha
    if (ImGui::IsKeyDown(ImGuiKey_A)) move.x -= 1.0f;  // izquierda

    if (move.x == 0.0f && move.y == 0.0f)
        return glm::vec3(0.0f);

    glm::vec3 fwd(0, 0, -1);
    glm::vec3 right(1, 0, 0);

    if (mSpringArm)
    {
        glm::quat camRot = mSpringArm->GetCameraWorldRotation();

        fwd = camRot * glm::vec3(0, 0, -1);

        fwd.y = 0.0f;
        if (glm::length2(fwd) > 0.0f)
            fwd = glm::normalize(fwd);

        right = glm::normalize(glm::cross(fwd, glm::vec3(0, 1, 0)));
    }

    glm::vec3 dir = fwd * move.y + right * move.x;

    if (glm::length2(dir) > 0.0f)
        dir = glm::normalize(dir);

    const bool sprint = ImGui::IsKeyDown(ImGuiKey_ModShift);
    animationComponentPtr->SetBool("sprint", sprint);

    const float spd = sprint ? SprintSpeed : MoveSpeed;
    return dir * spd;
}

void QECharacterController::SyncFromCharacter()
{
    if (!mTransform || mCharacter == nullptr)
        return;

    const glm::vec3 desiredPos = ToGLM(mCharacter->GetPosition());
    const glm::quat desiredRot = ToGLM(mCharacter->GetRotation());

    const glm::vec3 currentPos = mTransform->GetWorldPosition();
    const glm::quat currentRot = mTransform->GetWorldRotation();

    glm::vec3 deltaPos = desiredPos - currentPos;
    mTransform->TranslateWorld(deltaPos);

    glm::quat deltaRot = desiredRot * glm::inverse(currentRot);
    mTransform->RotateWorld(deltaRot);
}

void QECharacterController::UpdateCharacterOrientation(glm::vec3 dir, const float dt)
{
    glm::vec3 targetDir = glm::vec3(dir.x, 0.0f, dir.z);
    if (glm::length2(targetDir) < 1e-6f)
        return;

    targetDir = glm::normalize(targetDir);

    const float yaw = std::atan2(targetDir.x, targetDir.z);
    const glm::quat targetWorldRot = glm::angleAxis(yaw, glm::vec3(0, 1, 0));

    // Root = target * inv(offset)
    auto visualTransform = this->Owner->GetComponentInChildren<QETransform>(false);
    const glm::quat targetRootWorldRot = glm::normalize(targetWorldRot * glm::inverse(visualTransform->localRotation));

    // Rotación actual del root (mundo)
    const glm::quat currentRootWorldRot = ToGLM(mCharacter->GetRotation());

    // Interpolar con límite de velocidad angular
    // maxTurnSpeedRad = cuántos radianes/seg puede girar el root
    constexpr float maxTurnSpeedRad = 6.0f; // ~343°/s, ajusta
    const float t = glm::clamp(maxTurnSpeedRad * dt, 0.0f, 1.0f);

    const glm::quat newRootWorldRot = glm::normalize(glm::slerp(currentRootWorldRot, targetRootWorldRot, t));

    mCharacter->SetRotation(ToJPH(newRootWorldRot));
}

void QECharacterController::UpdateCharacterAnimation(glm::vec3 velocity)
{
    float vLength = glm::length(velocity);
    animationComponentPtr->SetFloat("speed", vLength);
    constexpr float kMoveEpsilon = 0.01f;
    animationComponentPtr->SetFloat("forward", float(vLength > kMoveEpsilon));

    if (ImGui::IsKeyReleased(ImGuiKey_LeftCtrl))
    {
        mCrouched = !mCrouched;
        animationComponentPtr->SetBool("crouch", mCrouched);
    }
}

void QECharacterController::UpdateJumpAnimation(const float dt)
{
    const bool grounded = mCharacter->IsSupported();

    if (!grounded)
        mVelocity.y += GravityY * dt;
    else
        mVelocity.y = std::min(mVelocity.y, 0.0f);

    if (grounded && ImGui::IsKeyPressed(ImGuiKey_Space))
    {
        animationComponentPtr->SetTrigger("jump", true);
        pendingJump = true;
    }

    if (pendingJump && animationComponentPtr->GetCurrentState().Id == "Jumping")
    {
        if (animationComponentPtr->animator->GetNormalizedTime() >= JumpAnimDelay)
        {
            if (mCharacter->IsSupported())
                mVelocity.y = JumpSpeed;

            pendingJump = false;
            blockAnimationDisplacement = false;
        }
        else
        {
            blockAnimationDisplacement = true;
        }
    }

    animationComponentPtr->SetBool("grounded", grounded);

    const float fallThreshold = 0.05f;
    const bool falling = (!grounded) && (mVelocity.y < -fallThreshold);
    animationComponentPtr->SetBool("falling", falling);
}

void QECharacterController::CheckBlockAnimationDisplacement()
{
        blockAnimationDisplacement =
            animationComponentPtr->GetCurrentState().Id == "Landing" ||
            animationComponentPtr->GetCurrentState().Id == "StandToCrouch" ||
            animationComponentPtr->GetCurrentState().Id == "CrouchToStand";
}

void QECharacterController::QEUpdate()
{
    if (!mCharacter)
    {
        BuildOrUpdateCharacter();
        if (!mCharacter)
            return;
    }

    const float dt = Timer::DeltaTime;
    if (dt <= 0.0f) return;

    if (mStartupLockFrames < 2)
    {
        mStartupLockFrames++;

        JPH::TempAllocatorImpl temp_alloc(4 * 1024 * 1024);
        auto bp_filter = MakeBPFilter();
        BodyFilterSelfIgnore body_filter(mPhysBody ? mPhysBody->body : JPH::BodyID());

        mCharacter->Update(
            dt,
            JPH::Vec3(0.0f, GravityY, 0.0f),
            bp_filter,
            kAllLayersFilter,
            body_filter,
            JPH::ShapeFilter(),
            temp_alloc
        );

        return;
    }

    if (QESessionManager::getInstance()->IsEditor()) return;
    if (!mTransform) return;

    CheckBlockAnimationDisplacement();
    UpdateJumpAnimation(dt);

    if (blockAnimationDisplacement) return;

    const glm::vec3 wishVel = ReadMoveInput();
    mVelocity.x = wishVel.x;
    mVelocity.z = wishVel.z;

    mCharacter->SetLinearVelocity(JPH::Vec3(mVelocity.x, mVelocity.y, mVelocity.z));

    // Orientation
    UpdateCharacterOrientation(wishVel, dt);

    JPH::TempAllocatorImpl temp_alloc(4 * 1024 * 1024);
    auto bp_filter = MakeBPFilter();
    BodyFilterSelfIgnore body_filter(mPhysBody ? mPhysBody->body : JPH::BodyID());

    mCharacter->Update(
        dt,
        JPH::Vec3(0.0f, GravityY, 0.0f),
        bp_filter,
        kAllLayersFilter,
        body_filter,
        JPH::ShapeFilter(),
        temp_alloc
    );

    mGrounded = mCharacter->IsSupported();

    SyncFromCharacter();

    UpdateCharacterAnimation(wishVel);
}
