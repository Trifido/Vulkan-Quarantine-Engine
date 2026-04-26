#pragma once

#include <QEGameComponent.h>
#include <QECamera.h>

class QETransform;

class QEEditorCameraController : public QEGameComponent
{
public:
    REFLECTABLE_COMPONENT(QEEditorCameraController)

private:
    bool firstMouse = true;
    double lastMouseX = 0.0;
    double lastMouseY = 0.0;
    float yawDeg = 0.0f;
    float pitchDeg = 0.0f;

    std::shared_ptr<QECamera> _cameraTarget = nullptr;
    std::shared_ptr<QETransform> _ownerTransform = nullptr;

public:
    REFLECT_PROPERTY(float, Speed)
    REFLECT_PROPERTY(float, MouseSensitivity)

private:
    void GetYawPitchFromForward(const glm::vec3& forward, float& outYawDeg, float& outPitchDeg);
    void HandleTranslation(float dt);
    void HandleScroll();
    void HandleRotation();

public:
    bool IsSerializable() const override { return false; }

    void QEStart() override;
    void QEInit() override;
    void QEUpdate() override;
    void QEDestroy() override;

    void SetInputEnabled(bool enabled) { _inputEnabled = enabled; }
    bool IsInputEnabled() const { return _inputEnabled; }

private:
    bool _inputEnabled = true;
};
