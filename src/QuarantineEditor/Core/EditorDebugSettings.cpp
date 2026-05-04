#include "EditorDebugSettings.h"

#include <QuarantineEditor/Core/EditorContext.h>
#include <QuarantineEditor/Runtime/QEEditorRuntimeBridge.h>
#include <CullingSceneManager.h>
#include <DebugSystem/QEDebugSystem.h>

void EditorDebugSettings::AttachContext(EditorContext* context)
{
    _context = context;
    SyncContext();
}

void EditorDebugSettings::AttachDebugSystem(QEDebugSystem* debugSystem)
{
    _debugSystem = debugSystem;
    if (_debugSystem)
    {
        _debugSystem->SetEnabled(_showColliderDebug);
    }
}

void EditorDebugSettings::SetShowEditorGrid(bool value)
{
    _showEditorGrid = value;
    SyncContext();
    QEEditorRuntimeBridge::SetEditorGridVisible(value);
}

void EditorDebugSettings::SetShowColliderDebug(bool value)
{
    _showColliderDebug = value;
    SyncContext();

    if (_debugSystem)
    {
        _debugSystem->SetEnabled(value);
    }
}

void EditorDebugSettings::SetShowCullingAABBDebug(bool value)
{
    _showCullingAABBDebug = value;
    SyncContext();

    auto cullingSceneManager = CullingSceneManager::getInstance();
    if (cullingSceneManager)
    {
        cullingSceneManager->EnsureInitialized();
        cullingSceneManager->DebugMode = value;
    }
}

void EditorDebugSettings::SyncContext() const
{
    if (!_context)
        return;

    _context->ShowEditorGrid = _showEditorGrid;
    _context->ShowColliderDebug = _showColliderDebug;
    _context->ShowCullingAABBDebug = _showCullingAABBDebug;
}
