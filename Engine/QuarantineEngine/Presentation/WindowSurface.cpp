#include "WindowSurface.h"
#include <stdexcept>

void WindowSurface::createSurface(VkInstance &instance, GLFWwindow* window)
{
    VkWin32SurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hwnd = glfwGetWin32Window(window);
    createInfo.hinstance = GetModuleHandle(nullptr);

    if (vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

void WindowSurface::cleanUp(VkInstance &instance)
{
    vkDestroySurfaceKHR(instance, surface, nullptr);
}
