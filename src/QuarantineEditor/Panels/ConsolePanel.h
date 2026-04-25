#pragma once

#include "IEditorPanel.h"
#include "Logging\QELogLevel.h"

class EditorContext;
class QEEditorConsole;

class ConsolePanel : public IEditorPanel
{
public:
    ConsolePanel(EditorContext* editorContext, QEEditorConsole* console);

    void Draw() override;
    const char* GetName() const override { return "Console"; }

private:
    bool PassesLevelFilter(QELogLevel level) const;
    const char* GetLevelText(QELogLevel level) const;

private:
    EditorContext* _editorContext = nullptr;
    QEEditorConsole* _console = nullptr;

    bool _showInfo = true;
    bool _showWarnings = true;
    bool _showErrors = true;
    bool _showDebug = true;

    char _filter[128] = "";
};
