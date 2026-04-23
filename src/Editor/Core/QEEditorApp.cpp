#include "QEEditorApp.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <ImGuizmo.h>
#include <SyncTool.h>

#include <Editor/Core/EditorContext.h>
#include <Editor/Core/EditorSelectionManager.h>
#include <Editor/Core/QEGizmoController.h>
#include <Editor/Core/EditorPickingSystem.h>
#include <Editor/Core/EditorSceneObjectFactory.h>
#include <QERenderTarget.h>

#include <Editor/Commands/EditorCommandManager.h>

#include "Panels/IEditorPanel.h"
#include "Panels/SceneHierarchyPanel.h"
#include "Panels/InspectorPanel.h"
#include "Panels/AnimationGraphPanel.h"
#include "Panels/ViewportPanel.h"
#include "Panels/EditorHeaderBar.h"
#include "Panels/QEProjectBrowserPanel.h"
#include "Panels/ConsolePanel.h"
#include "Panels/MaterialInspectorPanel.h"
#include "Panels/MaterialEditorPanel.h"
#include "Panels/QETextureViewerPanel.h"
#include "Rendering/EditorViewportResources.h"
#include <QEProjectManager.h>
#include <QEMeshRenderer.h>
#include <Material.h>
#include <algorithm>

#include <QECamera.h>
#include <QECameraController.h>

#include <Logging/QELogger.h>
#include <Logging/QEConsoleLogSink.h>
#include <Editor/Logs/QEEditorConsole.h>
#include <Editor/Logs/QEEditorConsoleSink.h>

namespace
{
    static constexpr const char* kEditorCameraPopupId = "EditorCameraSettingsPopup";
}

QEEditorApp::QEEditorApp()
{
}

QEEditorApp::~QEEditorApp() = default;

void QEEditorApp::OnInitialize()
{
    // Log
    editorConsole = std::make_unique<QEEditorConsole>();
    editorConsoleSink = std::make_unique<QEEditorConsoleSink>(editorConsole.get());
    consoleLogSink = std::make_unique<QEConsoleLogSink>();

    QELogger::Get().AddSink(consoleLogSink.get());
    QELogger::Get().AddSink(editorConsoleSink.get());

    // Main window & panels
    mainWindow->OnExternalFilesDropped = [this](const std::vector<std::filesystem::path>& paths)
        {
            for (const auto& path : paths)
            {
                QueueExternalDroppedFile(path);
            }
        };

    editorContext = std::make_unique<EditorContext>();

    viewportResources = std::make_unique<EditorViewportResources>();
    viewportResources->Initialize(deviceModule, renderPassModule, commandPoolModule, queueModule);
    viewportResources->Resize(1280, 720);

    SetAdditionalSceneRenderTarget();
    atmosphereSystem->UpdateAtmopshereResolution();

    selectionManager = std::make_unique<EditorSelectionManager>();
    gizmoController = std::make_unique<QEGizmoController>();
    gizmoController->SetOperation(QEGizmoController::Operation::Translate);
    pickingSystem = std::make_unique<EditorPickingSystem>();
    commandManager = std::make_unique<EditorCommandManager>();

    headerBar = std::make_unique<EditorHeaderBar>(
        editorContext.get(),
        commandManager.get(),
        sessionManager,
        physicsModule);

    headerBar->SetOnSaveRequested([this]()
        {
            SaveScene();
        });

    sceneObjectFactory = std::make_unique<EditorSceneObjectFactory>(
        gameObjectManager,
        MaterialManager::getInstance(),
        selectionManager.get(),
        sessionManager);

    CreatePanels();
}

void QEEditorApp::OnShutdown()
{
    if (editorConsoleSink)
    {
        QELogger::Get().RemoveSink(editorConsoleSink.get());
    }

    if (consoleLogSink)
    {
        QELogger::Get().RemoveSink(consoleLogSink.get());
    }

    if (viewportResources)
    {
        viewportResources->Cleanup();
        viewportResources.reset();
    }
}

void QEEditorApp::OnFrameStart()
{
    FlushClosedTextureViewerPanels();

    if (_pendingSceneOpenPath.has_value())
    {
        const auto scenePath = *_pendingSceneOpenPath;
        _pendingSceneOpenPath.reset();
        OpenScene(scenePath);
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
    physicsModule->SyncEditorBodies();
    physicsModule->UpdateDebugPhysicsDrawer();
}

void QEEditorApp::OnPostInitVulkan()
{
    InitializeImGui();
}

void QEEditorApp::OnPreCleanup()
{
    vkDeviceWaitIdle(deviceModule->device);
    if (sessionManager)
    {
        sessionManager->ExtraEditorPass = {};
    }
    panels.clear();

    sessionManager->CleanEditorResources();
    ShutdownImGui();
}

void QEEditorApp::OnSwapchainRecreated()
{
    if (viewportResources)
    {
        viewportResources->Rebuild();
        SetAdditionalSceneRenderTarget();
        atmosphereSystem->UpdateAtmopshereResolution();
    }

    if (materialEditorPanelPtr)
    {
        materialEditorPanelPtr->RebuildPreview();
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

    if (projectBrowserPanelPtr != nullptr)
    {
        projectBrowserPanelPtr->SetPendingExternalDrops(GetExternalDroppedFiles());
    }

    for (auto& panel : panels)
    {
        panel->Draw();
    }

    DrawTextureViewerPanels();

    if (projectBrowserPanelPtr != nullptr)
    {
        auto pendingSceneOpen = projectBrowserPanelPtr->ConsumePendingSceneOpenRequest();
        if (pendingSceneOpen.has_value())
        {
            _pendingSceneOpenPath = *pendingSceneOpen;
        }

        auto pendingTextureOpen = projectBrowserPanelPtr->ConsumePendingTextureOpenRequest();
        if (pendingTextureOpen.has_value())
        {
            OpenTextureViewer(*pendingTextureOpen);
        }

        auto pendingMaterialOpen = projectBrowserPanelPtr->ConsumePendingMaterialOpenRequest();
        if (pendingMaterialOpen.has_value())
        {
            OpenMaterialEditor(*pendingMaterialOpen);
        }
    }

    UpdateEditorCameraInputState();

    ClearExternalDroppedFiles();

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

void QEEditorApp::SetAdditionalSceneRenderTarget()
{
    if (!viewportResources || !viewportResources->IsValid())
    {
        return;
    }

    auto sessionManager = QESessionManager::getInstance();
    sessionManager->ExtraRenderTarget = &viewportResources->GetRenderTarget();
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
    panels.clear();

    panels.emplace_back(std::make_unique<SceneHierarchyPanel>(
        gameObjectManager,
        editorContext.get(),
        selectionManager.get(),
        sceneObjectFactory.get()));

    panels.emplace_back(std::make_unique<InspectorPanel>(
        gameObjectManager,
        editorContext.get(),
        selectionManager.get(),
        commandManager.get()));

    panels.emplace_back(std::make_unique<AnimationGraphPanel>(
        editorContext.get(),
        selectionManager.get()));

    panels.emplace_back(std::make_unique<MaterialInspectorPanel>(
        gameObjectManager,
        editorContext.get(),
        selectionManager.get(),
        [this](const std::shared_ptr<QEMaterial>& material)
        {
            OpenMaterialEditor(material);
        }));

    auto materialEditorPanelLocal = std::make_unique<MaterialEditorPanel>(
        editorContext.get(),
        MaterialManager::getInstance(),
        deviceModule,
        renderPassModule,
        commandPoolModule,
        queueModule);

    materialEditorPanelPtr = materialEditorPanelLocal.get();
    panels.emplace_back(std::move(materialEditorPanelLocal));

    panels.emplace_back(std::make_unique<ConsolePanel>(
        editorContext.get(),
        editorConsole.get()));

    auto viewportPanelLocal = std::make_unique<ViewportPanel>(
        editorContext.get(),
        viewportResources.get(),
        selectionManager.get(),
        gizmoController.get(),
        pickingSystem.get(),
        commandManager.get());

    viewportPanelLocal->OnAssetDroppedInViewport = [this](const std::string& assetPath)
        {
            SpawnDroppedMesh(assetPath);
        };

    panels.emplace_back(std::move(viewportPanelLocal));

    auto projectBrowserPanelLocal = std::make_unique<QEProjectBrowserPanel>();

    if (QEProjectManager::HasCurrentProject())
    {
        projectBrowserPanelLocal->SetProjectRootPath(QEProjectManager::GetCurrentProjectPath());
    }

    projectBrowserPanelLocal->InitializeIcons();

    projectBrowserPanelPtr = projectBrowserPanelLocal.get();
    panels.emplace_back(std::move(projectBrowserPanelLocal));

    if (sessionManager && materialEditorPanelPtr)
    {
        sessionManager->ExtraEditorPass =
            [this](VkCommandBuffer& commandBuffer, uint32_t currentFrame)
            {
                if (materialEditorPanelPtr)
                {
                    materialEditorPanelPtr->RenderPreview(commandBuffer, currentFrame);
                }
            };
    }
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

    if (headerBar)
    {
        headerBar->Draw();
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

    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z, false))
    {
        if (commandManager)
            commandManager->Undo();
    }

    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y, false))
    {
        if (commandManager)
            commandManager->Redo();
    }
}

void QEEditorApp::SaveScene()
{
    scene.cameraEditor = sessionManager->EditorCamera();
    scene.atmosphereDto = atmosphereSystem->CreateAtmosphereDto();
    scene.physicsGravity = physicsModule->GetGravity();
    scene.SerializeScene();
}

void QEEditorApp::OpenScene(const std::filesystem::path& scenePath)
{
    if (scenePath.empty())
        return;

    try
    {
        if (selectionManager)
        {
            selectionManager->ClearSelection();
        }

        if (commandManager)
        {
            commandManager->Clear();
        }

        LoadSceneFromPath(scenePath);

        if (projectBrowserPanelPtr != nullptr)
        {
            projectBrowserPanelPtr->SetProjectRootPath(QEProjectManager::GetCurrentProjectPath());
        }
    }
    catch (const std::exception& e)
    {
        QE_LOG_ERROR_CAT_F("QEEditorApp", "Failed to open scene: {}", e.what());
    }
}

void QEEditorApp::SpawnDroppedMesh(const std::string& assetPath)
{
    if (!sceneObjectFactory)
        return;

    sceneObjectFactory->CreateDroppedMeshObject(assetPath, 5.0f);
}

glm::vec3 QEEditorApp::GetSpawnPositionInFrontOfEditorCamera(float distance) const
{
    auto editorCamera = sessionManager->EditorCamera();
    if (!editorCamera)
        return glm::vec3(0.0f);

    auto cameraOwner = editorCamera->Owner;
    if (!cameraOwner)
        return glm::vec3(0.0f);

    auto cameraTransform = cameraOwner->GetComponent<QETransform>();
    if (!cameraTransform)
        return glm::vec3(0.0f);

    const glm::vec3 cameraPos = cameraTransform->GetWorldPosition();
    const glm::vec3 forward = glm::normalize(cameraTransform->Forward());

    return cameraPos + forward * distance;
}

void QEEditorApp::UpdateEditorCameraInputState()
{
    auto editorCamera = sessionManager->EditorCamera();
    if (!editorCamera || !editorCamera->Owner)
        return;

    auto controller = editorCamera->Owner->GetComponent<QECameraController>();
    if (!controller)
        return;

    const bool popupOpen =
        ImGui::IsPopupOpen("AddComponentPopup", ImGuiPopupFlags_AnyPopupId) ||
        ImGui::IsPopupOpen("Rename GameObject", ImGuiPopupFlags_AnyPopupId);

    const bool allowInput =
        editorContext &&
        !popupOpen &&
        !editorContext->BlockViewportInput &&
        (editorContext->ViewportFocused || editorContext->ViewportImageHovered);

    controller->SetInputEnabled(allowInput);
}

void QEEditorApp::FlushClosedTextureViewerPanels()
{
    if (_textureViewerPanelsPendingDestroy.empty())
        return;

    if (deviceModule && deviceModule->device != VK_NULL_HANDLE)
    {
        vkDeviceWaitIdle(deviceModule->device);
    }

    _textureViewerPanelsPendingDestroy.clear();
}

void QEEditorApp::QueueExternalDroppedFile(const std::filesystem::path& path)
{
    _externalDroppedFiles.push_back(path);
}

const std::vector<std::filesystem::path>& QEEditorApp::GetExternalDroppedFiles() const
{
    return _externalDroppedFiles;
}

void QEEditorApp::ClearExternalDroppedFiles()
{
    _externalDroppedFiles.clear();
}

void QEEditorApp::OpenTextureViewer(const std::filesystem::path& texturePath)
{
    auto it = std::find_if(
        _textureViewerPanels.begin(),
        _textureViewerPanels.end(),
        [&](const std::unique_ptr<QETextureViewerPanel>& panel)
        {
            return panel && panel->GetTexturePath() == texturePath;
        });

    if (it != _textureViewerPanels.end())
        return;

    _textureViewerPanels.push_back(std::make_unique<QETextureViewerPanel>(texturePath));
}

void QEEditorApp::OpenMaterialEditor(const std::filesystem::path& materialPath)
{
    if (!materialEditorPanelPtr)
        return;

    materialEditorPanelPtr->OpenMaterialFromFile(materialPath);
}

void QEEditorApp::OpenMaterialEditor(const std::shared_ptr<QEMaterial>& material)
{
    if (!materialEditorPanelPtr)
        return;

    materialEditorPanelPtr->OpenMaterial(material);
}

void QEEditorApp::DrawTextureViewerPanels()
{
    for (auto it = _textureViewerPanels.begin(); it != _textureViewerPanels.end();)
    {
        if (!(*it))
        {
            it = _textureViewerPanels.erase(it);
            continue;
        }

        (*it)->Draw();

        if (!(*it)->IsOpen())
        {
            _textureViewerPanelsPendingDestroy.push_back(std::move(*it));
            it = _textureViewerPanels.erase(it);
        }
        else
        {
            ++it;
        }
    }
}
