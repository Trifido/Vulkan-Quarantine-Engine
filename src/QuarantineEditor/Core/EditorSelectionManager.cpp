#include "EditorSelectionManager.h"

#include <GameObjectManager.h>
#include <QEGameObject.h>

void EditorSelectionManager::SelectGameObject(const std::shared_ptr<QEGameObject>& gameObject)
{
    selectionType = gameObject ? EditorSelectionType::GameObject : EditorSelectionType::None;
    selectedGameObjectId = gameObject ? gameObject->ID() : "";
}

void EditorSelectionManager::SelectGameObjectById(const std::string& id)
{
    selectionType = id.empty() ? EditorSelectionType::None : EditorSelectionType::GameObject;
    selectedGameObjectId = id;
}

void EditorSelectionManager::SelectAtmosphere()
{
    selectionType = EditorSelectionType::Atmosphere;
    selectedGameObjectId.clear();
}

void EditorSelectionManager::ClearSelection()
{
    selectionType = EditorSelectionType::None;
    selectedGameObjectId.clear();
}

std::shared_ptr<QEGameObject> EditorSelectionManager::GetSelectedGameObject() const
{
    if (selectionType != EditorSelectionType::GameObject)
        return nullptr;

    if (selectedGameObjectId.empty())
        return nullptr;

    return GameObjectManager::getInstance()->GetGameObjectById(selectedGameObjectId);
}

const std::string& EditorSelectionManager::GetSelectedGameObjectId() const
{
    return selectedGameObjectId;
}

EditorSelectionType EditorSelectionManager::GetSelectionType() const
{
    return selectionType;
}

bool EditorSelectionManager::HasSelection() const
{
    if (selectionType == EditorSelectionType::Atmosphere)
        return true;

    return selectionType == EditorSelectionType::GameObject && !selectedGameObjectId.empty();
}

bool EditorSelectionManager::IsAtmosphereSelected() const
{
    return selectionType == EditorSelectionType::Atmosphere;
}

bool EditorSelectionManager::IsSelected(const std::shared_ptr<QEGameObject>& gameObject) const
{
    if (!gameObject)
        return false;

    return IsSelected(gameObject->ID());
}

bool EditorSelectionManager::IsSelected(const std::string& id) const
{
    if (selectionType != EditorSelectionType::GameObject)
        return false;

    return !id.empty() && selectedGameObjectId == id;
}
