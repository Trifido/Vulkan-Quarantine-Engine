#pragma once

#include <Logging/QELogLevel.h>

class IQELogSink
{
public:
    virtual ~IQELogSink() = default;
    virtual void Write(const QELogEntry& entry) = 0;
};
