#pragma once

#ifndef DEBUG_CONTROLLER_H
#define DEBUG_CONTROLLER_H

#include <QEGameComponent.h>
#include <QEGameObject.h>
#include <imgui.h>
#include <Timer.h>

class DebugController : public QEGameComponent
{
    REFLECTABLE_DERIVED_COMPONENT(DebugController, QEGameComponent)
    float speed = 4.0f;     // m/s
    float sprint = 7.5f;    // m/s con Shift

    void QEUpdate() override {
        auto tr = Owner->GetComponent<QETransform>();
        if (!tr) return;

        float dt = Timer::DeltaTime;
        if (dt <= 0) return;

        // WASD en espacio mundo (Y up). Si quieres relativo a Yaw, cambia por rotación del personaje.
        glm::vec3 dir(0);
        if (ImGui::IsKeyDown(ImGuiKey_W)) dir += glm::vec3(0, 0, -1);
        if (ImGui::IsKeyDown(ImGuiKey_S)) dir += glm::vec3(0, 0, 1);
        if (ImGui::IsKeyDown(ImGuiKey_A)) dir += glm::vec3(-1, 0, 0);
        if (ImGui::IsKeyDown(ImGuiKey_D)) dir += glm::vec3(1, 0, 0);

        if (glm::length2(dir) > 0.f) {
            dir = glm::normalize(dir);
            float v = ImGui::IsKeyDown(ImGuiKey_ModShift) ? sprint : speed;
            tr->TranslateWorld(dir * v * dt);
        }
    }
};

#endif // !DEBUG_CONTROLLER_H
