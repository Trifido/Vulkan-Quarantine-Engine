
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include <stdio.h>          // printf, fprintf
#include <stdlib.h>         // abort
#include <windows.h>
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <vulkan/vulkan.hpp>

#include "App.h"
#include <QEProjectManager.h>
#include <stdio.h>
#define FULL_SCREEN false

#ifdef _WIN32
#include <windows.h>
#include <filesystem>
#include <iostream>
#endif

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

//#define IMGUI_UNLIMITED_FRAME_RATE
#ifdef _DEBUG
#define IMGUI_VULKAN_DEBUG_REPORT
#endif

static VkAllocationCallbacks*   g_Allocator = NULL;
static VkInstance               g_Instance = VK_NULL_HANDLE;
static VkPhysicalDevice         g_PhysicalDevice = VK_NULL_HANDLE;
static VkDevice                 g_Device = VK_NULL_HANDLE;
static uint32_t                 g_QueueFamily = (uint32_t)-1;
static VkQueue                  g_Queue = VK_NULL_HANDLE;
static VkDebugReportCallbackEXT g_DebugReport = VK_NULL_HANDLE;
static VkPipelineCache          g_PipelineCache = VK_NULL_HANDLE;
static VkDescriptorPool         g_DescriptorPool = VK_NULL_HANDLE;

static ImGui_ImplVulkanH_Window g_MainWindowData;
static int                      g_MinImageCount = 2;
static bool                     g_SwapChainRebuild = false;

int main(int, char**)
{
#ifdef _WIN32
    // 1) Buffer wide‐char para la ruta del exe
    wchar_t pathBuf[MAX_PATH];
    DWORD len = GetModuleFileNameW(nullptr, pathBuf, MAX_PATH);
    if (len == 0 || len == MAX_PATH)
    {
        std::cerr << "Error al resolver la ruta del exe\n";
        return -1;
    }

    // 2) Convertir a std::filesystem::path y extraer carpeta padre
    std::filesystem::path exePath(pathBuf);
    exePath = exePath.parent_path();

    // 3) Cambiar Working Directory a esa carpeta
    std::error_code ec;
    std::filesystem::current_path(exePath, ec);
    if (ec)
    {
        std::wcerr << L"No se pudo cambiar el Working Directory a '" << exePath.wstring() << L"': " << ec.message().c_str() << L"\n";
        return -1;
    }

    std::wcout << L"Working Directory fijado a: "
        << exePath.wstring() << L"\n\n";
#endif

    QEProjectManager::CreateQEProject("QEExample");
    QEScene scene{};
    QEProjectManager::InitializeDefaultQEScene(scene);
    //QEProjectManager::ImportMeshFile();
    App app;
    app.run(scene);
    return 0;
}
