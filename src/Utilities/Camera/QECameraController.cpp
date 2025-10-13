#include "QECameraController.h"
#include "QEGameObject.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <Timer.h>

void QECameraController::QEStart()
{
    QEGameComponent::QEStart();
    this->EditorControls = true;
    this->MouseSensitivity = 0.1f;
    this->Speed = 10.0f;
}

void QECameraController::QEInit()
{
    _CameraTarget = this->Owner->GetComponent<QECamera>();
    _OwnerTransform = this->Owner->GetComponent<QETransform>();
    QEGameComponent::QEInit();
}

void QECameraController::QEUpdate()
{
    if (!EditorControls || !_OwnerTransform) return;

    float dt = Timer::DeltaTime;
    if (dt <= 0.0f) return;

    EditorScroll(dt);
    EditorRotate(dt);
    EditorCameraController(dt);
}

void QECameraController::QEDestroy()
{
    QEGameComponent::QEDestroy();
}

void QECameraController::EditorCameraController(float dt)
{
    if (!this->EditorControls)
        return;

    EditorScroll(dt);
    EditorRotate(dt);

    glm::vec3 dir(0.0f);
    glm::vec3 fwd = _OwnerTransform->Forward();
    glm::vec3 right = _OwnerTransform->Right();
    glm::vec3 up = _OwnerTransform->Up();

    ImGuiIO& io = ImGui::GetIO();
    if (io.KeyShift && ImGui::IsKeyDown(ImGuiKey_W)) dir += fwd;
    if (io.KeyShift && ImGui::IsKeyDown(ImGuiKey_S)) dir -= fwd;
    if (io.KeyShift && ImGui::IsKeyDown(ImGuiKey_D)) dir += right;
    if (io.KeyShift && ImGui::IsKeyDown(ImGuiKey_A)) dir -= right;
    if (io.KeyShift && ImGui::IsKeyDown(ImGuiKey_E)) dir += up;
    if (io.KeyShift && ImGui::IsKeyDown(ImGuiKey_Q)) dir -= up;

    if (ImGui::IsKeyDown(ImGuiKey_UpArrow))    dir += fwd;
    if (ImGui::IsKeyDown(ImGuiKey_DownArrow))  dir -= fwd;
    if (ImGui::IsKeyDown(ImGuiKey_RightArrow)) dir += right;
    if (ImGui::IsKeyDown(ImGuiKey_LeftArrow))  dir -= right;

    if (glm::length2(dir) > 0.0f)
    {
        dir = glm::normalize(dir);
        _OwnerTransform->TranslateWorld(dir * Speed * dt);
    }
}

void QECameraController::EditorScroll(float dt)
{
    ImGuiIO& io = ImGui::GetIO();
    float wheel = io.MouseWheel;
    if (wheel == 0.0f) return;

    if (io.KeyShift && _CameraTarget)
    {   // Zoom
        float fov = _CameraTarget->GetFOV();
        fov -= wheel;
        fov = std::clamp(fov, 1.0f, 90.0f);
        _CameraTarget->SetFOV(fov);
    }
    else
    {   // Dolly
        glm::vec3 fwd = _OwnerTransform->Forward();
        _OwnerTransform->TranslateWorld(fwd * (wheel * Speed * 0.5f));
    }
}

void QECameraController::EditorRotate(float dt)
{
    ImGuiIO& io = ImGui::GetIO();

    if (io.KeyShift && ImGui::IsMouseDown(1))
    {
        if (firstMouse)
        {
            lastX = io.MousePos.x;
            lastY = io.MousePos.y;
            firstMouse = false;
            return;
        }

        float xoffset = io.MousePos.x - lastX;
        float yoffset = lastY - io.MousePos.y; // reversed since y-coordinates go from bottom to top
        lastX = io.MousePos.x;
        lastY = io.MousePos.y;

        float dxDeg = xoffset * MouseSensitivity;
        float dyDeg = yoffset * MouseSensitivity;

        yawDeg += dxDeg;
        pitchDeg += dyDeg;
        pitchDeg = std::clamp(pitchDeg, -89.0f, 89.0f);

        glm::quat qYawWorld = glm::angleAxis(glm::radians(dxDeg), glm::vec3(0, 1, 0));
        glm::quat qPitchLocal = glm::angleAxis(glm::radians(dyDeg), _OwnerTransform->Right());

        _OwnerTransform->RotateWorld(qYawWorld);
        _OwnerTransform->RotateLocal(qPitchLocal);
    }
    else
    {
        firstMouse = true;
    }
}
