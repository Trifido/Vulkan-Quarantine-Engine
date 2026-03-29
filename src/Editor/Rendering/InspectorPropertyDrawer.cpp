#include "InspectorPropertyDrawer.h"

#include <imgui.h>
#include <cstring>
#include <typeindex>
#include <string>
#include <algorithm>

#include <Reflectable.h>
#include <glm/glm.hpp>

namespace
{
    constexpr size_t INSPECTOR_TEXT_BUFFER_SIZE = 512;

    std::string BuildLabel(const std::string& visibleName, const std::string& uniqueId)
    {
        return visibleName + "##" + uniqueId;
    }

    void* GetFieldPtr(SerializableComponent* object, const QEMetaField& field)
    {
        return reinterpret_cast<void*>(reinterpret_cast<char*>(object) + field.offset);
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
}

namespace InspectorPropertyDrawer
{
    bool IsSupportedInspectorType(const QEMetaField& field)
    {
        const auto& t = field.type;

        return
            t == typeid(bool) ||
            t == typeid(int) ||
            t == typeid(unsigned int) ||
            t == typeid(uint32_t) ||
            t == typeid(float) ||
            t == typeid(double) ||
            t == typeid(char) ||
            t == typeid(std::string) ||
            t == typeid(glm::vec2) ||
            t == typeid(glm::vec3) ||
            t == typeid(glm::vec4);
    }

    bool DrawField(
        SerializableComponent* object,
        const QEMetaField& field,
        const std::string& widgetId)
    {
        if (!object)
            return false;

        if (!IsSupportedInspectorType(field))
            return false;

        void* fieldPtr = GetFieldPtr(object, field);
        if (!fieldPtr)
            return false;

        const std::string label = BuildLabel(field.name, widgetId);
        const auto& t = field.type;

        if (t == typeid(bool))
        {
            return ImGui::Checkbox(label.c_str(), reinterpret_cast<bool*>(fieldPtr));
        }
        else if (t == typeid(int))
        {
            return ImGui::DragInt(label.c_str(), reinterpret_cast<int*>(fieldPtr), 1.0f);
        }
        else if (t == typeid(unsigned int))
        {
            return ImGui::DragScalar(label.c_str(), ImGuiDataType_U32, fieldPtr, 1.0f);
        }
        else if (t == typeid(uint32_t))
        {
            return ImGui::DragScalar(label.c_str(), ImGuiDataType_U32, fieldPtr, 1.0f);
        }
        else if (t == typeid(float))
        {
            return ImGui::DragFloat(label.c_str(), reinterpret_cast<float*>(fieldPtr), 0.1f);
        }
        else if (t == typeid(double))
        {
            double step = 0.1;
            return ImGui::InputScalar(label.c_str(), ImGuiDataType_Double, fieldPtr, &step);
        }
        else if (t == typeid(char))
        {
            return DrawCharField(label.c_str(), *reinterpret_cast<char*>(fieldPtr));
        }
        else if (t == typeid(std::string))
        {
            return DrawStringField(label.c_str(), *reinterpret_cast<std::string*>(fieldPtr));
        }
        else if (t == typeid(glm::vec2))
        {
            return ImGui::DragFloat2(label.c_str(), &reinterpret_cast<glm::vec2*>(fieldPtr)->x, 0.1f);
        }
        else if (t == typeid(glm::vec3))
        {
            return ImGui::DragFloat3(label.c_str(), &reinterpret_cast<glm::vec3*>(fieldPtr)->x, 0.1f);
        }
        else if (t == typeid(glm::vec4))
        {
            return ImGui::DragFloat4(label.c_str(), &reinterpret_cast<glm::vec4*>(fieldPtr)->x, 0.1f);
        }

        return false;
    }

    bool DrawSerializableObject(
        SerializableComponent* object,
        const std::string& headerLabel,
        const std::string& idPrefix)
    {
        if (!object)
            return false;

        QEMetaType* meta = object->meta();
        if (!meta)
            return false;

        bool changed = false;

        if (ImGui::CollapsingHeader(headerLabel.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Indent();

            const auto fields = meta->allFields();
            for (size_t i = 0; i < fields.size(); ++i)
            {
                const auto& field = fields[i];

                if (!IsSupportedInspectorType(field))
                    continue;

                const std::string widgetId =
                    idPrefix + "_" + object->getTypeName() + "_" + field.name + "_" + std::to_string(i);

                changed |= DrawField(object, field, widgetId);
            }

            ImGui::Unindent();
        }

        return changed;
    }
}
