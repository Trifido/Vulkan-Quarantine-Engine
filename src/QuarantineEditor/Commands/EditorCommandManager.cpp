#include "EditorCommandManager.h"

void EditorCommandManager::ExecuteCommand(std::unique_ptr<IEditorCommand> command)
{
    if (!command)
        return;

    command->Execute();
    undoStack.emplace_back(std::move(command));
    redoStack.clear();
}

void EditorCommandManager::Undo()
{
    if (undoStack.empty())
        return;

    std::unique_ptr<IEditorCommand> command = std::move(undoStack.back());
    undoStack.pop_back();

    command->Undo();
    redoStack.emplace_back(std::move(command));
}

void EditorCommandManager::Redo()
{
    if (redoStack.empty())
        return;

    std::unique_ptr<IEditorCommand> command = std::move(redoStack.back());
    redoStack.pop_back();

    command->Execute();
    undoStack.emplace_back(std::move(command));
}

bool EditorCommandManager::CanUndo() const
{
    return !undoStack.empty();
}

bool EditorCommandManager::CanRedo() const
{
    return !redoStack.empty();
}

void EditorCommandManager::Clear()
{
    undoStack.clear();
    redoStack.clear();
}

void EditorCommandManager::PushExecutedCommand(std::unique_ptr<IEditorCommand> command)
{
    if (!command)
        return;

    undoStack.emplace_back(std::move(command));
    redoStack.clear();
}
