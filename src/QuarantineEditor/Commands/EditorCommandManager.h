#pragma once

#include <memory>
#include <vector>

#include "IEditorCommand.h"

class EditorCommandManager
{
public:
    void ExecuteCommand(std::unique_ptr<IEditorCommand> command);
    void PushExecutedCommand(std::unique_ptr<IEditorCommand> command);
    void Undo();
    void Redo();

    bool CanUndo() const;
    bool CanRedo() const;
    void Clear();

private:
    std::vector<std::unique_ptr<IEditorCommand>> undoStack;
    std::vector<std::unique_ptr<IEditorCommand>> redoStack;
};
