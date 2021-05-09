#pragma once
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

class WindowSurface
{
private:
    VkSurfaceKHR surface;

public:
    void createSurface(VkInstance &instance, GLFWwindow* window);
    void cleanUp(VkInstance &instance);
    VkSurfaceKHR &getSurface() { return surface; }
};

