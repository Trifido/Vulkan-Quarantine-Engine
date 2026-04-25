#pragma once

#include "IEditorPanel.h"

#include <filesystem>
#include <memory>

#include <Editor/Rendering/MaterialPreviewRenderer.h>
#include <TextureTypes.h>

class EditorContext;
class MaterialManager;
class DeviceModule;
class RenderPassModule;
class CommandPoolModule;
class QueueModule;
class QEMaterial;
class ShaderModule;

class MaterialEditorPanel : public IEditorPanel
{
public:
    MaterialEditorPanel(
        EditorContext* editorContext,
        MaterialManager* materialManager,
        DeviceModule* deviceModule,
        RenderPassModule* renderPassModule,
        CommandPoolModule* commandPoolModule,
        QueueModule* queueModule);
    ~MaterialEditorPanel() override;

    const char* GetName() const override { return "Material Editor"; }
    void Draw() override;

    void OpenMaterial(const std::shared_ptr<QEMaterial>& material);
    bool OpenMaterialFromFile(const std::filesystem::path& materialPath);
    void RebuildPreview();
    void RenderPreview(VkCommandBuffer& commandBuffer, uint32_t currentFrame);

private:
    void DrawToolbar();
    void DrawPreview();
    void HandlePreviewInteraction();
    void DrawProperties();
    void DrawTextureSlots();
    void DrawShaderReflection();
    bool DrawShaderPickerPopup(const char* popupId);
    bool DrawTexturePickerPopup(const char* popupId, TEXTURE_TYPE semantic);
    void AssignTexture(TEXTURE_TYPE semantic, const std::filesystem::path& texturePath);
    void SyncPreview();

private:
    EditorContext* editorContext = nullptr;
    MaterialManager* materialManager = nullptr;
    std::shared_ptr<QEMaterial> _material;
    std::shared_ptr<ShaderModule> _lastKnownShader = nullptr;
    std::filesystem::path _materialPath;
    MaterialPreviewRenderer _previewRenderer;
};
