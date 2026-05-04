#pragma once
#ifndef QE_CAMERA_CONTROLLER_H
#define QE_CAMERA_CONTROLLER_H

#include <QECamera.h>

class QECameraController : public QEGameComponent
{
public:
    REFLECTABLE_COMPONENT(QECameraController)
private:
    bool firstMouse = true;
    double lastMouseX = 0.0;
    double lastMouseY = 0.0;
    float yawDeg = 0.0f;
    float pitchDeg = 0.0f;

    std::shared_ptr<QETransform> _OwnerTransform = nullptr;

public:
    REFLECT_PROPERTY(float, Speed)
    REFLECT_PROPERTY(float, MouseSensitivity)

private:
    void GetYawPitchFromForward(const glm::vec3& fwd, float& yawDeg, float& pitchDeg);
    void RuntimeCameraController(float dt);
    void RuntimeRotate(float dt);


public:
    void QEStart() override;
    void QEInit() override;
    void QEUpdate() override;
    void QEDestroy() override;

public:
    void SetInputEnabled(bool enabled) { _inputEnabled = enabled; }
    bool IsInputEnabled() const { return _inputEnabled; }

private:
    bool _inputEnabled = true;
};



namespace QE
{
    using ::QECameraController;
} // namespace QE
// QE namespace aliases
#endif // !QE_CAMERA_CONTROLLER_H
