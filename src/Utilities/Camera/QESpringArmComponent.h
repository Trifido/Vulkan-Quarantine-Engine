#pragma once

#ifndef QE_SPRING_ARM_COMPONENT_H
#define QE_SPRING_ARM_COMPONENT_H

#include <QEGameComponent.h>

struct SweepHit
{
    bool hit = false;
    float distance = 0.0f;
    glm::vec3 point{};
    glm::vec3 normal{};
};

struct CameraMouseSettings
{
    float sensX = 0.12f;
    float sensY = 0.12f;
    bool  invertY = false;
};

class QESpringArmComponent : public QEGameComponent
{
    REFLECTABLE_DERIVED_COMPONENT(QESpringArmComponent, QEGameComponent)

private:
    CameraMouseSettings g_camMouse;
    glm::vec3 m_smoothedPivot{};
    glm::quat m_smoothedRot{};
    float     m_currentArmLen = 3.0f;

    glm::vec3 m_camPos{};
    glm::quat m_camRot{ 1,0,0,0 };

public:
    REFLECT_PROPERTY(float, TargetArmLength)
    REFLECT_PROPERTY(float, MinArmLength)
    REFLECT_PROPERTY(float, MaxArmLength)

    REFLECT_PROPERTY(glm::vec3, TargetOffset)
    REFLECT_PROPERTY(glm::vec3, SocketOffset)

    REFLECT_PROPERTY(bool, DoCollisionTest)
    REFLECT_PROPERTY(float, ProbeRadius)

    REFLECT_PROPERTY(float, PosLagSpeed)
    REFLECT_PROPERTY(float, RotLagSpeed)

    REFLECT_PROPERTY(float, Yaw)
    REFLECT_PROPERTY(float, Pitch)
    REFLECT_PROPERTY(float, MinPitch)
    REFLECT_PROPERTY(float, MaxPitch)

    REFLECT_PROPERTY(bool, InheritYaw)
    REFLECT_PROPERTY(bool, InheritPitch)
    REFLECT_PROPERTY(bool, InheritRoll)

private:
    // Helpers
    glm::vec3 ComputePivotWorldPos() const;
    glm::quat ComputeDesiredRotation() const;
    bool SphereSweep(const glm::vec3& start, const glm::vec3& end, float radius, SweepHit& outHit) const;

    // Utils
    static float  SmoothExp(float current, float target, float speed, float dt);
    static glm::vec3 SmoothExpVec3(glm::vec3 c, glm::vec3 t, float speed, float dt);
    static glm::quat SmoothExpQuat(glm::quat c, glm::quat t, float speed, float dt);

    void GetParentInterpolatedTRS(glm::vec3& pos, glm::quat& rot) const;

public:
    QESpringArmComponent();

    void AddYawInput(float deltaDegrees);
    void AddPitchInput(float deltaDegrees);
    void AddZoomInput(float deltaLength);
    glm::vec3 GetCameraWorldPosition() const { return m_camPos; }
    glm::quat GetCameraWorldRotation() const { return m_camRot; }

    void QEStart() override;
    void QEUpdate() override;
};

#endif // !QE_SPRING_ARM_COMPONENT_H


