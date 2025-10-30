#include "QECharacterController.h"

#include <QEGameObject.h>
#include <QETransform.h>
#include <Collider.h>
#include <PhysicsBody.h>
#include <PhysicsModule.h>
#include <Timer.h>

#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <imgui.h>

// --- Helpers locales para filtros ---
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
    MoveSpeed = 5.0f;
    SprintSpeed = 8.0f;
    JumpSpeed = 6.5f;
    GravityY = -9.81f;
    MaxSlopeDeg = 50.0f;

    mSettings.mBackFaceMode = JPH::EBackFaceMode::IgnoreBackFaces;
    mSettings.mEnhancedInternalEdgeRemoval = true;
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
    mPhysBody = Owner->GetComponent<PhysicsBody>(); // opcional

    BuildOrUpdateCharacter();

    QEGameComponent::QEInit();
}

void QECharacterController::QEDestroy()
{
    mCharacter = nullptr;
    QEGameComponent::QEDestroy();
}

void QECharacterController::BuildOrUpdateCharacter()
{
    if (mSettings.mShape == mLastShape)
        return; // nada que actualizar

    mLastShape = mSettings.mShape;

    if (!mCollider || mCollider->colShape == nullptr)
        return;

    // Actualiza settings a partir del collider / opciones
    mSettings.mShape = mCollider->colShape;
    mSettings.mMaxSlopeAngle = JPH::DegreesToRadians(MaxSlopeDeg);
    mSettings.mBackFaceMode = JPH::EBackFaceMode::IgnoreBackFaces;
    mSettings.mEnhancedInternalEdgeRemoval = true;

    // Crear character si no existe
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

    JPH::TempAllocatorImpl temp_alloc(32 * 1024);

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
    glm::vec3 dir(0);
    // Sustituye por tu sistema real de input; aquí un ejemplo con ImGui keys
    if (ImGui::IsKeyDown(ImGuiKey_W)) dir += glm::vec3(0, 0, -1);
    if (ImGui::IsKeyDown(ImGuiKey_S)) dir += glm::vec3(0, 0, 1);
    if (ImGui::IsKeyDown(ImGuiKey_A)) dir += glm::vec3(-1, 0, 0);
    if (ImGui::IsKeyDown(ImGuiKey_D)) dir += glm::vec3(1, 0, 0);

    if (glm::length2(dir) > 0) dir = glm::normalize(dir);

    const float spd = ImGui::IsKeyDown(ImGuiKey_ModShift) ? SprintSpeed : MoveSpeed;
    return dir * spd;
}

void QECharacterController::SyncFromCharacter()
{
    if (!mTransform || mCharacter == nullptr) return;

    const glm::vec3 pos = ToGLM(mCharacter->GetPosition());
    const glm::quat rot = ToGLM(mCharacter->GetRotation()); // puedes limitar a yaw si prefieres

    mTransform->TranslateWorld(pos);
    mTransform->RotateWorld(rot);
}

// --- Main update: velocidad -> Update(...) -> copiar transform ---
void QECharacterController::QEUpdate()
{
    if (!mCharacter || !mTransform) return;

    const float dt = Timer::DeltaTime;
    if (dt <= 0.0f) return;

    // 1) Input horizontal deseado
    const glm::vec3 wish = ReadMoveInput();

    // 2) Estado de suelo antes de integrar
    const bool wasGrounded = mCharacter->IsSupported();

    // 3) Integración vertical + salto
    if (wasGrounded)
    {
        if (ImGui::IsKeyPressed(ImGuiKey_Space))
            mVelocity.y = JumpSpeed;
        else
            mVelocity.y = std::min(mVelocity.y, 0.0f);
    }
    else
    {
        mVelocity.y += GravityY * dt; // GravityY negativa
    }

    // 4) Mezcla final de velocidad (X/Z del input, Y integrada)
    mVelocity.x = wish.x;
    mVelocity.z = wish.z;

    mCharacter->SetLinearVelocity(JPH::Vec3(mVelocity.x, mVelocity.y, mVelocity.z));

    // 5) Update del character con filtros
    JPH::TempAllocatorImpl temp_alloc(32 * 1024);
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

    // 6) Estado de suelo y sincronización de transform
    mGrounded = mCharacter->IsSupported();
    SyncFromCharacter();
}
