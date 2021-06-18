#pragma once

#include <string>
#include <GLFW/glfw3.h>
#include "imgui.h"

#include "DeviceModule.h"
#include "QueueModule.h"
#include "CommandPoolModule.h"

static void ShowDockingDisabledMessage()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui::Text("ERROR: Docking is not enabled! See Demo > Configuration.");
    ImGui::Text("Set io.ConfigFlags |= ImGuiConfigFlags_DockingEnable in your code, or ");
    ImGui::SameLine(0.0f, 0.0f);
    if (ImGui::SmallButton("click here"))
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
}

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

class GUIWindow
{
private:
    int width, height;
    std::string title;
    bool isRunning;
    GLFWmonitor* monitor;

public:
    GLFWwindow* window;

public:
    GUIWindow();
    bool init(bool fullScreen = false);
    void renderGUIWindow();
    void renderMainWindow();
    GLFWwindow* getWindow();
    void checkMinimize();
    void setupNewFrame();
private:
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
    void setupImgui();
};

