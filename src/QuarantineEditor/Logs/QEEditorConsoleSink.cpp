#include "QEEditorConsoleSink.h"
#include "QEEditorConsole.h"

void QEEditorConsoleSink::Write(const QELogEntry& entry)
{
    if (_console)
    {
        _console->AddLog(entry);
    }
}
