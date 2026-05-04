#pragma once

#include <memory>
#include <string>

class QEGameObject;

enum class EditorSelectionType
{
    None = 0,
    GameObject,
    Atmosphere
};

class EditorSelectionManager
{
public:
    void SelectGameObject(const std::shared_ptr<QEGameObject>& gameObject);
    void SelectGameObjectById(const std::string& id);
    void SelectAtmosphere();
    void ClearSelection();

    std::shared_ptr<QEGameObject> GetSelectedGameObject() const;
    const std::string& GetSelectedGameObjectId() const;

    EditorSelectionType GetSelectionType() const;
    bool HasSelection() const;

    bool IsAtmosphereSelected() const;

    bool IsSelected(const std::shared_ptr<QEGameObject>& gameObject) const;
    bool IsSelected(const std::string& id) const;

private:
    EditorSelectionType selectionType = EditorSelectionType::None;
    std::string selectedGameObjectId;
};
