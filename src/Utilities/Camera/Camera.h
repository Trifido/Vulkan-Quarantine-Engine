#pragma once
#ifndef CAMERA_H
#define CAMERA_H
#define _USE_MATH_DEFINES

#include <glm/glm.hpp>
#include <math.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <vulkan/vulkan_core.h>
#include <memory>
#include "UBO.h"
#include <FrustumComponent.h>
#include <CameraDto.h>

class Camera
{
private:
    DeviceModule* deviceModule = nullptr;
    std::shared_ptr<CameraUniform> cameraUniform = nullptr;
    glm::vec4 normalize_plane(glm::vec4 plane);
    bool isInputUpdated = true;

protected:
    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 cameraRight = glm::vec3(1.0f, 0.0f, 0.0f);

    const float cameraSpeed = 10.0f;

    bool firstMouse = true;
    float lastX = 1280.0f / 2.0;
    float lastY = 720.0 / 2.0;
    float fov = 45.0f;
    float nearPlane, farPlane;
    float yaw = 0.0f;	// yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right so we initially rotate a bit to the left.
    float pitch = 0.0f;

public:
    std::shared_ptr<FrustumComponent> frustumComponent = nullptr;
    std::shared_ptr<UniformBufferObject> cameraUBO = nullptr;
    float WIDTH, HEIGHT;
    glm::vec3 cameraFront;
    glm::vec3 cameraPos;
    glm::mat4 view;
    glm::mat4 projection;
    glm::mat4 VP;

protected:
    void UpdateUniform();
    void UpdateFrustumPlanes();

public:
    Camera();
    Camera(const float width, const float height, const CameraDto& cameraDto = CameraDto());
    bool LoadCameraDto(const float width, const float height, const CameraDto& cameraDto);
    CameraDto CreateCameraDto();
    void CameraController(float deltaTime);
    float GetFOV() { return fov; }
    float* GetRawFOV() { return &fov; }
    float GetNear() { return nearPlane; }
    float* GetRawNearPlane() { return &nearPlane; }
    float GetFar() { return farPlane; }
    float* GetRawFarPlane() { return &farPlane; }
    float* GetRawCameraPosition() { return &cameraPos[0]; }
    float* GetRawCameraFront() { return &cameraFront[0]; }
    void UpdateViewportSize(VkExtent2D size);
    void UpdateCamera();
    void CleanCameraUBO();
    bool IsModified();
    void ResetModifiedField();

private:
    void CheckCameraAttributes(float* positionCamera, float* frontCamera, float fov, float nearPlane, float farPlane);
    void EditorScroll();
    void EditorRotate();
    void InvertPitch(float heightPos);
    void UpdateUBOCamera();
};

#endif // !CAMERA_H



