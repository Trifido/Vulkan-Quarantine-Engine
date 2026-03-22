#include "EditorSelectionManager.h"

#include <GameObjectManager.h>
#include <QEGameObject.h>

void EditorSelectionManager::SelectGameObject(const std::shared_ptr<QEGameObject>& gameObject)
{
    selectedGameObjectId = gameObject ? gameObject->ID() : "";
}

void EditorSelectionManager::SelectGameObjectById(const std::string& id)
{
    selectedGameObjectId = id;
}

void EditorSelectionManager::ClearSelection()
{
    selectedGameObjectId.clear();
}

std::shared_ptr<QEGameObject> EditorSelectionManager::GetSelectedGameObject() const
{
    if (selectedGameObjectId.empty())
        return nullptr;

    return GameObjectManager::getInstance()->GetGameObjectById(selectedGameObjectId);
}

const std::string& EditorSelectionManager::GetSelectedGameObjectId() const
{
    return selectedGameObjectId;
}

bool EditorSelectionManager::HasSelection() const
{
    return !selectedGameObjectId.empty();
}

bool EditorSelectionManager::IsSelected(const std::shared_ptr<QEGameObject>& gameObject) const
{
    if (!gameObject)
        return false;

    return IsSelected(gameObject->ID());
}

bool EditorSelectionManager::IsSelected(const std::string& id) const
{
    return !id.empty() && selectedGameObjectId == id;
}
