#include "InspectorPanel.h"

#include <imgui.h>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>

#include <cstring>
#include <string>
#include <typeindex>

#include <algorithm>
#include <cctype>
#include <unordered_map>
#include <unordered_set>

#include <GameObjectManager.h>
#include <QEGameObject.h>
#include <QETransform.h>
#include <QEGameComponent.h>
#include <AtmosphereSystem.h>
#include <PhysicsTypes.h>
#include <Reflectable.h>
#include <Light.h>
#include <LightType.h>

#include <QuarantineEditor/Core/EditorContext.h>
#include <QuarantineEditor/Core/EditorSelectionManager.h>

#include <QuarantineEditor/Commands/EditorCommandManager.h>
#include <QuarantineEditor/Commands/TransformCommand.h>
#include <QuarantineEditor/Commands/EditorTransformUtils.h>

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

    bool ShouldSkipLightField(QELight* light, const QEMetaField& field)
    {
        if (!light)
            return false;

        if (field.name == "lightType" || field.name == "idxShadowMap")
            return true;

        switch (light->lightType)
        {
        case LightType::POINT_LIGHT:
            return field.name == "cutOff" ||
                field.name == "outerCutoff";

        case LightType::SPOT_LIGHT:
            return false;

        case LightType::DIRECTIONAL_LIGHT:
        case LightType::SUN_LIGHT:
            return field.name == "constant" ||
                field.name == "linear" ||
                field.name == "quadratic" ||
                field.name == "radius" ||
                field.name == "cutOff" ||
                field.name == "outerCutoff";

        default:
            return false;
        }
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
            type == typeid(glm::vec4) ||
            type == typeid(PhysicBodyType) ||
            type == typeid(CollisionFlag);
    }

    bool DrawCollisionMaskEditor(const char* label, CollisionFlag& value)
    {
        unsigned int maskValue = value == COL_ALL ? QEPhysicsCollisionMaskAll() : static_cast<unsigned int>(value);
        bool changed = false;

        if (ImGui::TreeNode(label))
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
            value = static_cast<CollisionFlag>(maskValue);

        return changed;
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

    bool DrawLightSpecificField(QELight* light, const QEMetaField& field, const std::string& label, bool& handled)
    {
        handled = false;

        if (!light)
            return false;

        if (field.name == "cutOff" && light->lightType == LightType::SPOT_LIGHT)
        {
            handled = true;
            float innerAngle = glm::degrees(glm::acos(glm::clamp(light->cutOff, -1.0f, 1.0f)));
            float outerAngle = glm::degrees(glm::acos(glm::clamp(light->outerCutoff, -1.0f, 1.0f)));
            innerAngle = glm::clamp(innerAngle, 0.0f, outerAngle);

            if (ImGui::DragFloat(label.c_str(), &innerAngle, 0.1f, 0.0f, 89.0f, "%.1f deg"))
            {
                innerAngle = glm::clamp(innerAngle, 0.0f, outerAngle);
                light->cutOff = glm::cos(glm::radians(innerAngle));
                return true;
            }
            return false;
        }

        if (field.name == "outerCutoff" && light->lightType == LightType::SPOT_LIGHT)
        {
            handled = true;
            float innerAngle = glm::degrees(glm::acos(glm::clamp(light->cutOff, -1.0f, 1.0f)));
            float outerAngle = glm::degrees(glm::acos(glm::clamp(light->outerCutoff, -1.0f, 1.0f)));
            outerAngle = glm::clamp(outerAngle, innerAngle, 89.5f);

            if (ImGui::DragFloat(label.c_str(), &outerAngle, 0.1f, 0.0f, 89.5f, "%.1f deg"))
            {
                outerAngle = glm::clamp(outerAngle, innerAngle, 89.5f);
                light->outerCutoff = glm::cos(glm::radians(outerAngle));
                return true;
            }
            return false;
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

        if (auto* light = dynamic_cast<QELight*>(object))
        {
            if (ShouldSkipLightField(light, field))
                return false;
        }

        if (!IsSupportedFieldType(field.type))
            return false;

        void* fieldPtr = GetFieldPtr(object, field);
        if (!fieldPtr)
            return false;

        const std::string label = BuildWidgetLabel(field.name, widgetId);
        if (auto* light = dynamic_cast<QELight*>(object))
        {
            bool handled = false;
            const bool changed = DrawLightSpecificField(light, field, label, handled);
            if (handled)
                return changed;
        }

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
            if (field.name == "constant" || field.name == "linear" || field.name == "quadratic" || field.name == "radius")
            {
                return ImGui::DragFloat(label.c_str(), reinterpret_cast<float*>(fieldPtr), 0.1f, 0.0f, 100000.0f);
            }
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
        else if (field.type == typeid(PhysicBodyType))
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
        else if (field.type == typeid(CollisionFlag))
        {
            auto* value = reinterpret_cast<CollisionFlag*>(fieldPtr);

            if (field.name == "CollisionMask")
                return DrawCollisionMaskEditor(label.c_str(), *value);

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

    bool DrawTransformInspector(
        const std::shared_ptr<QEGameObject>& gameObject,
        const std::shared_ptr<QETransform>& transform,
        EditorCommandManager* commandManager,
        bool isSelected,
        bool& selectedThisFrame)
    {
        if (!gameObject || !transform)
            return false;

        bool requestRemove = false;
        const ImGuiTreeNodeFlags headerFlags =
            ImGuiTreeNodeFlags_DefaultOpen |
            ImGuiTreeNodeFlags_Framed |
            ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_OpenOnDoubleClick |
            (isSelected ? ImGuiTreeNodeFlags_Selected : 0);

        const bool isOpen = ImGui::TreeNodeEx("QETransformHeader", headerFlags, "QETransform");
        if (ImGui::IsItemClicked())
        {
            selectedThisFrame = true;
        }

        if (ImGui::BeginPopupContextItem("QETransformContext"))
        {
            ImGui::TextUnformatted("QETransform cannot be removed.");
            ImGui::EndPopup();
        }

        if (isOpen)
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

            ImGui::TreePop();
        }

        ImGui::Separator();
        return requestRemove;
    }

    bool DrawGenericComponentInspector(
        QEGameComponent* component,
        const std::string& uniquePrefix,
        bool isSelected,
        bool& selectedThisFrame)
    {
        if (!component)
            return false;

        if (!component->IsSerializable())
            return false;

        QEMetaType* meta = component->meta();
        if (!meta)
            return false;

        bool requestRemove = false;
        const std::string headerLabel = component->getTypeName();

        const ImGuiTreeNodeFlags headerFlags =
            ImGuiTreeNodeFlags_DefaultOpen |
            ImGuiTreeNodeFlags_Framed |
            ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_OpenOnDoubleClick |
            (isSelected ? ImGuiTreeNodeFlags_Selected : 0);

        const bool isOpen = ImGui::TreeNodeEx((uniquePrefix + "_header").c_str(), headerFlags, "%s", headerLabel.c_str());
        if (ImGui::IsItemClicked())
        {
            selectedThisFrame = true;
        }

        if (ImGui::BeginPopupContextItem((uniquePrefix + "_context").c_str()))
        {
            if (ImGui::MenuItem("Remove Component"))
            {
                requestRemove = true;
            }

            ImGui::EndPopup();
        }

        if (isOpen)
        {
            const auto fields = meta->allFields();

            int drawnFields = 0;
            std::unordered_set<std::string> drawnFieldKeys;

            for (size_t i = 0; i < fields.size(); ++i)
            {
                const auto& field = fields[i];

                if (ShouldSkipField(field))
                    continue;

                if (auto* light = dynamic_cast<QELight*>(component))
                {
                    if (ShouldSkipLightField(light, field))
                        continue;
                }

                if (!IsSupportedFieldType(field.type))
                    continue;

                const std::string fieldKey =
                    field.name + "|" +
                    std::to_string(field.offset) + "|" +
                    field.type.name();

                if (!drawnFieldKeys.insert(fieldKey).second)
                    continue;

                const std::string widgetId =
                    uniquePrefix + "_" + component->getTypeName() + "_" + field.name + "_" + std::to_string(i);

                DrawReflectedField(component, field, widgetId);

                ++drawnFields;
            }

            if (drawnFields == 0)
            {
                ImGui::TextUnformatted("No supported serializable fields.");
            }

            ImGui::TreePop();
        }

        ImGui::Separator();
        return requestRemove;
    }

    void DrawAtmosphereInspector()
    {
        auto atmosphere = AtmosphereSystem::getInstance();
        if (!atmosphere)
        {
            ImGui::TextUnformatted("AtmosphereSystem is null.");
            return;
        }

        AtmosphereDto dto = atmosphere->GetEditableAtmosphereDto();

        bool changedSunOnly = false;
        bool changedVisualOnly = false;
        bool changedPhysical = false;

        ImGui::TextUnformatted("Atmosphere");
        ImGui::Separator();

        if (ImGui::Checkbox("Enabled", &dto.hasAtmosphere))
        {
            changedVisualOnly = true;
        }

        ImGui::BeginDisabled(!dto.hasAtmosphere);

        glm::vec3 sunEuler = dto.sunEulerDegrees;
        if (ImGui::DragFloat3("Sun Rotation", &sunEuler.x, 0.5f))
        {
            dto.sunEulerDegrees = sunEuler;
            changedSunOnly = true;
        }

        if (ImGui::DragFloat("Sun Intensity", &dto.sunBaseIntensity, 1.0f, 0.0f, 100000.0f))
        {
            changedSunOnly = true;
        }

        if (ImGui::CollapsingHeader("Scattering", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::DragFloat3("Rayleigh Scattering", &dto.rayleighScattering.x, 0.01f, 0.0f, 100.0f))
                changedPhysical = true;

            if (ImGui::DragFloat("Rayleigh Scale Height", &dto.rayleighScaleHeight, 10.0f, 1.0f, 100000.0f))
                changedPhysical = true;

            if (ImGui::DragFloat3("Mie Scattering", &dto.mieScattering.x, 0.01f, 0.0f, 100.0f))
                changedPhysical = true;

            if (ImGui::DragFloat("Mie Scale Height", &dto.mieScaleHeight, 1.0f, 1.0f, 100000.0f))
                changedPhysical = true;

            if (ImGui::DragFloat("Mie Anisotropy", &dto.mieAnisotropy, 0.001f, 0.0f, 0.999f))
                changedPhysical = true;
        }

        if (ImGui::CollapsingHeader("Absorption", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::DragFloat3("Ozone Absorption", &dto.ozoneAbsorption.x, 0.01f, 0.0f, 10.0f))
                changedPhysical = true;

            if (ImGui::DragFloat("Ozone Density", &dto.ozoneDensity, 0.01f, 0.0f, 10.0f))
                changedPhysical = true;
        }

        if (ImGui::CollapsingHeader("Planet", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::DragFloat("Planet Radius", &dto.planetRadius, 1000.0f, 1.0f, 100000000.0f))
                changedPhysical = true;

            if (ImGui::DragFloat("Atmosphere Radius", &dto.atmosphereRadius, 1000.0f, 1.0f, 100000000.0f))
                changedPhysical = true;
        }

        if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::ColorEdit3("Sun Color", &dto.sunColor.x))
                changedVisualOnly = true;

            if (ImGui::DragFloat("Sun Intensity Multiplier", &dto.sunIntensityMultiplier, 0.01f, 0.0f, 100.0f))
                changedVisualOnly = true;
        }

        if (ImGui::CollapsingHeader("Artistic", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::DragFloat("Exposure", &dto.exposure, 0.05f, 0.0f, 100.0f))
                changedVisualOnly = true;

            if (ImGui::DragFloat("Sky Tint", &dto.skyTint, 0.01f, 0.0f, 10.0f))
                changedVisualOnly = true;

            if (ImGui::DragFloat("Horizon Softness", &dto.horizonSoftness, 0.01f, 0.0f, 10.0f))
                changedVisualOnly = true;

            if (ImGui::DragFloat("Multi Scattering Factor", &dto.multiScatteringFactor, 0.01f, 0.0f, 10.0f))
                changedPhysical = true;
        }

        if (ImGui::CollapsingHeader("Sun Disc / Bloom", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::DragFloat("Sun Disk Size", &dto.sunDiskSize, 0.01f, 0.0f, 20.0f))
                changedVisualOnly = true;

            if (ImGui::DragFloat("Sun Disk Intensity", &dto.sunDiskIntensity, 0.01f, 0.0f, 100.0f))
                changedVisualOnly = true;

            if (ImGui::DragFloat("Sun Glow", &dto.sunGlow, 0.01f, 0.0f, 100.0f))
                changedVisualOnly = true;
        }

        ImGui::EndDisabled();

        if (changedPhysical || changedVisualOnly || changedSunOnly)
        {
            atmosphere->ApplyEditableAtmosphereDto(dto, changedPhysical);
        }
    }

    std::string ToLowerCopy(const std::string& text)
    {
        std::string result = text;
        std::transform(result.begin(), result.end(), result.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return result;
    }

    bool ContainsCaseInsensitive(const std::string& text, const std::string& filter)
    {
        if (filter.empty())
            return true;

        return ToLowerCopy(text).find(ToLowerCopy(filter)) != std::string::npos;
    }

    bool IsComponentAddableToGameObject(
        const std::shared_ptr<QEGameObject>& gameObject,
        const std::string& componentTypeName)
    {
        if (!gameObject)
            return false;

        if (componentTypeName.empty())
            return false;

        if (componentTypeName == "QEGameComponent")
            return false;

        for (const auto& component : gameObject->components)
        {
            if (!component)
                continue;

            if (component->getTypeName() == componentTypeName)
                return false;
        }

        return true;
    }

    void DrawAddComponentPopup(const std::shared_ptr<QEGameObject>& gameObject)
    {
        if (!gameObject)
            return;

        if (ImGui::Button("Add Component"))
        {
            ImGui::OpenPopup("AddComponentPopup");
        }

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        const ImVec2 center = viewport->GetCenter();

        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(420.0f, 360.0f), ImGuiCond_Appearing);

        if (ImGui::BeginPopupModal("AddComponentPopup", nullptr, ImGuiWindowFlags_NoResize))
        {
            static char filterBuffer[128] = "";
            static bool setFocusToSearch = true;

            ImGui::TextUnformatted("Add Component");
            ImGui::Separator();

            if (setFocusToSearch)
            {
                ImGui::SetKeyboardFocusHere();
                setFocusToSearch = false;
            }

            ImGui::InputTextWithHint(
                "##ComponentSearch",
                "Search component...",
                filterBuffer,
                sizeof(filterBuffer));

            ImGui::Separator();

            const std::string filterText = filterBuffer;
            auto& factoryRegistry = getFactoryRegistry();

            std::vector<std::string> componentNames;
            componentNames.reserve(factoryRegistry.size());

            for (const auto& [typeName, factory] : factoryRegistry)
            {
                if (!factory)
                    continue;

                if (!IsComponentAddableToGameObject(gameObject, typeName))
                    continue;

                if (!ContainsCaseInsensitive(typeName, filterText))
                    continue;

                componentNames.push_back(typeName);
            }

            std::sort(componentNames.begin(), componentNames.end());

            ImGui::BeginChild("ComponentList", ImVec2(0.0f, -40.0f), true);

            if (componentNames.empty())
            {
                ImGui::TextUnformatted("No components found.");
            }
            else
            {
                for (const auto& componentName : componentNames)
                {
                    if (ImGui::Selectable(componentName.c_str()))
                    {
                        auto it = factoryRegistry.find(componentName);
                        if (it != factoryRegistry.end() && it->second)
                        {
                            std::unique_ptr<QEGameComponent> uniqueComponent = it->second();
                            if (uniqueComponent)
                            {
                                std::shared_ptr<QEGameComponent> sharedComponent(std::move(uniqueComponent));
                                gameObject->AddComponent(sharedComponent);
                            }
                        }

                        filterBuffer[0] = '\0';
                        setFocusToSearch = true;
                        ImGui::CloseCurrentPopup();
                        break;
                    }
                }
            }

            ImGui::EndChild();

            if (ImGui::Button("Close"))
            {
                filterBuffer[0] = '\0';
                setFocusToSearch = true;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
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

    if (selectionManager && selectionManager->IsAtmosphereSelected())
    {
        selectedComponentIndex = -1;
        DrawAtmosphereInspector();
        ImGui::End();
        return;
    }

    if (!selectionManager || !selectionManager->HasSelection())
    {
        selectedComponentIndex = -1;
        ImGui::TextUnformatted("No GameObject selected.");
        ImGui::End();
        return;
    }

    auto gameObject = selectionManager->GetSelectedGameObject();

    if (!gameObject)
    {
        selectedComponentIndex = -1;
        ImGui::TextUnformatted("Selected GameObject no longer exists.");
        selectionManager->ClearSelection();
        ImGui::End();
        return;
    }

    ImGui::Text("Name: %s", gameObject->Name.c_str());
    ImGui::Text("ID: %s", gameObject->ID().c_str());
    ImGui::Text("Children: %d", static_cast<int>(gameObject->childs.size()));
    ImGui::Text("Components: %d", static_cast<int>(gameObject->components.size()));

    bool isActive = gameObject->QEActive;
    if (ImGui::Checkbox("Active", &isActive))
    {
        gameObject->QEActive = isActive;
    }

    if (!gameObject->IsActiveInHierarchy() && gameObject->QEActive)
    {
        ImGui::TextDisabled("Inactive in hierarchy (parent disabled)");
    }

    int updateOrder = static_cast<int>(gameObject->UpdateOrder);
    if (ImGui::DragInt("Order", &updateOrder, 1.0f, 0, 100000))
    {
        updateOrder = std::max(0, updateOrder);

        if (gameObjectManager)
        {
            gameObjectManager->SetGameObjectUpdateOrder(
                gameObject,
                static_cast<unsigned int>(updateOrder));
        }
        else
        {
            gameObject->UpdateOrder = static_cast<unsigned int>(updateOrder);
        }
    }

    DrawAddComponentPopup(gameObject);

    ImGui::Separator();

    std::shared_ptr<QEGameComponent> componentToRemove = nullptr;
    int componentIndex = 0;
    for (auto& component : gameObject->components)
    {
        if (!component)
            continue;

        if (!component->IsSerializable())
            continue;

        bool requestRemove = false;
        bool selectedThisFrame = false;

        if (auto transform = std::dynamic_pointer_cast<QETransform>(component))
        {
            requestRemove = DrawTransformInspector(
                gameObject,
                transform,
                commandManager,
                selectedComponentIndex == componentIndex,
                selectedThisFrame);
        }
        else
        {
            const std::string uniquePrefix =
                "comp_" + gameObject->ID() + "_" + std::to_string(componentIndex);

            requestRemove = DrawGenericComponentInspector(
                component.get(),
                uniquePrefix,
                selectedComponentIndex == componentIndex,
                selectedThisFrame);
        }

        if (selectedThisFrame)
        {
            selectedComponentIndex = componentIndex;
        }

        if (requestRemove)
        {
            componentToRemove = component;
            break;
        }

        ++componentIndex;
    }

    if (componentToRemove)
    {
        gameObject->RemoveComponent(componentToRemove);
        if (selectedComponentIndex >= static_cast<int>(gameObject->components.size()))
        {
            selectedComponentIndex = static_cast<int>(gameObject->components.size()) - 1;
        }
    }

    const bool canDeleteWithKeyboard =
        selectedComponentIndex > 0 &&
        ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
        ImGui::IsKeyPressed(ImGuiKey_Delete, false) &&
        !ImGui::GetIO().WantTextInput;

    if (canDeleteWithKeyboard)
    {
        DeleteSelectedComponent(gameObject);
    }

    ImGui::End();
}

void InspectorPanel::DeleteSelectedComponent(const std::shared_ptr<QEGameObject>& gameObject)
{
    if (!gameObject)
        return;

    if (selectedComponentIndex <= 0)
        return;

    if (selectedComponentIndex >= static_cast<int>(gameObject->components.size()))
        return;

    auto it = gameObject->components.begin();
    std::advance(it, selectedComponentIndex);
    auto component = (it != gameObject->components.end()) ? *it : nullptr;
    if (!component)
        return;

    if (!gameObject->RemoveComponent(component))
        return;

    if (selectedComponentIndex >= static_cast<int>(gameObject->components.size()))
    {
        selectedComponentIndex = static_cast<int>(gameObject->components.size()) - 1;
    }
}

