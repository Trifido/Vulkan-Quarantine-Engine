#pragma once
#ifndef QE_CAMERA_CONTROLLER_H
#define QE_CAMERA_CONTROLLER_H

#include <QECamera.h>

class QECameraController : public QEGameComponent
{
    REFLECTABLE_COMPONENT(QECameraController)
private:
    bool firstMouse = true;
    float lastX = 0.0f;
    float lastY = 0.0f;
    float yawDeg = 0.0f;
    float pitchDeg = 0.0f;

    std::shared_ptr<QECamera> _CameraTarget = nullptr;
    std::shared_ptr<QETransform> _OwnerTransform = nullptr;

public:
    REFLECT_PROPERTY(float, Speed)
    REFLECT_PROPERTY(float, MouseSensitivity)
    REFLECT_PROPERTY(bool, EditorControls)

private:
    void EditorCameraController(float dt);
    void EditorScroll(float dt);
    void EditorRotate(float dt);

public:
    void QEStart() override;
    void QEInit() override;
    void QEUpdate() override;
    void QEDestroy() override;
};

#endif // !QE_CAMERA_CONTROLLER_H
