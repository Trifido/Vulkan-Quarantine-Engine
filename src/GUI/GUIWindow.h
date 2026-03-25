#pragma once

// Define WIN32_LEAN_AND_MEAN and NOMINMAX before including windows.h
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

// Undefine problematic macros that might have been defined by windows.h
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include <string>
#include <GLFW/glfw3.h>
#include "imgui.h"

#include "DeviceModule.h"
#include "QueueModule.h"

#include <functional>
#include <filesystem>
#include <vector>

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
    std::function<void(const std::vector<std::filesystem::path>&)> OnExternalFilesDropped;

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
    static void dropCallback(GLFWwindow* window, int count, const char** paths);
};

