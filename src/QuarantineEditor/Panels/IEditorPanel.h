#pragma once
#include <string>

class IEditorPanel
{
public:
    virtual ~IEditorPanel() = default;
    virtual const char* GetName() const = 0;
    virtual void Draw() = 0;
};
