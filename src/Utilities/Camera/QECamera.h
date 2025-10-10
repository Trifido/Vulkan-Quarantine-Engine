#pragma once
#ifndef QE_CAMERA_H
#define QE_CAMERA_H
#define _USE_MATH_DEFINES

#include <glm/glm.hpp>
#include <math.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <vulkan/vulkan_core.h>
#include <memory>
#include "UBO.h"
#include <CameraDto.h>
#include <QEGameComponent.h>

class FrustumComponent;

class QECamera : public QEGameComponent
{
    REFLECTABLE_COMPONENT(QECamera)
private:
    DeviceModule* deviceModule = nullptr;
    std::shared_ptr<CameraUniform> cameraUniform;
    glm::vec4 normalize_plane(glm::vec4 plane);
    bool _cameraModified = true;
    bool allowEditorControls = false;

protected:
    REFLECT_PROPERTY(float, nearPlane)
    REFLECT_PROPERTY(float, farPlane)
    REFLECT_PROPERTY(glm::vec3, cameraUp)
    REFLECT_PROPERTY(float, fov)
    REFLECT_PROPERTY(float, pitch)
    REFLECT_PROPERTY(float, yaw)

    glm::vec3 cameraRight;
    bool firstMouse;
    float cameraSpeed;
    float lastX;
    float lastY;

public:
    REFLECT_PROPERTY(glm::vec3, cameraPos)
    REFLECT_PROPERTY(glm::vec3, cameraFront)

    std::shared_ptr<FrustumComponent> frustumComponent;
    float WIDTH, HEIGHT;
    glm::mat4 view;
    glm::mat4 projection;
    glm::mat4 VP;

protected:
    void UpdateUniform();
    void UpdateFrustumPlanes();

public:
    QECamera();
    QECamera(const float width, const float height, const CameraDto& cameraDto = CameraDto(), bool isEditor = false);
    bool LoadCameraDto(const float width, const float height, const CameraDto& cameraDto);
    CameraDto CreateCameraDto();
    void SetEditorControlsEnabled(bool v) { allowEditorControls = v; }
    void EditorCameraController(float deltaTime);
    float GetFOV() { return fov; }
    float* GetRawFOV() { return &fov; }
    float GetNear() { return nearPlane; }
    float* GetRawNearPlane() { return &nearPlane; }
    float GetFar() { return farPlane; }
    float* GetRawFarPlane() { return &farPlane; }
    float* GetRawCameraPosition() { return &cameraPos[0]; }
    float* GetRawCameraFront() { return &cameraFront[0]; }
    CameraUniform* GetCameraUniform() { return this->cameraUniform.get(); }
    void UpdateViewportSize(VkExtent2D size);
    void UpdateCamera();
    bool IsModified();
    void ResetModifiedField();

    void QEStart() override;
    void QEInit() override;
    void QEUpdate() override;
    void QEDestroy() override;

private:
    void EditorScroll();
    void EditorRotate();
    void UpdateUBOCamera();
};

#endif // !CAMERA_H
