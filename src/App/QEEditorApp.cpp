#include "QEEditorApp.h"
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <SyncTool.h>
#include "../Editor/Grid.h"

void QEEditorApp::OnInitialize()
{
}

void QEEditorApp::OnShutdown()
{
}

void QEEditorApp::OnBeginFrame()
{
    BeginImGuiFrame();
    DrawEditorUI();
}

void QEEditorApp::OnEndFrame()
{
    EndImGuiFrame();
}

void QEEditorApp::OnPostInitVulkan()
{
    renderPassModule->CreateImGuiRenderPass(swapchainModule->swapChainImageFormat, *antialiasingModule->msaaSamples);
    InitializeImGui();
}

void QEEditorApp::OnPreCleanup()
{
    sessionManager->CleanEditorResources();
    ShutdownImGui();
}

void QEEditorApp::OnSwapchainRecreated()
{
    renderPassModule->CreateImGuiRenderPass(
        swapchainModule->swapChainImageFormat,
        *antialiasingModule->msaaSamples);

    // aquí seguramente luego necesitarás revalidar o recrear
    // cosas del backend/editor si tu integración lo exige
}

void QEEditorApp::BeginImGuiFrame()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void QEEditorApp::DrawEditorUI()
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false))
    {
        SaveScene();
    }

    // Aquí luego:
    // DrawDockspace();
    // DrawHierarchyPanel();
    // DrawInspectorPanel();

    ImGui::ShowDemoWindow();
}

void QEEditorApp::EndImGuiFrame()
{
    ImGui::Render();

    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

void QEEditorApp::InitializeImGui()
{
    //1: create descriptor pool for IMGUI
    // the size of the pool is very oversize, but it's copied from imgui demo itself.
    VkDescriptorPoolSize pool_sizes[] =
    {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000;
    pool_info.poolSizeCount = (uint32_t)std::size(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;

    vkCreateDescriptorPool(deviceModule->device, &pool_info, nullptr, &imguiPool);


    // 2: initialize imgui library

    //this initializes the core structures of imgui
    ImGui::CreateContext();

    //this initializes imgui for GLFW
    ImGui_ImplGlfw_InitForVulkan(mainWindow->window, true);

    //this initializes imgui for Vulkan
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = vulkanInstance.getInstance();
    init_info.PhysicalDevice = deviceModule->physicalDevice;
    init_info.Device = deviceModule->device;
    init_info.Queue = queueModule->graphicsQueue;
    init_info.DescriptorPool = imguiPool;
    init_info.Subpass = 0;                      // normalmente el primero
    init_info.RenderPass = *this->renderPassModule->ImGuiRenderPass;
    init_info.MinImageCount = 3;
    init_info.ImageCount = 3;
    init_info.MSAASamples = *deviceModule->getMsaaSamples();//VK_SAMPLE_COUNT_1_BIT;

    ImGui_ImplVulkan_Init(&init_info);

    //execute a gpu command to upload imgui font textures
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(deviceModule->device, commandPoolModule->getCommandPool());
    ImGui_ImplVulkan_CreateFontsTexture();
    endSingleTimeCommands(deviceModule->device, queueModule->graphicsQueue, commandPoolModule->getCommandPool(), commandBuffer);
}

void QEEditorApp::ShutdownImGui()
{
    ImGui_ImplVulkan_Shutdown();
    vkDestroyDescriptorPool(deviceModule->device, imguiPool, nullptr);
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void QEEditorApp::SaveScene()
{
    this->scene.cameraEditor = this->sessionManager->EditorCamera();
    this->scene.atmosphereDto = this->atmosphereSystem->CreateAtmosphereDto();
    this->scene.SerializeScene();
}
