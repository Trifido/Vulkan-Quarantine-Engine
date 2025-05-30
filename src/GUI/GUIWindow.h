#pragma once

#include <windows.h>
#include <string>
#include <GLFW/glfw3.h>
#include "imgui.h"

#include "DeviceModule.h"
#include "QueueModule.h"

class GUIWindow : public QESingleton<GUIWindow>
{
private:
    friend class QESingleton<GUIWindow>;

    std::string title;
    bool isRunning;
    GLFWmonitor* monitor;

public:
    GLFWwindow* window;
    int width, height;

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

