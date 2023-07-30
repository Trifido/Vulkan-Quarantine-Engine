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

class Camera
{
private:
    DeviceModule* deviceModule = nullptr;
    std::shared_ptr<CameraUniform> cameraUniform = nullptr;

protected:
    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    const float cameraSpeed = 10.0f;

    bool firstMouse = true;
    float lastX = 1280.0f / 2.0;
    float lastY = 720.0 / 2.0;
    float fov = 45.0f;
    float nearPlane, farPlane;
    float yaw = 0.0f;	// yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right so we initially rotate a bit to the left.
    float pitch = 0.0f;

public:
    std::shared_ptr<UniformBufferObject> cameraUBO = nullptr;
    float WIDTH, HEIGHT;
    glm::vec3 cameraFront;
    glm::vec3 cameraPos;
    glm::mat4 view;
    glm::mat4 projection;
    glm::mat4 VP;
    Camera() {};
    Camera(float width, float height);
    void CameraController(float deltaTime);
    void EditorScroll();
    void EditorRotate();
    float GetFOV() { return fov; }
    float* GetRawFOV() { return &fov; }
    float* GetRawNearPlane() { return &nearPlane; }
    float* GetRawFarPlane() { return &farPlane; }
    float* GetRawCameraPosition() { return &cameraPos[0]; }
    float* GetRawCameraFront() { return &cameraFront[0]; }
    void CheckCameraAttributes(float* positionCamera, float* frontCamera, float fov, float nearPlane, float farPlane);
    void InvertPitch(float heightPos);
    void UpdateSize(VkExtent2D size);
    void UpdateUBOCamera();

protected:
    void UpdateUniform();
};

#endif // !CAMERA_H



