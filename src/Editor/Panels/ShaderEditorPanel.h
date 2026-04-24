#pragma once

#ifndef QE_SHADER_EDITOR_PANEL_H
#define QE_SHADER_EDITOR_PANEL_H

#include "IEditorPanel.h"

#include <QEShaderAsset.h>

#include <filesystem>
#include <string>

class EditorContext;

class ShaderEditorPanel : public IEditorPanel
{
public:
    explicit ShaderEditorPanel(EditorContext* editorContext);

    const char* GetName() const override { return "Shader Editor"; }
    void Draw() override;

    bool OpenShaderFromFile(const std::filesystem::path& shaderPath);

private:
    void DrawToolbar();
    void DrawGeneralSection();
    void DrawStagesSection();
    void DrawGraphicsSection();
    void DrawShadowSection();
    bool DrawAssetPickerPopup(const char* popupId, std::string& value);

    bool Save();
    bool DrawStagePathField(const char* label, std::string& value);

private:
    EditorContext* editorContext = nullptr;
    QEShaderAsset _asset;
    std::filesystem::path _shaderPath;
    bool _isOpen = false;
    bool _isDirty = false;
};

#endif
