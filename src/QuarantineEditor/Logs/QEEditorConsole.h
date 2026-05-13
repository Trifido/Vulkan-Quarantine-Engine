#pragma once

#include <vector>
#include <mutex>
#include <Logging/QELogLevel.h>

class QEEditorConsole
{
public:
    void AddLog(const QELogEntry& entry);
    void Clear();
    std::vector<QELogEntry> GetEntriesSnapshot() const;

public:
    bool AutoScroll = true;

private:
    mutable std::mutex _mutex;
    std::vector<QELogEntry> _entries;
};
