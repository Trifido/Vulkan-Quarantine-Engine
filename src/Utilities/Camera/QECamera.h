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
#include "QETransform.h"

class FrustumComponent;

class QECamera : public QEGameComponent
{
    REFLECTABLE_COMPONENT(QECamera)
private:
    struct { float fov, n, f, w, h; } _projCache{ -1,-1,-1,-1,-1 };

    DeviceModule* deviceModule = nullptr;
    std::shared_ptr<QETransform> _OwnerTransform = nullptr;
    glm::vec4 normalize_plane(glm::vec4 plane);
    bool _dirtyData = true;
    uint32_t _lastTransformVersion = 0;

protected:
    REFLECT_PROPERTY(float, _near)
    REFLECT_PROPERTY(float, _far)
    REFLECT_PROPERTY(float, _fov)

public:
    std::shared_ptr<FrustumComponent> _frustumComponent;
    std::shared_ptr<UniformCamera> CameraData;

    REFLECT_PROPERTY(float, Width)
    REFLECT_PROPERTY(float, Height)

private:
    void UpdateProjectionIfNeeded();

protected:
    void UpdateFrustumPlanes();

public:
    QECamera();
    QECamera(const float width, const float height);
    void UpdateViewportSize(VkExtent2D size);
    void UpdateCamera();

    float GetNear() { return _near; }
    float GetFar() { return _far; }
    float GetFOV() { return _fov; }
    void SetNear(float newValue);
    void SetFar(float newValue);
    void SetFOV(float newValue);

    void QEStart() override;
    void QEInit() override;
    void QEUpdate() override;
    void QEDestroy() override;
};

#endif // !CAMERA_H
