#pragma once

#include <Logging/IQELogSink.h>

class QEEditorConsole;

class QEEditorConsoleSink : public IQELogSink
{
public:
    explicit QEEditorConsoleSink(QEEditorConsole* console)
        : _console(console)
    {
    }

    void Write(const QELogEntry& entry) override;

private:
    QEEditorConsole* _console = nullptr;
};
