#include "QEEditorCameraController.h"

#include <QEGameObject.h>
#include <QETransform.h>
#include <Timer.h>
#include <imgui.h>
#include <algorithm>
#include <cmath>

void QEEditorCameraController::QEStart()
{
    QEGameComponent::QEStart();
    MouseSensitivity = 0.1f;
    Speed = 10.0f;
}

void QEEditorCameraController::QEInit()
{
    _cameraTarget = Owner ? Owner->GetComponent<QECamera>() : nullptr;
    _ownerTransform = Owner ? Owner->GetComponent<QETransform>() : nullptr;
    QEGameComponent::QEInit();
}

void QEEditorCameraController::QEUpdate()
{
    if (!_ownerTransform)
        return;

    if (!_inputEnabled)
    {
        firstMouse = true;
        return;
    }

    const float dt = Timer::DeltaTime;
    if (dt <= 0.0f)
        return;

    HandleScroll();
    HandleRotation();
    HandleTranslation(dt);
}

void QEEditorCameraController::QEDestroy()
{
    QEGameComponent::QEDestroy();
}

void QEEditorCameraController::GetYawPitchFromForward(const glm::vec3& forward, float& outYawDeg, float& outPitchDeg)
{
    const glm::vec3 normalizedForward = glm::normalize(forward);
    const float yaw = std::atan2(-normalizedForward.x, -normalizedForward.z);
    const float pitch = std::atan2(normalizedForward.y, std::sqrt(normalizedForward.x * normalizedForward.x + normalizedForward.z * normalizedForward.z));
    outYawDeg = glm::degrees(yaw);
    outPitchDeg = glm::degrees(pitch);
}

void QEEditorCameraController::HandleTranslation(float dt)
{
    glm::vec3 dir(0.0f);
    const glm::vec3 forward = _ownerTransform->Forward();
    const glm::vec3 right = _ownerTransform->Right();
    const glm::vec3 up = _ownerTransform->Up();

    ImGuiIO& io = ImGui::GetIO();
    if (io.KeyShift && ImGui::IsKeyDown(ImGuiKey_W)) dir += forward;
    if (io.KeyShift && ImGui::IsKeyDown(ImGuiKey_S)) dir -= forward;
    if (io.KeyShift && ImGui::IsKeyDown(ImGuiKey_D)) dir += right;
    if (io.KeyShift && ImGui::IsKeyDown(ImGuiKey_A)) dir -= right;
    if (io.KeyShift && ImGui::IsKeyDown(ImGuiKey_E)) dir += up;
    if (io.KeyShift && ImGui::IsKeyDown(ImGuiKey_Q)) dir -= up;

    if (ImGui::IsKeyDown(ImGuiKey_UpArrow)) dir += forward;
    if (ImGui::IsKeyDown(ImGuiKey_DownArrow)) dir -= forward;
    if (ImGui::IsKeyDown(ImGuiKey_RightArrow)) dir += right;
    if (ImGui::IsKeyDown(ImGuiKey_LeftArrow)) dir -= right;

    if (glm::length2(dir) <= 0.0f)
        return;

    dir = glm::normalize(dir);
    _ownerTransform->TranslateWorld(dir * Speed * dt);
}

void QEEditorCameraController::HandleScroll()
{
    ImGuiIO& io = ImGui::GetIO();
    const float wheel = io.MouseWheel;
    if (wheel == 0.0f)
        return;

    if (io.KeyShift && _cameraTarget)
    {
        float fov = _cameraTarget->GetFOV();
        fov -= wheel;
        fov = std::clamp(fov, 1.0f, 90.0f);
        _cameraTarget->SetFOV(fov);
        return;
    }

    const glm::vec3 forward = _ownerTransform->Forward();
    _ownerTransform->TranslateWorld(forward * (wheel * Speed * 0.5f));
}

void QEEditorCameraController::HandleRotation()
{
    ImGuiIO& io = ImGui::GetIO();

    if (io.KeyShift && ImGui::IsMouseDown(1))
    {
        if (firstMouse)
        {
            GetYawPitchFromForward(_ownerTransform->Forward(), yawDeg, pitchDeg);
            firstMouse = false;
            return;
        }

        const float dxDeg = -io.MouseDelta.x * MouseSensitivity;
        const float dyDeg = -io.MouseDelta.y * MouseSensitivity;

        yawDeg += dxDeg;
        pitchDeg = std::clamp(pitchDeg + dyDeg, -89.0f, 89.0f);

        const glm::quat qYaw = glm::angleAxis(glm::radians(yawDeg), glm::vec3(0, 1, 0));
        const glm::vec3 right = qYaw * glm::vec3(1, 0, 0);
        const glm::quat qPitch = glm::angleAxis(glm::radians(pitchDeg), right);
        const glm::quat q = glm::normalize(qPitch * qYaw);

        if (auto parent = _ownerTransform->GetParent())
        {
            const glm::quat parentWorldRotation = parent->GetWorldRotation();
            const glm::quat localRotation = glm::normalize(glm::inverse(parentWorldRotation) * q);
            _ownerTransform->SetLocalRotation(localRotation);
        }
        else
        {
            _ownerTransform->SetLocalRotation(q);
        }
    }
    else
    {
        firstMouse = true;
    }
}
