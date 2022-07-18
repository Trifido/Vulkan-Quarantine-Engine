#pragma once

#include <windows.h>
#include <string>
#include <GLFW/glfw3.h>
#include "imgui.h"

#include "DeviceModule.h"
#include "QueueModule.h"
#include "CommandPoolModule.h"

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

