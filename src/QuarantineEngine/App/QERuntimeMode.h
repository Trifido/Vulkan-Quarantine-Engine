#pragma once

#include <QESingleton.h>

class QERuntimeMode : public QESingleton<QERuntimeMode>
{
private:
    friend class QESingleton<QERuntimeMode>;

public:
    void SetGameplayEnabled(bool value) { _gameplayEnabled = value; }
    bool IsGameplayEnabled() const { return _gameplayEnabled; }

private:
    bool _gameplayEnabled = true;
};

namespace QE
{
    using ::QERuntimeMode;
} // namespace QE
