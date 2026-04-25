#pragma once

#include "IQELogSink.h"

class QEConsoleLogSink : public IQELogSink
{
public:
    void Write(const QELogEntry& entry) override;
};


namespace QE
{
    using ::QEConsoleLogSink;
} // namespace QE
// QE namespace aliases
