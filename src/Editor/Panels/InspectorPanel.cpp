#include "InspectorPanel.h"

#include <imgui.h>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>

#include <cstring>
#include <string>
#include <typeindex>

#include <GameObjectManager.h>
#include <QEGameObject.h>
#include <QETransform.h>
#include <QEGameComponent.h>
#include <Reflectable.h>

#include <Editor/Core/EditorContext.h>
#include <Editor/Core/EditorSelectionManager.h>

#include <Editor/Commands/EditorCommandManager.h>
#include <Editor/Commands/TransformCommand.h>
#include <Editor/Commands/EditorTransformUtils.h>

namespace
{
    constexpr size_t INSPECTOR_TEXT_BUFFER_SIZE = 512;

    bool ShouldSkipField(const QEMetaField& field)
    {
        // Campos que no queremos mostrar en el inspector
        if (field.name == "id")
            return true;

        return false;
    }

    bool IsSupportedFieldType(const std::type_index& type)
    {
        return
            type == typeid(bool) ||
            type == typeid(int) ||
            type == typeid(unsigned int) ||
            type == typeid(uint32_t) ||
            type == typeid(float) ||
            type == typeid(double) ||
            type == typeid(char) ||
            type == typeid(std::string) ||
            type == typeid(glm::vec2) ||
            type == typeid(glm::vec3) ||
            type == typeid(glm::vec4);
    }

    void* GetFieldPtr(SerializableComponent* object, const QEMetaField& field)
    {
        return reinterpret_cast<void*>(reinterpret_cast<char*>(object) + field.offset);
    }

    std::string BuildWidgetLabel(const std::string& visibleName, const std::string& uniqueSuffix)
    {
        return visibleName + "##" + uniqueSuffix;
    }

    bool DrawStringField(const char* label, std::string& value)
    {
        char buffer[INSPECTOR_TEXT_BUFFER_SIZE];
        std::memset(buffer, 0, sizeof(buffer));
        std::strncpy(buffer, value.c_str(), sizeof(buffer) - 1);

        if (ImGui::InputText(label, buffer, sizeof(buffer)))
        {
            value = buffer;
            return true;
        }

        return false;
    }

    bool DrawCharField(const char* label, char& value)
    {
        char buffer[2] = { value, '\0' };

        if (ImGui::InputText(label, buffer, sizeof(buffer)))
        {
            value = buffer[0];
            return true;
        }

        return false;
    }

    bool DrawReflectedField(
        SerializableComponent* object,
        const QEMetaField& field,
        const std::string& widgetId)
    {
        if (!object)
            return false;

        if (ShouldSkipField(field))
            return false;

        if (!IsSupportedFieldType(field.type))
            return false;

        void* fieldPtr = GetFieldPtr(object, field);
        if (!fieldPtr)
            return false;

        const std::string label = BuildWidgetLabel(field.name, widgetId);

        if (field.type == typeid(bool))
        {
            return ImGui::Checkbox(label.c_str(), reinterpret_cast<bool*>(fieldPtr));
        }
        else if (field.type == typeid(int))
        {
            return ImGui::DragInt(label.c_str(), reinterpret_cast<int*>(fieldPtr), 1.0f);
        }
        else if (field.type == typeid(unsigned int))
        {
            return ImGui::DragScalar(label.c_str(), ImGuiDataType_U32, fieldPtr, 1.0f);
        }
        else if (field.type == typeid(uint32_t))
        {
            return ImGui::DragScalar(label.c_str(), ImGuiDataType_U32, fieldPtr, 1.0f);
        }
        else if (field.type == typeid(float))
        {
            return ImGui::DragFloat(label.c_str(), reinterpret_cast<float*>(fieldPtr), 0.1f);
        }
        else if (field.type == typeid(double))
        {
            double step = 0.1;
            return ImGui::InputScalar(label.c_str(), ImGuiDataType_Double, fieldPtr, &step);
        }
        else if (field.type == typeid(char))
        {
            return DrawCharField(label.c_str(), *reinterpret_cast<char*>(fieldPtr));
        }
        else if (field.type == typeid(std::string))
        {
            return DrawStringField(label.c_str(), *reinterpret_cast<std::string*>(fieldPtr));
        }
        else if (field.type == typeid(glm::vec2))
        {
            return ImGui::DragFloat2(label.c_str(), &reinterpret_cast<glm::vec2*>(fieldPtr)->x, 0.1f);
        }
        else if (field.type == typeid(glm::vec3))
        {
            return ImGui::DragFloat3(label.c_str(), &reinterpret_cast<glm::vec3*>(fieldPtr)->x, 0.1f);
        }
        else if (field.type == typeid(glm::vec4))
        {
            return ImGui::DragFloat4(label.c_str(), &reinterpret_cast<glm::vec4*>(fieldPtr)->x, 0.1f);
        }

        return false;
    }

    void DrawTransformInspector(
        const std::shared_ptr<QEGameObject>& gameObject,
        const std::shared_ptr<QETransform>& transform,
        EditorCommandManager* commandManager)
    {
        if (!gameObject || !transform)
            return;

        if (ImGui::CollapsingHeader("QETransform", ImGuiTreeNodeFlags_DefaultOpen))
        {
            glm::vec3 position = transform->localPosition;
            glm::vec3 rotation = glm::degrees(glm::eulerAngles(transform->localRotation));
            glm::vec3 scale = transform->localScale;

            if (ImGui::DragFloat3("Position", &position.x, 0.1f))
            {
                if (commandManager)
                {
                    TransformState before = EditorTransformUtils::CaptureState(transform);
                    TransformState after = before;
                    after.Position = position;

                    commandManager->ExecuteCommand(
                        std::make_unique<TransformCommand>(
                            gameObject->ID(),
                            before,
                            after));
                }
                else
                {
                    transform->SetLocalPosition(position);
                }
            }

            if (ImGui::DragFloat3("Rotation", &rotation.x, 0.5f))
            {
                if (commandManager)
                {
                    TransformState before = EditorTransformUtils::CaptureState(transform);
                    TransformState after = before;
                    after.Rotation = glm::quat(glm::radians(rotation));

                    commandManager->ExecuteCommand(
                        std::make_unique<TransformCommand>(
                            gameObject->ID(),
                            before,
                            after));
                }
                else
                {
                    transform->SetLocalEulerDegrees(rotation);
                }
            }

            if (ImGui::DragFloat3("Scale", &scale.x, 0.05f))
            {
                scale = glm::max(scale, glm::vec3(0.0001f));

                if (commandManager)
                {
                    TransformState before = EditorTransformUtils::CaptureState(transform);
                    TransformState after = before;
                    after.Scale = scale;

                    commandManager->ExecuteCommand(
                        std::make_unique<TransformCommand>(
                            gameObject->ID(),
                            before,
                            after));
                }
                else
                {
                    transform->SetLocalScale(scale);
                }
            }
        }

        ImGui::Separator();
    }

    void DrawGenericComponentInspector(
        QEGameComponent* component,
        const std::string& uniquePrefix)
    {
        if (!component)
            return;

        if (!component->IsSerializable())
            return;

        QEMetaType* meta = component->meta();
        if (!meta)
            return;

        const std::string headerLabel = component->getTypeName();

        if (ImGui::CollapsingHeader(headerLabel.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
        {
            const auto fields = meta->allFields();

            int drawnFields = 0;

            for (size_t i = 0; i < fields.size(); ++i)
            {
                const auto& field = fields[i];

                if (ShouldSkipField(field))
                    continue;

                if (!IsSupportedFieldType(field.type))
                    continue;

                const std::string widgetId =
                    uniquePrefix + "_" + component->getTypeName() + "_" + field.name + "_" + std::to_string(i);

                if (DrawReflectedField(component, field, widgetId))
                {
                    // De momento edición directa, sin command system genérico
                }

                ++drawnFields;
            }

            if (drawnFields == 0)
            {
                ImGui::TextUnformatted("No supported serializable fields.");
            }
        }

        ImGui::Separator();
    }
}

InspectorPanel::InspectorPanel(
    GameObjectManager* gameObjectManager,
    EditorContext* editorContext,
    EditorSelectionManager* selectionManager,
    EditorCommandManager* commandManager)
    : gameObjectManager(gameObjectManager)
    , editorContext(editorContext)
    , selectionManager(selectionManager)
    , commandManager(commandManager)
{
}

void InspectorPanel::Draw()
{
    if (!editorContext || !editorContext->ShowInspector)
    {
        return;
    }

    ImGui::Begin("Inspector", &editorContext->ShowInspector);

    if (!gameObjectManager)
    {
        ImGui::TextUnformatted("GameObjectManager is null.");
        ImGui::End();
        return;
    }

    if (!selectionManager || !selectionManager->HasSelection())
    {
        ImGui::TextUnformatted("No GameObject selected.");
        ImGui::End();
        return;
    }

    auto gameObject = selectionManager->GetSelectedGameObject();

    if (!gameObject)
    {
        ImGui::TextUnformatted("Selected GameObject no longer exists.");
        selectionManager->ClearSelection();
        ImGui::End();
        return;
    }

    ImGui::Text("Name: %s", gameObject->Name.c_str());
    ImGui::Text("ID: %s", gameObject->ID().c_str());
    ImGui::Text("Children: %d", static_cast<int>(gameObject->childs.size()));
    ImGui::Text("Components: %d", static_cast<int>(gameObject->components.size()));

    ImGui::Separator();

    int componentIndex = 0;
    for (auto& component : gameObject->components)
    {
        if (!component)
            continue;

        if (!component->IsSerializable())
            continue;

        if (auto transform = std::dynamic_pointer_cast<QETransform>(component))
        {
            DrawTransformInspector(gameObject, transform, commandManager);
        }
        else
        {
            const std::string uniquePrefix =
                "comp_" + gameObject->ID() + "_" + std::to_string(componentIndex);

            DrawGenericComponentInspector(component.get(), uniquePrefix);
        }

        ++componentIndex;
    }

    ImGui::End();
}
