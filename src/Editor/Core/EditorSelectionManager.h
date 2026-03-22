#pragma once

#include <memory>
#include <string>

class QEGameObject;

class EditorSelectionManager
{
public:
    void SelectGameObject(const std::shared_ptr<QEGameObject>& gameObject);
    void SelectGameObjectById(const std::string& id);
    void ClearSelection();

    std::shared_ptr<QEGameObject> GetSelectedGameObject() const;
    const std::string& GetSelectedGameObjectId() const;

    bool HasSelection() const;
    bool IsSelected(const std::shared_ptr<QEGameObject>& gameObject) const;
    bool IsSelected(const std::string& id) const;

private:
    std::string selectedGameObjectId;
};
