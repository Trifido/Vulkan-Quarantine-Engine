#include "QEEditorRuntimeBridge.h"

#include <EditorObjectManager.h>
#include <Grid.h>

namespace QEEditorRuntimeBridge
{
    void SetupEditorRuntime(bool showGrid)
    {
        auto editorManager = EditorObjectManager::getInstance();
        if (!editorManager)
        {
            return;
        }

        auto existing = editorManager->GetObject("editor:grid");
        if (!existing)
        {
            auto grid = std::make_shared<Grid>();
            grid->IsRenderable = showGrid;
            editorManager->AddEditorObject(grid, "editor:grid");
            return;
        }

        existing->IsRenderable = showGrid;

        auto existingGrid = std::dynamic_pointer_cast<Grid>(existing);
        if (existingGrid)
        {
            existingGrid->EnsureResources();
        }
    }

    void SetEditorGridVisible(bool visible)
    {
        auto editorManager = EditorObjectManager::getInstance();
        if (!editorManager)
        {
            return;
        }

        auto grid = editorManager->GetObject("editor:grid");
        if (grid)
        {
            grid->IsRenderable = visible;
        }
    }

    void DrawEditorObjects(VkCommandBuffer& commandBuffer, uint32_t frameIndex)
    {
        auto editorManager = EditorObjectManager::getInstance();
        if (editorManager)
        {
            editorManager->DrawCommnad(commandBuffer, frameIndex);
        }
    }

    void CleanupEditorRuntime()
    {
        auto editorManager = EditorObjectManager::getInstance();
        if (!editorManager)
        {
            return;
        }

        editorManager->Cleanup();
        editorManager->CleanLastResources();
        editorManager->ResetInstance();
    }
}
