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
    JumpSpeed = 6.5f;
    GravityY = -9.81f;
    MaxSlopeDeg = 50.0f;
    TurnSpeedDeg = 540.0f;

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

    const float spd = ImGui::IsKeyDown(ImGuiKey_ModShift) ? SprintSpeed : MoveSpeed;
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

void QECharacterController::UpdateCharacterOrientation(glm::vec3 dir)
{
    glm::vec3 targetDir = glm::vec3(dir.x, 0.0f, dir.z);
    if (glm::length2(targetDir) < 1e-6f)
        return;

    targetDir = glm::normalize(targetDir);

    float yaw = std::atan2(targetDir.x, targetDir.z);
    glm::quat targetWorldRot = glm::angleAxis(yaw, glm::vec3(0, 1, 0));

    // Root = target * inv(offset)
    auto visual = this->Owner->GetComponentInChildren<QETransform>(false);
    glm::quat rootWorldRot = glm::normalize(targetWorldRot * glm::inverse(visual->localRotation));

    mCharacter->SetRotation(ToJPH(rootWorldRot));
}

void QECharacterController::QEUpdate()
{
    if (QESessionManager::getInstance()->IsEditor()) return;

    if (!mCharacter)
    {
        BuildOrUpdateCharacter();
        if (!mCharacter)
            return;
    }

    if (!mTransform) return;

    const float dt = Timer::DeltaTime;
    if (dt <= 0.0f) return;

    const glm::vec3 wishVel = ReadMoveInput();
    
    const bool wasGrounded = mCharacter->IsSupported();

    if (wasGrounded)
    {
        if (ImGui::IsKeyPressed(ImGuiKey_Space))
            mVelocity.y = JumpSpeed;
        else
            mVelocity.y = std::min(mVelocity.y, 0.0f);
    }
    else
    {
        mVelocity.y += GravityY * dt;
    }

    mVelocity.x = wishVel.x;
    mVelocity.z = wishVel.z;

    mCharacter->SetLinearVelocity(JPH::Vec3(mVelocity.x, mVelocity.y, mVelocity.z));

    // Orientation
    UpdateCharacterOrientation(wishVel);

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

    auto characterDir = mTransform->Forward();
    characterDir = mTransform->Forward();
    characterDir.y = 0.0f;
    characterDir = glm::normalize(characterDir);

    auto camTransform = mSpringArm->Owner->GetComponentInChildren<QETransform>(false);
    auto cameraDir = camTransform->Forward();
    auto pos = mTransform->GetWorldPosition();
    auto endPosCamera = pos + cameraDir * 2.0f;
    auto endPosCharacter = pos + characterDir * 2.0f;

    QEDebugSystem::getInstance()->AddLine(pos, endPosCamera, glm::vec4(0.0, 1.0, 0.0, 1.0));
    QEDebugSystem::getInstance()->AddLine(pos, endPosCharacter, glm::vec4(1.0, 0.0, 0.0, 1.0));
}
