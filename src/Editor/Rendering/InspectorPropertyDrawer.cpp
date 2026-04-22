#include "InspectorPropertyDrawer.h"

#include <imgui.h>
#include <cstring>
#include <typeindex>
#include <string>
#include <algorithm>

#include <Reflectable.h>
#include <glm/glm.hpp>
#include <PhysicsTypes.h>

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
            t == typeid(glm::vec4) ||
            t == typeid(PhysicBodyType) ||
            t == typeid(CollisionFlag);
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
        else if (t == typeid(PhysicBodyType))
        {
            static const char* items[] = { "Static", "Rigid", "Kinematic" };
            auto* value = reinterpret_cast<PhysicBodyType*>(fieldPtr);
            int current = *value == RIGID_BODY ? 1 : (*value == KINEMATIC_BODY ? 2 : 0);
            if (ImGui::Combo(label.c_str(), &current, items, IM_ARRAYSIZE(items)))
            {
                *value = current == 1 ? RIGID_BODY : (current == 2 ? KINEMATIC_BODY : STATIC_BODY);
                return true;
            }
            return false;
        }
        else if (t == typeid(CollisionFlag))
        {
            auto* value = reinterpret_cast<CollisionFlag*>(fieldPtr);

            if (field.name == "CollisionMask")
            {
                unsigned int maskValue = *value == COL_ALL ? QEPhysicsCollisionMaskAll() : static_cast<unsigned int>(*value);
                bool changed = false;

                if (ImGui::TreeNode(label.c_str()))
                {
                    struct Entry { const char* name; CollisionFlag flag; };
                    static const Entry entries[] = {
                        { "Default", COL_DEFAULT },
                        { "Player", COL_PLAYER },
                        { "Scene", COL_SCENE },
                        { "Enemy", COL_ENEMY },
                        { "Trigger", COL_TRIGGER }
                    };

                    for (const Entry& entry : entries)
                    {
                        bool enabled = (maskValue & static_cast<unsigned int>(entry.flag)) != 0;
                        if (ImGui::Checkbox(entry.name, &enabled))
                        {
                            if (enabled)
                                maskValue |= static_cast<unsigned int>(entry.flag);
                            else
                                maskValue &= ~static_cast<unsigned int>(entry.flag);
                            changed = true;
                        }
                    }

                    ImGui::TreePop();
                }

                if (changed)
                    *value = static_cast<CollisionFlag>(maskValue);

                return changed;
            }

            static const char* items[] = { "Scene", "Player", "Enemy", "Trigger" };
            int current = *value == COL_PLAYER ? 1 : (*value == COL_ENEMY ? 2 : (*value == COL_TRIGGER ? 3 : 0));
            if (ImGui::Combo(label.c_str(), &current, items, IM_ARRAYSIZE(items)))
            {
                *value = current == 1 ? COL_PLAYER : (current == 2 ? COL_ENEMY : (current == 3 ? COL_TRIGGER : COL_SCENE));
                return true;
            }
            return false;
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

