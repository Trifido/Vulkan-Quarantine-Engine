#pragma once

class EditorContext;
class QEDebugSystem;

class EditorDebugSettings
{
public:
    void AttachContext(EditorContext* context);
    void AttachDebugSystem(QEDebugSystem* debugSystem);

    bool ShowEditorGrid() const { return _showEditorGrid; }
    bool ShowColliderDebug() const { return _showColliderDebug; }
    bool ShowCullingAABBDebug() const { return _showCullingAABBDebug; }

    void SetShowEditorGrid(bool value);
    void SetShowColliderDebug(bool value);
    void SetShowCullingAABBDebug(bool value);

private:
    void SyncContext() const;

private:
    EditorContext* _context = nullptr;
    QEDebugSystem* _debugSystem = nullptr;
    bool _showEditorGrid = true;
    bool _showColliderDebug = false;
    bool _showCullingAABBDebug = false;
};
