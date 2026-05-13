#include "QEEditorConsole.h"

void QEEditorConsole::AddLog(const QELogEntry& entry)
{
    std::scoped_lock lock(_mutex);
    _entries.push_back(entry);
}

void QEEditorConsole::Clear()
{
    std::scoped_lock lock(_mutex);
    _entries.clear();
}

std::vector<QELogEntry> QEEditorConsole::GetEntriesSnapshot() const
{
    std::scoped_lock lock(_mutex);
    return _entries;
}
