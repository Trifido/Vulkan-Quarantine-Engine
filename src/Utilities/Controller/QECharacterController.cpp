#include "QECharacterController.h"

#include <QEGameObject.h>
#include <QETransform.h>
#include <Collider.h>
#include <PhysicsBody.h>
#include <PhysicsModule.h>
#include <Timer.h>

#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Renderer/DebugRenderer.h>
#include <imgui.h>

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
    auto* pm = PhysicsModule::getInstance();
    JPH::PhysicsSystem& system = pm->World();

    if (!mCollider || mCollider->colShape == nullptr)
        return;

    mSettings.mShape = mCollider->colShape;
    mSettings.mMaxSlopeAngle = JPH::DegreesToRadians(MaxSlopeDeg);

    // 2) Si no existe el character: créalo
    if (mCharacter == nullptr)
    {
        const glm::vec3 p = mTransform ? mTransform->GetWorldPosition() : glm::vec3(0);
        const glm::quat q = mTransform ? mTransform->GetWorldRotation() : glm::quat(1, 0, 0, 0);

        mCharacter = new JPH::CharacterVirtual(
            &mSettings,
            ToJPH(p),
            ToJPH(q),
            &system
        );

        // Pose inicial desde el transform
        const glm::vec3 p = mTransform ? mTransform->GetWorldPosition() : glm::vec3(0);
        const glm::quat q = mTransform ? mTransform->GetWorldRotation() : glm::quat(1, 0, 0, 0);
        mCharacter->SetPosition(ToJPH(p));
        mCharacter->SetRotation(ToJPH(q));
    }
    else
    {
        // (Opcional) si cambiaste la shape en runtime:
        JPH::TempAllocatorImpl temp_alloc(16 * 1024);
        mCharacter->SetShape(mSettings.mShape.GetPtr(), /*maxPenDepth*/ 0.05f,
            pm->GetBroadPhaseLayerFilter(),
            pm->GetObjectLayerFilter(),
            pm->GetBodyFilterSelfIgnore(mPhysBody ? mPhysBody->body : JPH::BodyID()),
            JPH::ShapeFilter(), temp_alloc); // firma SetShape(...) :contentReference[oaicite:2]{index=2}
    }
}

glm::vec3 QECharacterController::ReadMoveInput(float dt) const
{
    glm::vec3 dir(0);
    // Sustituye por tu sistema real de input; aquí un ejemplo con ImGui keys
    if (ImGui::IsKeyDown(ImGuiKey_W)) dir += glm::vec3(0, 0, -1);
    if (ImGui::IsKeyDown(ImGuiKey_S)) dir += glm::vec3(0, 0, 1);
    if (ImGui::IsKeyDown(ImGuiKey_A)) dir += glm::vec3(-1, 0, 0);
    if (ImGui::IsKeyDown(ImGuiKey_D)) dir += glm::vec3(1, 0, 0);

    if (glm::length2(dir) > 0) dir = glm::normalize(dir);

    const float spd = ImGui::IsKeyDown(ImGuiKey_ModShift) ? SprintSpeed : MoveSpeed;
    return dir * spd; // *dt NO: la velocity es en m/s, el step se hace en Update(...)
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

    auto* pm = PhysicsModule::getInstance();

    // 1) Horizontal deseado
    const glm::vec3 wish = ReadMoveInput(dt);

    // 2) Ground state antes de modificar
    const bool wasGrounded = mCharacter->IsSupported(); // CharacterBase::IsSupported() :contentReference[oaicite:3]{index=3}

    // 3) Vertical + salto
    //    - En CharacterVirtual la gravedad debes aplicarla tú al vector velocidad. :contentReference[oaicite:4]{index=4}
    if (wasGrounded)
    {
        // salto (un único impulso)
        if (ImGui::IsKeyPressed(ImGuiKey_Space))
            mVelocity.y = JumpSpeed;
        else
            mVelocity.y = std::min(mVelocity.y, 0.0f);
    }
    else
    {
        mVelocity.y += GravityY * dt; // GravityY negativa
    }

    // 4) Mezcla final de velocidad
    mVelocity.x = wish.x;
    mVelocity.z = wish.z;

    // 5) SetLinearVelocity y Update
    mCharacter->SetLinearVelocity(JPH::Vec3(mVelocity.x, mVelocity.y, mVelocity.z)); // :contentReference[oaicite:5]{index=5}

    // Nota: El parámetro gravedad del Update solo se usa para empujar hacia abajo cuando vas sobre un body moviéndose. :contentReference[oaicite:6]{index=6}
    JPH::TempAllocatorImpl temp_alloc(32 * 1024);
    mCharacter->Update(dt,
        JPH::Vec3(0.0f, GravityY, 0.0f),
        pm->GetBroadPhaseLayerFilter(),
        pm->GetObjectLayerFilter(),
        pm->GetBodyFilterSelfIgnore(mPhysBody ? mPhysBody->body : JPH::BodyID()),
        JPH::ShapeFilter(),
        temp_alloc); // firma Update(...) :contentReference[oaicite:7]{index=7}

    // 6) Estado de suelo tras mover
    mGrounded = mCharacter->IsSupported();

    // 7) Copiar transform
    SyncFromCharacter();

    // (Opcional) mantener pegado al suelo extra:
    // mCharacter->StickToFloor(/*stepDown*/ 0.3f, pm->GetBroadPhaseLayerFilter(), pm->GetObjectLayerFilter(),
    //                          pm->GetBodyFilterSelfIgnore(...), JPH::ShapeFilter(), temp_alloc); // ver docs: 'project onto the floor' :contentReference[oaicite:8]{index=8}
}
