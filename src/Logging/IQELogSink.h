#pragma once

#include <QELogLevel.h>

class IQELogSink
{
public:
    virtual ~IQELogSink() = default;
    virtual void Write(const QELogEntry& entry) = 0;
};
