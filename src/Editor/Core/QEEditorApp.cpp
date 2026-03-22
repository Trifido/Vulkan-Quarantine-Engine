#include "QEEditorApp.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <ImGuizmo.h>
#include <SyncTool.h>

#include "Core/EditorSelectionManager.h"
#include "Core/QEGizmoController.h"
#include "Panels/IEditorPanel.h"
#include "Panels/SceneHierarchyPanel.h"
#include "Panels/InspectorPanel.h"
#include "Panels/ViewportPanel.h"
#include "Rendering/EditorViewportResources.h"

QEEditorApp::~QEEditorApp() = default;

void QEEditorApp::OnInitialize()
{
    editorContext = std::make_unique<EditorContext>();
    panels = std::make_unique<std::vector<std::unique_ptr<IEditorPanel>>>();

    viewportResources = std::make_unique<EditorViewportResources>();
    viewportResources->Initialize(deviceModule, renderPassModule, commandPoolModule, queueModule);
    viewportResources->Resize(1280, 720);

    selectionManager = std::make_unique<EditorSelectionManager>();
    gizmoController = std::make_unique<QEGizmoController>();
    gizmoController->SetOperation(QEGizmoController::Operation::Translate);

    CreatePanels();
}

void QEEditorApp::OnShutdown()
{
    if (viewportResources)
    {
        viewportResources->Cleanup();
        viewportResources.reset();
    }
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
    InitializeImGui();
}

void QEEditorApp::OnPreCleanup()
{
    sessionManager->CleanEditorResources();
    ShutdownImGui();
}

void QEEditorApp::OnSwapchainRecreated()
{
    if (viewportResources)
    {
        viewportResources->Rebuild();
    }
}

void QEEditorApp::BeginImGuiFrame()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();
}

void QEEditorApp::DrawEditorUI()
{
    HandleShortcuts();
    DrawDockspace();

    for (auto& panel : *panels)
    {
        panel->Draw();
    }

    if (editorContext && editorContext->ShowDemoWindow)
    {
        ImGui::ShowDemoWindow(&editorContext->ShowDemoWindow);
    }
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

const QERenderTarget* QEEditorApp::GetAdditionalSceneRenderTarget() const
{
    if (!viewportResources || !viewportResources->IsValid())
    {
        return nullptr;
    }

    return &viewportResources->GetRenderTarget();
}

void QEEditorApp::InitializeImGui()
{
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

    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui_ImplGlfw_InitForVulkan(mainWindow->window, true);

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = vulkanInstance.getInstance();
    init_info.PhysicalDevice = deviceModule->physicalDevice;
    init_info.Device = deviceModule->device;
    init_info.Queue = queueModule->graphicsQueue;
    init_info.DescriptorPool = imguiPool;
    init_info.Subpass = 0;
    init_info.RenderPass = *renderPassModule->DefaultRenderPass;
    init_info.MinImageCount = 3;
    init_info.ImageCount = 3;
    init_info.MSAASamples = *deviceModule->getMsaaSamples();

    ImGui_ImplVulkan_Init(&init_info);

    VkCommandBuffer commandBuffer = beginSingleTimeCommands(
        deviceModule->device,
        commandPoolModule->getCommandPool());

    ImGui_ImplVulkan_CreateFontsTexture();

    endSingleTimeCommands(
        deviceModule->device,
        queueModule->graphicsQueue,
        commandPoolModule->getCommandPool(),
        commandBuffer);
}

void QEEditorApp::ShutdownImGui()
{
    ImGui_ImplVulkan_Shutdown();
    vkDestroyDescriptorPool(deviceModule->device, imguiPool, nullptr);
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void QEEditorApp::CreatePanels()
{
    panels->clear();

    panels->emplace_back(std::make_unique<SceneHierarchyPanel>(
        gameObjectManager,
        editorContext.get(),
        selectionManager.get()));

    panels->emplace_back(std::make_unique<InspectorPanel>(
        gameObjectManager,
        editorContext.get(),
        selectionManager.get()));

    panels->emplace_back(std::make_unique<ViewportPanel>(
        editorContext.get(),
        viewportResources.get(),
        selectionManager.get(),
        gizmoController.get()));
}

void QEEditorApp::DrawDockspace()
{
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    window_flags |= ImGuiWindowFlags_NoTitleBar;
    window_flags |= ImGuiWindowFlags_NoCollapse;
    window_flags |= ImGuiWindowFlags_NoResize;
    window_flags |= ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
    window_flags |= ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    ImGui::Begin("MainDockspace", nullptr, window_flags);

    ImGui::PopStyleVar(2);

    ImGuiID dockspace_id = ImGui::GetID("QE_MainDockspace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Save", "Ctrl+S"))
            {
                SaveScene();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Window"))
        {
            ImGui::MenuItem("Hierarchy", nullptr, &editorContext->ShowHierarchy);
            ImGui::MenuItem("Inspector", nullptr, &editorContext->ShowInspector);
            ImGui::MenuItem("Viewport", nullptr, &editorContext->ShowViewport);
            ImGui::MenuItem("Console", nullptr, &editorContext->ShowConsole);
            ImGui::MenuItem("Content Browser", nullptr, &editorContext->ShowContentBrowser);
            ImGui::MenuItem("ImGui Demo", nullptr, &editorContext->ShowDemoWindow);
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }

    ImGui::End();
}

void QEEditorApp::HandleShortcuts()
{
    ImGuiIO& io = ImGui::GetIO();

    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false))
    {
        SaveScene();
    }
}

void QEEditorApp::SaveScene()
{
    scene.cameraEditor = sessionManager->EditorCamera();
    scene.atmosphereDto = atmosphereSystem->CreateAtmosphereDto();
    scene.SerializeScene();
}
