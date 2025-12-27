#pragma once
#ifndef QE_CHARACTER_CONTROLLER
#define QE_CHARACTER_CONTROLLER

#include <QEGameComponent.h>
#include <QETransform.h>
#include <memory>

// Jolt
#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>

class QECollider;
class PhysicsModule;
class QEAnimationComponent;
class PhysicsBody;
class QESpringArmComponent;

class QECharacterController : public QEGameComponent
{
    REFLECTABLE_DERIVED_COMPONENT(QECharacterController, QEGameComponent)
    private:
        REFLECT_PROPERTY(float, MoveSpeed)       // m/s
        REFLECT_PROPERTY(float, SprintSpeed)     // m/s
        REFLECT_PROPERTY(float, JumpSpeed)       // m/s
        REFLECT_PROPERTY(float, GravityY)        // m/s^2 (negativo)
        REFLECT_PROPERTY(float, MaxSlopeDeg)     // grados
        REFLECT_PROPERTY(float, TurnSpeedDeg)    // grados/seg

        bool   mGrounded = false;
        glm::vec3 mVelocity{ 0.0f };

        std::shared_ptr<QETransform>        mTransform = nullptr;
        std::shared_ptr<QECollider>         mCollider = nullptr;
        std::shared_ptr<PhysicsBody>        mPhysBody = nullptr;
        std::shared_ptr<QEAnimationComponent> animationComponentPtr = nullptr;
        std::shared_ptr<QESpringArmComponent> mSpringArm = nullptr;

        JPH::Ref<JPH::CharacterVirtual>     mCharacter;
        JPH::CharacterVirtualSettings       mSettings;
        JPH::ShapeRefC mLastShape;
    private:
        static inline JPH::Quat  ToJPH(const glm::quat& q) { return JPH::Quat(q.x, q.y, q.z, q.w); }
        static inline JPH::RVec3 ToJPH(const glm::vec3& v) { return JPH::RVec3((double)v.x, (double)v.y, (double)v.z); }
        static inline glm::vec3  ToGLM(const JPH::RVec3& v) { return glm::vec3((float)v.GetX(), (float)v.GetY(), (float)v.GetZ()); }
        static inline glm::quat  ToGLM(const JPH::Quat& q) { return glm::quat(q.GetW(), q.GetX(), q.GetY(), q.GetZ()); }

        glm::vec3 ReadMoveInput() const;
        void      BuildOrUpdateCharacter();
        void      SyncFromCharacter();

        void UpdateCharacterOrientation(glm::vec3 dir);

    public:
        QECharacterController();

        void DebugDraw(JPH::DebugRenderer& renderer);

        void QEStart() override;
        void QEInit() override;
        void QEUpdate() override;
        void QEDestroy() override;
};

#endif // !QE_CHARACTER_CONTROLLER

