#include "AnimationGraphPanel.h"

#include <imgui.h>

#include <Data/QEAnimationGraphAssetHelper.h>
#include <QuarantineEditor/Animation/QEAnimationGraphRuntimeAdapter.h>
#include <QuarantineEditor/Core/EditorContext.h>
#include <QuarantineEditor/Core/EditorSelectionManager.h>
#include <QEAnimationComponent.h>
#include <QEGameObject.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <filesystem>
#include <unordered_map>
#include <unordered_set>

namespace
{
    constexpr const char* kContextMenuId = "AnimationGraphContextMenu";
    constexpr int kMaxStateSlots = 8;

    const char* ToString(QEParamType type)
    {
        switch (type)
        {
        case QEParamType::Bool: return "Bool";
        case QEParamType::Int: return "Int";
        case QEParamType::Float: return "Float";
        case QEParamType::Trigger: return "Trigger";
        }

        return "Unknown";
    }

    const char* ToString(QEOp op)
    {
        switch (op)
        {
        case QEOp::Equal: return "Equal";
        case QEOp::NotEqual: return "NotEqual";
        case QEOp::Greater: return "Greater";
        case QEOp::Less: return "Less";
        case QEOp::GreaterEqual: return "GreaterEqual";
        case QEOp::LessEqual: return "LessEqual";
        }

        return "Equal";
    }

    const std::array<QEOp, 6> kAllOps = {
        QEOp::Equal,
        QEOp::NotEqual,
        QEOp::Greater,
        QEOp::Less,
        QEOp::GreaterEqual,
        QEOp::LessEqual
    };

    QEAnimationGraphNode* FindNodeById(QEAnimationGraphEditorData& graphData, const std::string& nodeId)
    {
        for (auto& node : graphData.Nodes)
        {
            if (node.Id == nodeId)
                return &node;
        }
        return nullptr;
    }

    const QEAnimationGraphNode* FindNodeById(const QEAnimationGraphEditorData& graphData, const std::string& nodeId)
    {
        for (const auto& node : graphData.Nodes)
        {
            if (node.Id == nodeId)
                return &node;
        }
        return nullptr;
    }

    QEAnimationGraphNode* FindEntryNode(QEAnimationGraphEditorData& graphData)
    {
        for (auto& node : graphData.Nodes)
        {
            if (node.Kind == QEAnimationGraphNodeKind::Entry)
                return &node;
        }
        return nullptr;
    }

    const QEAnimationGraphNode* FindEntryNode(const QEAnimationGraphEditorData& graphData)
    {
        for (const auto& node : graphData.Nodes)
        {
            if (node.Kind == QEAnimationGraphNodeKind::Entry)
                return &node;
        }
        return nullptr;
    }

    QEAnimationGraphLink* FindLinkById(QEAnimationGraphEditorData& graphData, const std::string& linkId)
    {
        for (auto& link : graphData.Links)
        {
            if (link.Id == linkId)
                return &link;
        }
        return nullptr;
    }

    bool HasNodeId(const QEAnimationGraphEditorData& graphData, const std::string& nodeId)
    {
        return FindNodeById(graphData, nodeId) != nullptr;
    }

    bool HasStateId(const QEAnimationGraphEditorData& graphData, const std::string& stateId)
    {
        for (const auto& node : graphData.Nodes)
        {
            if (node.Kind == QEAnimationGraphNodeKind::State && node.StateId == stateId)
                return true;
        }
        return false;
    }

    bool IsTriggerParameter(const QEAnimationComponent& component, const std::string& paramName)
    {
        const auto& triggers = component.GetTriggerParameterNames();
        return std::find(triggers.begin(), triggers.end(), paramName) != triggers.end();
    }

    std::vector<std::pair<std::string, QEParamType>> BuildDeclaredParameters(const QEAnimationComponent& component)
    {
        std::vector<std::pair<std::string, QEParamType>> params;

        for (const auto& name : component.GetBoolParameterNames())
            params.emplace_back(name, QEParamType::Bool);
        for (const auto& name : component.GetIntParameterNames())
            params.emplace_back(name, QEParamType::Int);
        for (const auto& name : component.GetFloatParameterNames())
            params.emplace_back(name, QEParamType::Float);
        for (const auto& name : component.GetTriggerParameterNames())
            params.emplace_back(name, QEParamType::Trigger);

        return params;
    }

    int FindNextFreeSourceSlot(const QEAnimationGraphEditorData& graphData, const std::string& fromNodeId)
    {
        const auto* node = FindNodeById(graphData, fromNodeId);
        const int slotLimit = node ? std::clamp(node->OutputSlotCount, 1, kMaxStateSlots) : kMaxStateSlots;
        std::array<bool, kMaxStateSlots> used{};
        for (const auto& link : graphData.Links)
        {
            if (link.FromNodeId == fromNodeId && link.SourceSlot >= 0 && link.SourceSlot < slotLimit)
            {
                used[link.SourceSlot] = true;
            }
        }

        for (int slot = 0; slot < slotLimit; ++slot)
        {
            if (!used[slot])
                return slot;
        }

        return slotLimit - 1;
    }

    int FindNextFreeTargetSlot(const QEAnimationGraphEditorData& graphData, const std::string& toNodeId)
    {
        const auto* node = FindNodeById(graphData, toNodeId);
        const int slotLimit = node ? std::clamp(node->InputSlotCount, 1, kMaxStateSlots) : kMaxStateSlots;
        std::array<bool, kMaxStateSlots> used{};
        for (const auto& link : graphData.Links)
        {
            if (link.ToNodeId == toNodeId && link.TargetSlot >= 0 && link.TargetSlot < slotLimit)
            {
                used[link.TargetSlot] = true;
            }
        }

        for (int slot = 0; slot < slotLimit; ++slot)
        {
            if (!used[slot])
                return slot;
        }

        return slotLimit - 1;
    }

    std::string MakeStateNodeId(const std::string& stateId)
    {
        return "state:" + stateId;
    }

    std::string MakeTransitionLinkId(const std::string& fromStateId, const std::string& toStateId, int ordinal)
    {
        return "link:" + fromStateId + ":" + toStateId + ":" + std::to_string(ordinal);
    }

    std::string GetGraphDisplayName(const std::string& graphAssetPath)
    {
        if (graphAssetPath.empty())
        {
            return "<none>";
        }

        return std::filesystem::path(graphAssetPath).stem().string();
    }

    void NormalizeLinks(QEAnimationGraphEditorData& graphData)
    {
        std::unordered_map<std::string, int> ordinalsByPair;

        for (auto& link : graphData.Links)
        {
            if (link.FromNodeId == "entry")
            {
                link.Id = "link:entry";
                link.TransitionOrdinal = 0;
                continue;
            }

            const std::string pairKey = link.FromStateId + "->" + link.ToStateId;
            link.TransitionOrdinal = ordinalsByPair[pairKey]++;
            link.Id = MakeTransitionLinkId(link.FromStateId, link.ToStateId, link.TransitionOrdinal);
        }
    }

    int MakeStateTemplateIndex(int inputCount, int outputCount)
    {
        const int clampedInput = std::clamp(inputCount, 1, kMaxStateSlots);
        const int clampedOutput = std::clamp(outputCount, 1, kMaxStateSlots);
        return 1 + (clampedInput - 1) * kMaxStateSlots + (clampedOutput - 1);
    }

    void NormalizeNodeSlotCounts(QEAnimationGraphEditorData& graphData)
    {
        std::unordered_map<std::string, int> incomingCounts;
        std::unordered_map<std::string, int> outgoingCounts;

        for (auto& link : graphData.Links)
        {
            if (link.FromNodeId != "entry")
            {
                link.SourceSlot = std::min(outgoingCounts[link.FromNodeId]++, kMaxStateSlots - 1);
            }

            link.TargetSlot = std::min(incomingCounts[link.ToNodeId]++, kMaxStateSlots - 1);
        }

        for (auto& node : graphData.Nodes)
        {
            if (node.Kind == QEAnimationGraphNodeKind::Entry)
            {
                node.InputSlotCount = 0;
                node.OutputSlotCount = 1;
                continue;
            }

            node.InputSlotCount = std::clamp(std::max(node.InputSlotCount, incomingCounts[node.Id]), 1, kMaxStateSlots);
            node.OutputSlotCount = std::clamp(std::max(node.OutputSlotCount, outgoingCounts[node.Id]), 1, kMaxStateSlots);
        }
    }

    class AnimationGraphDelegate final : public GraphEditor::Delegate
    {
    public:
        AnimationGraphDelegate(
            AnimationGraphPanel& panel,
            const std::shared_ptr<QEGameObject>& gameObject,
            QEAnimationComponent& component,
            AnimationGraphPanel::SessionGraphState& sessionGraph)
            : _panel(panel)
            , _gameObject(gameObject)
            , _component(component)
            , _sessionGraph(sessionGraph)
        {
            for (size_t i = 0; i < _sessionGraph.GraphData.Nodes.size(); ++i)
            {
                _nodeIndices[_sessionGraph.GraphData.Nodes[i].Id] = i;
            }

            for (const auto& state : _component.GetAnimationStates())
            {
                _statesById[state.Id] = state;
            }
        }

        bool AllowedLink(GraphEditor::NodeIndex from, GraphEditor::NodeIndex to) override
        {
            if (from >= _sessionGraph.GraphData.Nodes.size() || to >= _sessionGraph.GraphData.Nodes.size())
                return false;

            if (from == to)
                return false;

            const auto& fromNode = _sessionGraph.GraphData.Nodes[from];
            const auto& toNode = _sessionGraph.GraphData.Nodes[to];

            if (toNode.Kind == QEAnimationGraphNodeKind::Entry)
                return false;

            return fromNode.Kind == QEAnimationGraphNodeKind::Entry ||
                fromNode.Kind == QEAnimationGraphNodeKind::State;
        }

        void SelectNode(GraphEditor::NodeIndex nodeIndex, bool selected) override
        {
            if (nodeIndex >= _sessionGraph.GraphData.Nodes.size())
                return;

            const std::string& nodeId = _sessionGraph.GraphData.Nodes[nodeIndex].Id;
            if (selected)
                _sessionGraph.SelectedNodeIds.insert(nodeId);
            else
                _sessionGraph.SelectedNodeIds.erase(nodeId);
        }

        void MoveSelectedNodes(const ImVec2 delta) override
        {
            for (auto& node : _sessionGraph.GraphData.Nodes)
            {
                if (_sessionGraph.SelectedNodeIds.contains(node.Id))
                {
                    node.Position += glm::vec2(delta.x, delta.y);
                }
            }
        }

        void AddLink(GraphEditor::NodeIndex inputNodeIndex, GraphEditor::SlotIndex, GraphEditor::NodeIndex outputNodeIndex, GraphEditor::SlotIndex) override
        {
            if (inputNodeIndex >= _sessionGraph.GraphData.Nodes.size() || outputNodeIndex >= _sessionGraph.GraphData.Nodes.size())
                return;

            auto& fromNode = _sessionGraph.GraphData.Nodes[inputNodeIndex];
            auto& toNode = _sessionGraph.GraphData.Nodes[outputNodeIndex];

            if (fromNode.Kind == QEAnimationGraphNodeKind::Entry)
            {
                auto* entryNode = FindEntryNode(_sessionGraph.GraphData);
                if (entryNode)
                {
                    entryNode->StateId = toNode.StateId;
                }

                _sessionGraph.GraphData.Links.erase(
                    std::remove_if(
                        _sessionGraph.GraphData.Links.begin(),
                        _sessionGraph.GraphData.Links.end(),
                        [](const QEAnimationGraphLink& existingLink)
                        {
                            return existingLink.FromNodeId == "entry";
                        }),
                    _sessionGraph.GraphData.Links.end());

                QEAnimationGraphLink entryLink;
                entryLink.Id = "link:entry";
                entryLink.FromNodeId = "entry";
                entryLink.ToNodeId = toNode.Id;
                entryLink.ToStateId = toNode.StateId;
                entryLink.TargetSlot = FindNextFreeTargetSlot(_sessionGraph.GraphData, toNode.Id);
                _sessionGraph.GraphData.Links.insert(_sessionGraph.GraphData.Links.begin(), entryLink);
                NormalizeNodeSlotCounts(_sessionGraph.GraphData);
                _panel.SetEntryNode(_sessionGraph, toNode.StateId);
                _panel.ApplyGraphToComponent(_component, _sessionGraph);
                return;
            }

            if (FindNextFreeSourceSlot(_sessionGraph.GraphData, fromNode.Id) == std::max(0, fromNode.OutputSlotCount - 1) &&
                fromNode.OutputSlotCount < kMaxStateSlots)
            {
                bool allUsed = true;
                for (int slot = 0; slot < fromNode.OutputSlotCount; ++slot)
                {
                    bool used = false;
                    for (const auto& existingLink : _sessionGraph.GraphData.Links)
                    {
                        if (existingLink.FromNodeId == fromNode.Id && existingLink.SourceSlot == slot)
                        {
                            used = true;
                            break;
                        }
                    }
                    if (!used)
                    {
                        allUsed = false;
                        break;
                    }
                }
                if (allUsed)
                {
                    fromNode.OutputSlotCount++;
                }
            }

            if (FindNextFreeTargetSlot(_sessionGraph.GraphData, toNode.Id) == std::max(0, toNode.InputSlotCount - 1) &&
                toNode.InputSlotCount < kMaxStateSlots)
            {
                bool allUsed = true;
                for (int slot = 0; slot < toNode.InputSlotCount; ++slot)
                {
                    bool used = false;
                    for (const auto& existingLink : _sessionGraph.GraphData.Links)
                    {
                        if (existingLink.ToNodeId == toNode.Id && existingLink.TargetSlot == slot)
                        {
                            used = true;
                            break;
                        }
                    }
                    if (!used)
                    {
                        allUsed = false;
                        break;
                    }
                }
                if (allUsed)
                {
                    toNode.InputSlotCount++;
                }
            }

            QEAnimationGraphLink link;
            link.FromNodeId = fromNode.Id;
            link.ToNodeId = toNode.Id;
            link.FromStateId = fromNode.StateId;
            link.ToStateId = toNode.StateId;
            link.SourceSlot = FindNextFreeSourceSlot(_sessionGraph.GraphData, fromNode.Id);
            link.TargetSlot = FindNextFreeTargetSlot(_sessionGraph.GraphData, toNode.Id);
            _sessionGraph.GraphData.Links.push_back(link);
            NormalizeNodeSlotCounts(_sessionGraph.GraphData);
            NormalizeLinks(_sessionGraph.GraphData);
            if (!_sessionGraph.GraphData.Links.empty())
            {
                _sessionGraph.SelectedLinkId = _sessionGraph.GraphData.Links.back().Id;
            }
            _panel.ApplyGraphToComponent(_component, _sessionGraph);
        }

        void DelLink(GraphEditor::LinkIndex linkIndex) override
        {
            if (linkIndex >= _sessionGraph.GraphData.Links.size())
                return;

            const std::string removedLinkId = _sessionGraph.GraphData.Links[linkIndex].Id;
            const bool isEntryLink = _sessionGraph.GraphData.Links[linkIndex].FromNodeId == "entry";
            _sessionGraph.GraphData.Links.erase(_sessionGraph.GraphData.Links.begin() + static_cast<std::ptrdiff_t>(linkIndex));

            if (_sessionGraph.SelectedLinkId == removedLinkId)
            {
                _sessionGraph.SelectedLinkId.clear();
            }

            if (isEntryLink)
            {
                _panel.SetEntryNode(_sessionGraph, "");
            }

            NormalizeNodeSlotCounts(_sessionGraph.GraphData);
            NormalizeLinks(_sessionGraph.GraphData);
            _panel.ApplyGraphToComponent(_component, _sessionGraph);
        }

        void CustomDraw(ImDrawList* drawList, ImRect rectangle, GraphEditor::NodeIndex nodeIndex) override
        {
            if (nodeIndex >= _sessionGraph.GraphData.Nodes.size())
                return;

            const auto& node = _sessionGraph.GraphData.Nodes[nodeIndex];
            ImVec2 cursor(rectangle.Min.x + 8.0f, rectangle.Min.y + 8.0f);

            if (node.Kind == QEAnimationGraphNodeKind::Entry)
            {
                const std::string text = node.StateId.empty() ? "Target: None" : "Target: " + node.StateId;
                drawList->AddText(cursor, IM_COL32(220, 220, 220, 255), text.c_str());
                return;
            }

            auto stateIt = _statesById.find(node.StateId);
            if (stateIt == _statesById.end())
            {
                drawList->AddText(cursor, IM_COL32(255, 140, 120, 255), "State data missing");
                return;
            }

            const AnimationState& state = stateIt->second;
            const std::string clipText = "Clip: " + (state.AnimationClip.empty() ? std::string("<none>") : state.AnimationClip);
            const std::string loopText = std::string("Loop: ") + (state.Loop ? "true" : "false");
            const std::string currentStateId = _component.GetCurrentState().Id;
            const bool isCurrentState = (!currentStateId.empty() && node.Id == MakeStateNodeId(currentStateId));

            if (isCurrentState)
            {
                drawList->AddRect(rectangle.Min, rectangle.Max, IM_COL32(34, 197, 94, 255), 4.0f, 0, 3.0f);
                drawList->AddText(ImVec2(rectangle.Max.x - 70.0f, rectangle.Min.y + 8.0f), IM_COL32(134, 239, 172, 255), "ACTIVE");
            }

            drawList->AddText(cursor, IM_COL32(220, 220, 220, 255), clipText.c_str());
            cursor.y += 18.0f;
            drawList->AddText(cursor, IM_COL32(180, 220, 180, 255), loopText.c_str());
        }

        ImU32 GetLinkColor(GraphEditor::LinkIndex index) override
        {
            if (index >= _sessionGraph.GraphData.Links.size())
                return 0;

            if (!_component.IsInStateTransition())
                return 0;

            const auto& link = _sessionGraph.GraphData.Links[index];
            if (link.FromNodeId == "entry")
                return 0;

            if (link.FromStateId == _component.GetActiveTransitionFromState() &&
                link.ToStateId == _component.GetActiveTransitionToState())
            {
                return IM_COL32(34, 197, 94, 255);
            }

            return 0;
        }

        void RightClick(GraphEditor::NodeIndex nodeIndex, GraphEditor::SlotIndex slotIndexInput, GraphEditor::SlotIndex slotIndexOutput) override
        {
            _panel.RequestContextMenu(
                _gameObject ? _gameObject->ID() : std::string{},
                static_cast<int>(nodeIndex),
                static_cast<int>(slotIndexInput),
                static_cast<int>(slotIndexOutput));
        }

        const size_t GetTemplateCount() override
        {
            return 1 + static_cast<size_t>(kMaxStateSlots * kMaxStateSlots);
        }

        const GraphEditor::Template GetTemplate(GraphEditor::TemplateIndex index) override
        {
            static const char* entryOutputs[] = { "Out" };
            static const char* stateInputs[] = { "In", "In", "In", "In", "In", "In", "In", "In" };
            static const char* stateOutputs[] = { "Out", "Out", "Out", "Out", "Out", "Out", "Out", "Out" };
            static ImU32 entryOutputColors[] = { IM_COL32(234, 179, 8, 255) };
            static ImU32 stateInputColors[] =
            {
                IM_COL32(148, 163, 184, 255), IM_COL32(148, 163, 184, 255), IM_COL32(148, 163, 184, 255), IM_COL32(148, 163, 184, 255),
                IM_COL32(148, 163, 184, 255), IM_COL32(148, 163, 184, 255), IM_COL32(148, 163, 184, 255), IM_COL32(148, 163, 184, 255)
            };
            static ImU32 stateOutputColors[] =
            {
                IM_COL32(96, 165, 250, 255), IM_COL32(96, 165, 250, 255), IM_COL32(96, 165, 250, 255), IM_COL32(96, 165, 250, 255),
                IM_COL32(96, 165, 250, 255), IM_COL32(96, 165, 250, 255), IM_COL32(96, 165, 250, 255), IM_COL32(96, 165, 250, 255)
            };

            static const GraphEditor::Template templates[] =
            {
                {
                    IM_COL32(196, 138, 12, 255),
                    IM_COL32(78, 61, 16, 255),
                    IM_COL32(106, 79, 18, 255),
                    0,
                    nullptr,
                    nullptr,
                    1,
                    entryOutputs,
                    entryOutputColors
                }
            };

            if (index == 0)
            {
                return templates[0];
            }

            const int stateTemplateIndex = static_cast<int>(index) - 1;
            GraphEditor::Template stateTemplate{};
            stateTemplate.mHeaderColor = IM_COL32(30, 64, 175, 255);
            stateTemplate.mBackgroundColor = IM_COL32(30, 41, 59, 255);
            stateTemplate.mBackgroundColorOver = IM_COL32(51, 65, 85, 255);
            stateTemplate.mInputCount = static_cast<ImU8>(1 + (stateTemplateIndex / kMaxStateSlots));
            stateTemplate.mInputNames = stateInputs;
            stateTemplate.mInputColors = stateInputColors;
            stateTemplate.mOutputCount = static_cast<ImU8>(1 + (stateTemplateIndex % kMaxStateSlots));
            stateTemplate.mOutputNames = stateOutputs;
            stateTemplate.mOutputColors = stateOutputColors;
            return stateTemplate;
        }

        const size_t GetNodeCount() override
        {
            return _sessionGraph.GraphData.Nodes.size();
        }

        const GraphEditor::Node GetNode(GraphEditor::NodeIndex index) override
        {
            const auto& node = _sessionGraph.GraphData.Nodes[index];
            return GraphEditor::Node
            {
                node.Title.c_str(),
                node.Kind == QEAnimationGraphNodeKind::Entry
                    ? 0u
                    : static_cast<GraphEditor::TemplateIndex>(MakeStateTemplateIndex(node.InputSlotCount, node.OutputSlotCount)),
                ImRect(
                    ImVec2(node.Position.x, node.Position.y),
                    ImVec2(node.Position.x + node.Size.x, node.Position.y + node.Size.y)),
                _sessionGraph.SelectedNodeIds.contains(node.Id)
            };
        }

        const size_t GetLinkCount() override
        {
            return _sessionGraph.GraphData.Links.size();
        }

        const GraphEditor::Link GetLink(GraphEditor::LinkIndex index) override
        {
            const auto& link = _sessionGraph.GraphData.Links[index];

            auto fromIt = _nodeIndices.find(link.FromNodeId);
            auto toIt = _nodeIndices.find(link.ToNodeId);

            return GraphEditor::Link
            {
                fromIt != _nodeIndices.end() ? fromIt->second : 0u,
                static_cast<size_t>(std::max(0, link.SourceSlot)),
                toIt != _nodeIndices.end() ? toIt->second : 0u,
                static_cast<size_t>(std::max(0, link.TargetSlot))
            };
        }

    private:
        AnimationGraphPanel& _panel;
        std::shared_ptr<QEGameObject> _gameObject;
        QEAnimationComponent& _component;
        AnimationGraphPanel::SessionGraphState& _sessionGraph;
        std::unordered_map<std::string, size_t> _nodeIndices;
        std::unordered_map<std::string, AnimationState> _statesById;
    };
}

AnimationGraphPanel::AnimationGraphPanel(
    EditorContext* editorContext,
    EditorSelectionManager* selectionManager)
    : editorContext(editorContext)
    , selectionManager(selectionManager)
{
    _graphOptions.mLineThickness = 3.0f;
    _graphOptions.mNodeSlotRadius = 6.0f;
    _graphOptions.mNodeSlotHoverFactor = 1.15f;
    _graphOptions.mMinimap = ImRect(0.0f, 0.0f, 0.0f, 0.0f);
    _graphOptions.mDrawIONameOnHover = false;
}

void AnimationGraphPanel::Draw()
{
    if (!editorContext || !editorContext->ShowAnimationGraph)
        return;

    if (!ImGui::Begin("Animation Graph", &editorContext->ShowAnimationGraph))
    {
        ImGui::End();
        return;
    }

    if (!selectionManager || !selectionManager->HasSelection() || selectionManager->IsAtmosphereSelected())
    {
        ImGui::TextUnformatted("Select a GameObject with a QEAnimationComponent.");
        ImGui::End();
        return;
    }

    auto gameObject = selectionManager->GetSelectedGameObject();
    if (!gameObject)
    {
        ImGui::TextUnformatted("Selected GameObject no longer exists.");
        ImGui::End();
        return;
    }

    auto animationComponent = gameObject->GetComponent<QEAnimationComponent>();
    if (!animationComponent)
    {
        ImGui::Text("'%s' does not have a QEAnimationComponent.", gameObject->Name.c_str());
        ImGui::End();
        return;
    }

    auto& sessionGraph = GetOrCreateSessionGraph(gameObject, *animationComponent);

    DrawToolbar(gameObject, *animationComponent, sessionGraph);
    ImGui::Separator();

    if (ImGui::BeginTable("AnimationGraphLayout", 3, ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn("Parameters", ImGuiTableColumnFlags_WidthStretch, 0.22f);
        ImGui::TableSetupColumn("Graph", ImGuiTableColumnFlags_WidthStretch, 0.50f);
        ImGui::TableSetupColumn("Properties", ImGuiTableColumnFlags_WidthStretch, 0.28f);

        ImGui::TableNextColumn();
        DrawParametersPane(*animationComponent, sessionGraph);

        ImGui::TableNextColumn();
        DrawGraphArea(gameObject, *animationComponent, sessionGraph);

        ImGui::TableNextColumn();
        DrawPropertiesPane(*animationComponent, sessionGraph);

        ImGui::EndTable();
    }

    DrawContextMenu(gameObject, *animationComponent, sessionGraph);

    ImGui::End();
}

AnimationGraphPanel::SessionGraphState& AnimationGraphPanel::GetOrCreateSessionGraph(
    const std::shared_ptr<QEGameObject>& gameObject,
    QEAnimationComponent& component)
{
    const std::string objectId = gameObject ? gameObject->ID() : std::string{};
    auto [it, inserted] = _sessionGraphs.try_emplace(objectId);
    const std::string currentGraphAssetPath = QEAnimationGraphAssetHelper::GetGraphAssetPathOrDefault(component);
    if (inserted || it->second.GraphData.Nodes.empty() || it->second.LoadedGraphAssetPath != currentGraphAssetPath)
    {
        it->second.GraphData = QEAnimationGraphRuntimeAdapter::BuildEditorData(component);
        NormalizeNodeSlotCounts(it->second.GraphData);
        it->second.SelectedNodeIds.clear();
        it->second.SelectedLinkId.clear();
        it->second.LoadedGraphAssetPath = currentGraphAssetPath;
        it->second.RenameGraphNameBuffer = GetGraphDisplayName(currentGraphAssetPath);
    }

    return it->second;
}

void AnimationGraphPanel::DrawToolbar(
    const std::shared_ptr<QEGameObject>& gameObject,
    QEAnimationComponent& component,
    SessionGraphState& sessionGraph)
{
    ImGui::Text("Object: %s", gameObject ? gameObject->Name.c_str() : "None");
    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();
    ImGui::Text("States: %d", static_cast<int>(component.GetAnimationStates().size()));
    ImGui::SameLine();
    ImGui::Text("Transitions: %d", static_cast<int>(component.GetAnimationTransitions().size()));

    const std::string currentGraphAssetPath = QEAnimationGraphAssetHelper::GetGraphAssetPathOrDefault(component);
    const auto availableGraphAssets = QEAnimationGraphAssetHelper::ListGraphAssets(component);

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Graph");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(220.0f);
    if (ImGui::BeginCombo("##AnimationGraphAsset", GetGraphDisplayName(currentGraphAssetPath).c_str()))
    {
        for (const auto& graphAssetPath : availableGraphAssets)
        {
            const bool selected = (graphAssetPath == currentGraphAssetPath);
            const std::string label = GetGraphDisplayName(graphAssetPath);
            if (ImGui::Selectable(label.c_str(), selected))
            {
                QEAnimationGraphAssetHelper::SaveGraphAsset(component);
                if (QEAnimationGraphAssetHelper::LoadGraphAsset(component, graphAssetPath))
                {
                    RefreshGraphFromComponent(gameObject, component);
                    sessionGraph.LoadedGraphAssetPath = QEAnimationGraphAssetHelper::GetGraphAssetPathOrDefault(component);
                    sessionGraph.RenameGraphNameBuffer = GetGraphDisplayName(sessionGraph.LoadedGraphAssetPath);
                }
            }

            if (selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine();
    if (ImGui::Button("New Graph"))
    {
        QEAnimationGraphAssetHelper::SaveGraphAsset(component);
        if (QEAnimationGraphAssetHelper::CreateGraphAsset(component, "default", true))
        {
            const std::string createdGraphPath = component.GetGraphAssetPath();
            QEAnimationGraphAssetHelper::LoadGraphAsset(component, createdGraphPath);
            RefreshGraphFromComponent(gameObject, component);
            sessionGraph.LoadedGraphAssetPath = QEAnimationGraphAssetHelper::GetGraphAssetPathOrDefault(component);
            sessionGraph.RenameGraphNameBuffer = GetGraphDisplayName(sessionGraph.LoadedGraphAssetPath);
        }
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(140.0f);
    char graphNameBuffer[128] = {};
    std::strncpy(graphNameBuffer, sessionGraph.RenameGraphNameBuffer.c_str(), sizeof(graphNameBuffer) - 1);
    if (ImGui::InputTextWithHint("##GraphName", "Graph Name", graphNameBuffer, sizeof(graphNameBuffer)))
    {
        sessionGraph.RenameGraphNameBuffer = graphNameBuffer;
    }

    ImGui::SameLine();
    if (ImGui::Button("Rename Graph"))
    {
        QEAnimationGraphAssetHelper::SaveGraphAsset(component);
        if (QEAnimationGraphAssetHelper::RenameGraphAsset(component, sessionGraph.RenameGraphNameBuffer))
        {
            sessionGraph.LoadedGraphAssetPath = QEAnimationGraphAssetHelper::GetGraphAssetPathOrDefault(component);
            sessionGraph.RenameGraphNameBuffer = GetGraphDisplayName(sessionGraph.LoadedGraphAssetPath);
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Delete Graph"))
    {
        QEAnimationGraphAssetHelper::SaveGraphAsset(component);
        if (QEAnimationGraphAssetHelper::DeleteGraphAsset(component))
        {
            RefreshGraphFromComponent(gameObject, component);
            sessionGraph.LoadedGraphAssetPath = QEAnimationGraphAssetHelper::GetGraphAssetPathOrDefault(component);
            sessionGraph.RenameGraphNameBuffer = GetGraphDisplayName(sessionGraph.LoadedGraphAssetPath);
        }
    }

    ImGui::Spacing();

    if (ImGui::Button("Refresh From Runtime"))
    {
        RefreshGraphFromComponent(gameObject, component);
    }

    ImGui::SameLine();
    if (ImGui::Button("Add State"))
    {
        AddStateNode(sessionGraph);
        ApplyGraphToComponent(component, sessionGraph);
    }

    ImGui::SameLine();
    if (ImGui::Button("Fit All"))
    {
        _fitRequest = GraphEditor::Fit_AllNodes;
    }

    ImGui::SameLine();
    if (ImGui::Button("Clear Selection"))
    {
        sessionGraph.SelectedNodeIds.clear();
        sessionGraph.SelectedLinkId.clear();
    }
}

void AnimationGraphPanel::DrawParametersPane(QEAnimationComponent& component, SessionGraphState& sessionGraph)
{
    ImGui::BeginChild("AnimationGraphParameters", ImVec2(0.0f, 0.0f), true);

    auto patchParameterNameInTransitions = [&](const std::string& oldName, const std::string& newName)
    {
        auto transitionsToPatch = component.GetAnimationTransitions();
        for (auto& transitionToPatch : transitionsToPatch)
        {
            for (auto& conditionToPatch : transitionToPatch.conditions)
            {
                if (conditionToPatch.param == oldName)
                {
                    conditionToPatch.param = newName;
                }
            }
        }
        return transitionsToPatch;
    };

    auto removeParameterFromTransitions = [&](const std::string& removedName)
    {
        auto transitionsToPatch = component.GetAnimationTransitions();
        for (auto& transitionToPatch : transitionsToPatch)
        {
            transitionToPatch.conditions.erase(
                std::remove_if(
                    transitionToPatch.conditions.begin(),
                    transitionToPatch.conditions.end(),
                    [&](const QECondition& conditionToPatch)
                    {
                        return conditionToPatch.param == removedName;
                    }),
                transitionToPatch.conditions.end());
        }
        return transitionsToPatch;
    };

    auto applyParameterEdits = [&](const std::vector<std::string>& boolNames,
                                   const std::vector<std::string>& intNames,
                                   const std::vector<std::string>& floatNames,
                                   const std::vector<std::string>& triggerNames,
                                   const std::vector<QETransition>& transitionsToPatch)
    {
        component.SetBoolParameterNames(boolNames);
        component.SetIntParameterNames(intNames);
        component.SetFloatParameterNames(floatNames);
        component.SetTriggerParameterNames(triggerNames);
        component.SetStateMachineData(component.GetAnimationStates(), transitionsToPatch, component.GetCurrentState().Id);
    };

    struct ParameterEntry
    {
        QEParamType Type;
        std::string Name;
    };

    std::vector<ParameterEntry> parameterEntries;
    for (const auto& name : component.GetBoolParameterNames())
        parameterEntries.push_back({ QEParamType::Bool, name });
    for (const auto& name : component.GetIntParameterNames())
        parameterEntries.push_back({ QEParamType::Int, name });
    for (const auto& name : component.GetFloatParameterNames())
        parameterEntries.push_back({ QEParamType::Float, name });
    for (const auto& name : component.GetTriggerParameterNames())
        parameterEntries.push_back({ QEParamType::Trigger, name });

    ImGui::TextUnformatted("Parameters");
    ImGui::Separator();

    int paramToRemove = -1;
    for (size_t i = 0; i < parameterEntries.size(); ++i)
    {
        ImGui::PushID(1000 + static_cast<int>(i));

        ImGui::Text("%s", ToString(parameterEntries[i].Type));
        ImGui::SameLine();

        char nameBuffer[128] = {};
        std::strncpy(nameBuffer, parameterEntries[i].Name.c_str(), sizeof(nameBuffer) - 1);
        if (ImGui::InputText("##ParameterName", nameBuffer, sizeof(nameBuffer)))
        {
            const std::string updatedName = nameBuffer;
            if (!updatedName.empty())
            {
                auto boolNames = component.GetBoolParameterNames();
                auto intNames = component.GetIntParameterNames();
                auto floatNames = component.GetFloatParameterNames();
                auto triggerNames = component.GetTriggerParameterNames();
                auto transitionsToPatch = patchParameterNameInTransitions(parameterEntries[i].Name, updatedName);

                auto renameInList = [&](std::vector<std::string>& names)
                {
                    auto it = std::find(names.begin(), names.end(), parameterEntries[i].Name);
                    if (it != names.end())
                    {
                        *it = updatedName;
                    }
                };

                switch (parameterEntries[i].Type)
                {
                case QEParamType::Bool: renameInList(boolNames); break;
                case QEParamType::Int: renameInList(intNames); break;
                case QEParamType::Float: renameInList(floatNames); break;
                case QEParamType::Trigger: renameInList(triggerNames); break;
                }

                applyParameterEdits(boolNames, intNames, floatNames, triggerNames, transitionsToPatch);
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("X"))
        {
            paramToRemove = static_cast<int>(i);
        }

        ImGui::PopID();
    }

    if (paramToRemove >= 0)
    {
        const auto& param = parameterEntries[paramToRemove];
        auto boolNames = component.GetBoolParameterNames();
        auto intNames = component.GetIntParameterNames();
        auto floatNames = component.GetFloatParameterNames();
        auto triggerNames = component.GetTriggerParameterNames();
        auto transitionsToPatch = removeParameterFromTransitions(param.Name);

        auto removeFromList = [&](std::vector<std::string>& names)
        {
            names.erase(std::remove(names.begin(), names.end(), param.Name), names.end());
        };

        switch (param.Type)
        {
        case QEParamType::Bool: removeFromList(boolNames); break;
        case QEParamType::Int: removeFromList(intNames); break;
        case QEParamType::Float: removeFromList(floatNames); break;
        case QEParamType::Trigger: removeFromList(triggerNames); break;
        }

        applyParameterEdits(boolNames, intNames, floatNames, triggerNames, transitionsToPatch);
    }

    ImGui::SeparatorText("Add");

    const std::array<QEParamType, 4> paramTypes = {
        QEParamType::Bool,
        QEParamType::Int,
        QEParamType::Float,
        QEParamType::Trigger
    };

    if (ImGui::BeginCombo("Type", ToString(sessionGraph.NewParameterType)))
    {
        for (int i = 0; i < static_cast<int>(paramTypes.size()); ++i)
        {
            const bool selected = (paramTypes[i] == sessionGraph.NewParameterType);
            if (ImGui::Selectable(ToString(paramTypes[i]), selected))
            {
                sessionGraph.NewParameterType = paramTypes[i];
            }

            if (selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    if (ImGui::Button("Add Parameter"))
    {
        auto boolNames = component.GetBoolParameterNames();
        auto intNames = component.GetIntParameterNames();
        auto floatNames = component.GetFloatParameterNames();
        auto triggerNames = component.GetTriggerParameterNames();

        switch (sessionGraph.NewParameterType)
        {
        case QEParamType::Bool:
            boolNames.push_back("bool_" + std::to_string(static_cast<int>(boolNames.size()) + 1));
            break;
        case QEParamType::Int:
            intNames.push_back("int_" + std::to_string(static_cast<int>(intNames.size()) + 1));
            break;
        case QEParamType::Float:
            floatNames.push_back("float_" + std::to_string(static_cast<int>(floatNames.size()) + 1));
            break;
        case QEParamType::Trigger:
            triggerNames.push_back("trigger_" + std::to_string(static_cast<int>(triggerNames.size()) + 1));
            break;
        }

        applyParameterEdits(boolNames, intNames, floatNames, triggerNames, component.GetAnimationTransitions());
    }

    ImGui::EndChild();
}

void AnimationGraphPanel::DrawGraphArea(
    const std::shared_ptr<QEGameObject>& gameObject,
    QEAnimationComponent& component,
    SessionGraphState& sessionGraph)
{
    GraphEditor::ViewState viewState;
    viewState.mPosition = ImVec2(sessionGraph.GraphData.ViewState.Scroll.x, sessionGraph.GraphData.ViewState.Scroll.y);
    viewState.mFactor = sessionGraph.GraphData.ViewState.Zoom;
    viewState.mFactorTarget = sessionGraph.GraphData.ViewState.Zoom;

    AnimationGraphDelegate delegate(*this, gameObject, component, sessionGraph);
    GraphEditor::Show(delegate, _graphOptions, viewState, true, &_fitRequest);

    sessionGraph.GraphData.ViewState.Scroll = glm::vec2(viewState.mPosition.x, viewState.mPosition.y);
    sessionGraph.GraphData.ViewState.Zoom = viewState.mFactor;
}

void AnimationGraphPanel::DrawContextMenu(
    const std::shared_ptr<QEGameObject>& gameObject,
    QEAnimationComponent& component,
    SessionGraphState& sessionGraph)
{
    const std::string objectId = gameObject ? gameObject->ID() : std::string{};
    if (_contextMenuState.OpenRequested && _contextMenuState.TargetObjectId == objectId)
    {
        ImGui::OpenPopup(kContextMenuId);
        _contextMenuState.OpenRequested = false;
    }

    if (!ImGui::BeginPopup(kContextMenuId))
        return;

    QEAnimationGraphNode* targetNode = nullptr;
    if (_contextMenuState.NodeIndex >= 0 &&
        _contextMenuState.NodeIndex < static_cast<int>(sessionGraph.GraphData.Nodes.size()))
    {
        targetNode = &sessionGraph.GraphData.Nodes[_contextMenuState.NodeIndex];
    }

    if (!targetNode && sessionGraph.SelectedNodeIds.size() == 1)
    {
        targetNode = FindNodeById(sessionGraph.GraphData, *sessionGraph.SelectedNodeIds.begin());
    }

    if (targetNode && targetNode->Kind == QEAnimationGraphNodeKind::State)
    {
        if (ImGui::MenuItem("Set As Entry"))
        {
            SetEntryNode(sessionGraph, targetNode->StateId);
            ApplyGraphToComponent(component, sessionGraph);
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::MenuItem("Delete State"))
        {
            RemoveStateNode(sessionGraph, targetNode->Id);
            ApplyGraphToComponent(component, sessionGraph);
            ImGui::CloseCurrentPopup();
        }
    }
    else
    {
        if (ImGui::MenuItem("Add State"))
        {
            AddStateNode(sessionGraph);
            ApplyGraphToComponent(component, sessionGraph);
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::MenuItem("Refresh From Runtime"))
        {
            RefreshGraphFromComponent(gameObject, component);
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::MenuItem("Fit All"))
        {
            _fitRequest = GraphEditor::Fit_AllNodes;
            ImGui::CloseCurrentPopup();
        }
    }

    ImGui::EndPopup();
}

void AnimationGraphPanel::DrawPropertiesPane(QEAnimationComponent& component, SessionGraphState& sessionGraph)
{
    const QEAnimationGraphNode* selectedNode = nullptr;
    const QEAnimationGraphNode* selectedStateNode = nullptr;
    const QEAnimationGraphNode* selectedEntryNode = nullptr;
    if (sessionGraph.SelectedNodeIds.size() == 1)
    {
        selectedNode = FindNodeById(sessionGraph.GraphData, *sessionGraph.SelectedNodeIds.begin());
        if (selectedNode && selectedNode->Kind == QEAnimationGraphNodeKind::Entry)
        {
            selectedEntryNode = selectedNode;
        }
        else
        {
            selectedStateNode = selectedNode;
        }
    }

    if (!sessionGraph.SelectedLinkId.empty() && !FindLinkById(sessionGraph.GraphData, sessionGraph.SelectedLinkId))
    {
        sessionGraph.SelectedLinkId.clear();
    }

    ImGui::BeginChild("AnimationGraphProperties", ImVec2(0.0f, 0.0f), true);

    if (selectedEntryNode)
    {
        ImGui::SeparatorText("Entry");

        bool autoStart = component.GetAutoStart();
        if (ImGui::Checkbox("Auto Start", &autoStart))
        {
            component.SetAutoStart(autoStart);
        }

        if (!component.HasStartedPlayback() && ImGui::Button("Start Now"))
        {
            component.StartEntryPlayback();
        }

        ImGui::Text("Target State: %s", selectedEntryNode->StateId.empty() ? "<none>" : selectedEntryNode->StateId.c_str());
        ImGui::SeparatorText("Transitions");
    }
    else if (selectedStateNode)
    {
        ImGui::SeparatorText("State");

        auto states = component.GetAnimationStates();
        auto stateIt = std::find_if(
            states.begin(),
            states.end(),
            [&](const AnimationState& state)
            {
                return state.Id == selectedStateNode->StateId;
            });

        if (stateIt != states.end())
        {
            AnimationState state = *stateIt;
            auto* editableNode = FindNodeById(sessionGraph.GraphData, selectedStateNode->Id);

            char idBuffer[256] = {};
            std::strncpy(idBuffer, state.Id.c_str(), sizeof(idBuffer) - 1);
            if (ImGui::InputText("Id", idBuffer, sizeof(idBuffer)))
            {
                const std::string newStateId = idBuffer;
                if (!newStateId.empty() && newStateId != state.Id && !HasStateId(sessionGraph.GraphData, newStateId))
                {
                    RenameState(sessionGraph, component, state.Id, newStateId);
                    state.Id = newStateId;
                }
            }

            const auto clipNames = component.GetAnimationClipNames();
            if (ImGui::BeginCombo("Animation Clip", state.AnimationClip.empty() ? "<none>" : state.AnimationClip.c_str()))
            {
                for (const auto& clipName : clipNames)
                {
                    const bool selected = (state.AnimationClip == clipName);
                    if (ImGui::Selectable(clipName.c_str(), selected))
                    {
                        state.AnimationClip = clipName;
                        UpdateState(sessionGraph, component, state.Id, state);
                    }

                    if (selected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            char clipBuffer[256] = {};
            std::strncpy(clipBuffer, state.AnimationClip.c_str(), sizeof(clipBuffer) - 1);
            if (ImGui::InputText("Clip Override", clipBuffer, sizeof(clipBuffer)))
            {
                state.AnimationClip = clipBuffer;
                UpdateState(sessionGraph, component, state.Id, state);
            }

            bool loop = state.Loop;
            if (ImGui::Checkbox("Loop", &loop))
            {
                state.Loop = loop;
                UpdateState(sessionGraph, component, state.Id, state);
            }

            if (editableNode)
            {
                int inputSlots = editableNode->InputSlotCount;
                if (ImGui::DragInt("Input Slots", &inputSlots, 0.1f, 1, kMaxStateSlots))
                {
                    editableNode->InputSlotCount = std::clamp(inputSlots, 1, kMaxStateSlots);
                    NormalizeNodeSlotCounts(sessionGraph.GraphData);
                }

                int outputSlots = editableNode->OutputSlotCount;
                if (ImGui::DragInt("Output Slots", &outputSlots, 0.1f, 1, kMaxStateSlots))
                {
                    editableNode->OutputSlotCount = std::clamp(outputSlots, 1, kMaxStateSlots);
                    NormalizeNodeSlotCounts(sessionGraph.GraphData);
                }
            }

            if (ImGui::Button("Delete State"))
            {
                const std::string nodeIdToDelete = selectedStateNode->Id;
                RemoveStateNode(sessionGraph, nodeIdToDelete);
                ApplyGraphToComponent(component, sessionGraph);
                ImGui::EndChild();
                return;
            }
        }
        else
        {
            ImGui::TextUnformatted("Runtime state not found.");
        }
        ImGui::SeparatorText("Transitions");
    }
    else
    {
        ImGui::TextUnformatted("Select an entry or state node to edit it.");
        ImGui::SeparatorText("Transitions");
    }

    std::vector<const QEAnimationGraphLink*> visibleLinks;
    visibleLinks.reserve(sessionGraph.GraphData.Links.size());
    for (const auto& link : sessionGraph.GraphData.Links)
    {
        if (link.FromNodeId == "entry")
            continue;

        if (selectedStateNode)
        {
            if (link.FromStateId != selectedStateNode->StateId)
                continue;
        }

        visibleLinks.push_back(&link);
    }

    if (visibleLinks.empty())
    {
        ImGui::TextUnformatted("No transitions in current scope.");
    }
    else
    {
        for (const QEAnimationGraphLink* link : visibleLinks)
        {
            const std::string label =
                link->FromStateId + " -> " + link->ToStateId + "##" + link->Id;
            if (ImGui::Selectable(label.c_str(), sessionGraph.SelectedLinkId == link->Id))
            {
                sessionGraph.SelectedLinkId = link->Id;
            }
        }
    }

    if (!sessionGraph.SelectedLinkId.empty())
    {
        auto* selectedLink = FindLinkById(sessionGraph.GraphData, sessionGraph.SelectedLinkId);
        if (selectedLink)
        {
            auto transitions = component.GetAnimationTransitions();
            std::unordered_map<std::string, int> ordinalsByPair;

            auto transitionIt = transitions.end();
            for (auto it = transitions.begin(); it != transitions.end(); ++it)
            {
                const std::string pairKey = it->fromState + "->" + it->toState;
                const int ordinal = ordinalsByPair[pairKey]++;
                if (it->fromState == selectedLink->FromStateId &&
                    it->toState == selectedLink->ToStateId &&
                    ordinal == selectedLink->TransitionOrdinal)
                {
                    transitionIt = it;
                    break;
                }
            }

            ImGui::SeparatorText("Transition Properties");

            if (transitionIt != transitions.end())
            {
                QETransition transition = *transitionIt;

                ImGui::Text("From: %s", transition.fromState.c_str());
                ImGui::Text("To: %s", transition.toState.c_str());

                if (ImGui::Button("Delete Transition"))
                {
                    const std::string linkIdToDelete = selectedLink->Id;
                    sessionGraph.GraphData.Links.erase(
                        std::remove_if(
                            sessionGraph.GraphData.Links.begin(),
                            sessionGraph.GraphData.Links.end(),
                            [&](const QEAnimationGraphLink& link)
                            {
                                return link.Id == linkIdToDelete;
                            }),
                        sessionGraph.GraphData.Links.end());
                    sessionGraph.SelectedLinkId.clear();
                    ApplyGraphToComponent(component, sessionGraph);
                    ImGui::EndChild();
                    return;
                }

                int priority = transition.priority;
                if (ImGui::DragInt("Priority", &priority, 1.0f))
                {
                    transition.priority = priority;
                    UpdateTransition(sessionGraph, component, selectedLink->Id, transition);
                }

                float blendDuration = transition.blendDuration;
                if (ImGui::DragFloat("Blend Duration", &blendDuration, 0.01f, 0.0f, 10.0f, "%.2f"))
                {
                    transition.blendDuration = blendDuration;
                    UpdateTransition(sessionGraph, component, selectedLink->Id, transition);
                }

                bool hasExitTime = transition.hasExitTime;
                if (ImGui::Checkbox("Has Exit Time", &hasExitTime))
                {
                    transition.hasExitTime = hasExitTime;
                    UpdateTransition(sessionGraph, component, selectedLink->Id, transition);
                }

                if (transition.hasExitTime)
                {
                    float exitTime = transition.exitTimeNormalized;
                    if (ImGui::SliderFloat("Exit Time", &exitTime, 0.0f, 1.0f, "%.2f"))
                    {
                        transition.exitTimeNormalized = exitTime;
                        UpdateTransition(sessionGraph, component, selectedLink->Id, transition);
                    }
                }

                ImGui::SeparatorText("Conditions");

                int conditionToRemove = -1;
                for (size_t i = 0; i < transition.conditions.size(); ++i)
                {
                    auto& condition = transition.conditions[i];
                    ImGui::PushID(static_cast<int>(i));

                    QEParamType paramType = QEParamType::Float;
                    const bool hasDeclaredType = component.TryGetDeclaredParameterType(condition.param, paramType);
                    const auto declaredParams = BuildDeclaredParameters(component);

                    if (ImGui::BeginCombo("Param", condition.param.empty() ? "<none>" : condition.param.c_str()))
                    {
                        for (const auto& [paramName, declaredType] : declaredParams)
                        {
                            std::string label = paramName + " [";
                            switch (declaredType)
                            {
                            case QEParamType::Bool: label += "Bool"; break;
                            case QEParamType::Int: label += "Int"; break;
                            case QEParamType::Float: label += "Float"; break;
                            case QEParamType::Trigger: label += "Trigger"; break;
                            }
                            label += "]";

                            const bool selected = (condition.param == paramName);
                            if (ImGui::Selectable(label.c_str(), selected))
                            {
                                condition.param = paramName;
                                UpdateTransition(sessionGraph, component, selectedLink->Id, transition);
                            }

                            if (selected)
                            {
                                ImGui::SetItemDefaultFocus();
                            }
                        }
                        ImGui::EndCombo();
                    }

                    char paramBuffer[128] = {};
                    std::strncpy(paramBuffer, condition.param.c_str(), sizeof(paramBuffer) - 1);
                    if (ImGui::InputText("Param Override", paramBuffer, sizeof(paramBuffer)))
                    {
                        condition.param = paramBuffer;
                        UpdateTransition(sessionGraph, component, selectedLink->Id, transition);
                    }

                    int currentOp = 0;
                    for (int opIndex = 0; opIndex < static_cast<int>(kAllOps.size()); ++opIndex)
                    {
                        if (kAllOps[opIndex] == condition.op)
                        {
                            currentOp = opIndex;
                            break;
                        }
                    }

                    if (hasDeclaredType && paramType == QEParamType::Trigger)
                    {
                        ImGui::TextUnformatted("Trigger condition: operator and value are ignored.");
                    }
                    else
                    {
                        if (ImGui::BeginCombo("Operator", ToString(condition.op)))
                        {
                            for (int opIndex = 0; opIndex < static_cast<int>(kAllOps.size()); ++opIndex)
                            {
                                const bool selected = (condition.op == kAllOps[opIndex]);
                                if (ImGui::Selectable(ToString(kAllOps[opIndex]), selected))
                                {
                                    condition.op = kAllOps[opIndex];
                                    UpdateTransition(sessionGraph, component, selectedLink->Id, transition);
                                }

                                if (selected)
                                {
                                    ImGui::SetItemDefaultFocus();
                                }
                            }
                            ImGui::EndCombo();
                        }

                        if (hasDeclaredType && paramType == QEParamType::Bool)
                        {
                            bool boolValue = condition.value >= 0.5f;
                            if (ImGui::Checkbox("Value", &boolValue))
                            {
                                condition.value = boolValue ? 1.0f : 0.0f;
                                UpdateTransition(sessionGraph, component, selectedLink->Id, transition);
                            }
                        }
                        else if (hasDeclaredType && paramType == QEParamType::Int)
                        {
                            int intValue = static_cast<int>(condition.value);
                            if (ImGui::DragInt("Value", &intValue, 1.0f))
                            {
                                condition.value = static_cast<float>(intValue);
                                UpdateTransition(sessionGraph, component, selectedLink->Id, transition);
                            }
                        }
                        else
                        {
                            float value = condition.value;
                            if (ImGui::DragFloat("Value", &value, 0.05f))
                            {
                                condition.value = value;
                                UpdateTransition(sessionGraph, component, selectedLink->Id, transition);
                            }
                        }
                    }

                    if (ImGui::Button("Remove Condition"))
                    {
                        conditionToRemove = static_cast<int>(i);
                    }

                    ImGui::Separator();
                    ImGui::PopID();
                }

                if (conditionToRemove >= 0)
                {
                    transition.conditions.erase(transition.conditions.begin() + conditionToRemove);
                    UpdateTransition(sessionGraph, component, selectedLink->Id, transition);
                }

                if (ImGui::Button("Add Condition"))
                {
                    transition.conditions.push_back(QECondition{});
                    UpdateTransition(sessionGraph, component, selectedLink->Id, transition);
                }
            }
            else
            {
                ImGui::TextUnformatted("Runtime transition not found.");
            }
        }
    }

    ImGui::EndChild();
}

void AnimationGraphPanel::RequestContextMenu(const std::string& objectId, int nodeIndex, int inputSlotIndex, int outputSlotIndex)
{
    _contextMenuState.TargetObjectId = objectId;
    _contextMenuState.NodeIndex = nodeIndex;
    _contextMenuState.InputSlotIndex = inputSlotIndex;
    _contextMenuState.OutputSlotIndex = outputSlotIndex;
    _contextMenuState.OpenRequested = true;
}

void AnimationGraphPanel::ApplyGraphToComponent(QEAnimationComponent& component, SessionGraphState& sessionGraph)
{
    RemoveDanglingLinks(sessionGraph);
    NormalizeNodeSlotCounts(sessionGraph.GraphData);
    NormalizeLinks(sessionGraph.GraphData);
    QEAnimationGraphRuntimeAdapter::ApplyEditorData(component, sessionGraph.GraphData);
}

void AnimationGraphPanel::RefreshGraphFromComponent(const std::shared_ptr<QEGameObject>& gameObject, QEAnimationComponent& component)
{
    if (!gameObject)
        return;

    auto& sessionGraph = _sessionGraphs[gameObject->ID()];
    sessionGraph.GraphData = QEAnimationGraphRuntimeAdapter::BuildEditorData(component, &sessionGraph.GraphData);
    NormalizeNodeSlotCounts(sessionGraph.GraphData);

    std::unordered_set<std::string> validNodeIds;
    for (const auto& node : sessionGraph.GraphData.Nodes)
    {
        validNodeIds.insert(node.Id);
    }

    for (auto it = sessionGraph.SelectedNodeIds.begin(); it != sessionGraph.SelectedNodeIds.end();)
    {
        if (!validNodeIds.contains(*it))
            it = sessionGraph.SelectedNodeIds.erase(it);
        else
            ++it;
    }

    if (!sessionGraph.SelectedLinkId.empty() && !FindLinkById(sessionGraph.GraphData, sessionGraph.SelectedLinkId))
    {
        sessionGraph.SelectedLinkId.clear();
    }
}

void AnimationGraphPanel::AddStateNode(SessionGraphState& sessionGraph)
{
    const std::string stateId = GenerateUniqueStateId(sessionGraph);
    ++_stateCounter;
    const int index = static_cast<int>(sessionGraph.GraphData.Nodes.size());

    QEAnimationGraphNode node;
    node.Id = MakeStateNodeId(stateId);
    node.Kind = QEAnimationGraphNodeKind::State;
    node.Title = stateId;
    node.StateId = stateId;
    node.Position = glm::vec2(
        120.0f + static_cast<float>(index % 4) * 280.0f,
        static_cast<float>(index / 4) * 180.0f);

    sessionGraph.GraphData.Nodes.push_back(node);
    sessionGraph.SelectedNodeIds.clear();
    sessionGraph.SelectedNodeIds.insert(node.Id);

    auto* entryNode = FindEntryNode(sessionGraph.GraphData);
    if (entryNode && entryNode->StateId.empty())
    {
        SetEntryNode(sessionGraph, stateId);
    }
}

void AnimationGraphPanel::RemoveStateNode(SessionGraphState& sessionGraph, const std::string& nodeId)
{
    const QEAnimationGraphNode* node = FindNodeById(sessionGraph.GraphData, nodeId);
    if (!node || node->Kind != QEAnimationGraphNodeKind::State)
        return;

    const std::string stateId = node->StateId;

    sessionGraph.GraphData.Nodes.erase(
        std::remove_if(
            sessionGraph.GraphData.Nodes.begin(),
            sessionGraph.GraphData.Nodes.end(),
            [&](const QEAnimationGraphNode& candidate)
            {
                return candidate.Id == nodeId;
            }),
        sessionGraph.GraphData.Nodes.end());

    sessionGraph.SelectedNodeIds.erase(nodeId);

    if (const auto* entryNode = FindEntryNode(sessionGraph.GraphData);
        entryNode && entryNode->StateId == stateId)
    {
        std::string nextEntryStateId;
        for (const auto& candidate : sessionGraph.GraphData.Nodes)
        {
            if (candidate.Kind == QEAnimationGraphNodeKind::State)
            {
                nextEntryStateId = candidate.StateId;
                break;
            }
        }
        SetEntryNode(sessionGraph, nextEntryStateId);
    }

    RemoveDanglingLinks(sessionGraph);
}

void AnimationGraphPanel::SetEntryNode(SessionGraphState& sessionGraph, const std::string& stateId)
{
    auto* entryNode = FindEntryNode(sessionGraph.GraphData);
    if (!entryNode)
        return;

    entryNode->StateId = stateId;

    sessionGraph.GraphData.Links.erase(
        std::remove_if(
            sessionGraph.GraphData.Links.begin(),
            sessionGraph.GraphData.Links.end(),
            [](const QEAnimationGraphLink& link)
            {
                return link.FromNodeId == "entry";
            }),
        sessionGraph.GraphData.Links.end());

    if (stateId.empty())
        return;

    QEAnimationGraphLink entryLink;
    entryLink.Id = "link:entry";
    entryLink.FromNodeId = "entry";
    entryLink.ToNodeId = MakeStateNodeId(stateId);
    entryLink.ToStateId = stateId;
    entryLink.TargetSlot = FindNextFreeTargetSlot(sessionGraph.GraphData, entryLink.ToNodeId);
    sessionGraph.GraphData.Links.insert(sessionGraph.GraphData.Links.begin(), entryLink);
}

void AnimationGraphPanel::RemoveDanglingLinks(SessionGraphState& sessionGraph)
{
    std::unordered_set<std::string> validNodeIds;
    std::unordered_set<std::string> validStateIds;

    for (const auto& node : sessionGraph.GraphData.Nodes)
    {
        validNodeIds.insert(node.Id);
        if (node.Kind == QEAnimationGraphNodeKind::State)
        {
            validStateIds.insert(node.StateId);
        }
    }

    sessionGraph.GraphData.Links.erase(
        std::remove_if(
            sessionGraph.GraphData.Links.begin(),
            sessionGraph.GraphData.Links.end(),
            [&](const QEAnimationGraphLink& link)
            {
                if (!validNodeIds.contains(link.FromNodeId) || !validNodeIds.contains(link.ToNodeId))
                    return true;

                if (link.FromNodeId == "entry")
                    return !validStateIds.contains(link.ToStateId);

                return !validStateIds.contains(link.FromStateId) || !validStateIds.contains(link.ToStateId);
            }),
        sessionGraph.GraphData.Links.end());

    auto* entryNode = FindEntryNode(sessionGraph.GraphData);
    if (entryNode && !entryNode->StateId.empty() && !validStateIds.contains(entryNode->StateId))
    {
        entryNode->StateId.clear();
    }

    if (!sessionGraph.SelectedLinkId.empty() && !FindLinkById(sessionGraph.GraphData, sessionGraph.SelectedLinkId))
    {
        sessionGraph.SelectedLinkId.clear();
    }
}

void AnimationGraphPanel::RenameState(SessionGraphState& sessionGraph, QEAnimationComponent& component, const std::string& oldStateId, const std::string& newStateId)
{
    if (oldStateId.empty() || newStateId.empty() || oldStateId == newStateId)
        return;

    if (HasStateId(sessionGraph.GraphData, newStateId))
        return;

    auto states = component.GetAnimationStates();
    auto transitions = component.GetAnimationTransitions();
    std::string entryStateId = FindEntryNode(sessionGraph.GraphData) ? FindEntryNode(sessionGraph.GraphData)->StateId : std::string{};

    for (auto& state : states)
    {
        if (state.Id == oldStateId)
        {
            state.Id = newStateId;
            break;
        }
    }

    for (auto& transition : transitions)
    {
        if (transition.fromState == oldStateId)
            transition.fromState = newStateId;
        if (transition.toState == oldStateId)
            transition.toState = newStateId;
    }

    for (auto& node : sessionGraph.GraphData.Nodes)
    {
        if (node.Kind == QEAnimationGraphNodeKind::State && node.StateId == oldStateId)
        {
            const std::string oldNodeId = node.Id;
            node.StateId = newStateId;
            node.Id = MakeStateNodeId(newStateId);
            node.Title = newStateId;

            if (sessionGraph.SelectedNodeIds.erase(oldNodeId) > 0)
            {
                sessionGraph.SelectedNodeIds.insert(node.Id);
            }
        }
    }

    for (auto& link : sessionGraph.GraphData.Links)
    {
        if (link.FromStateId == oldStateId)
        {
            link.FromStateId = newStateId;
            link.FromNodeId = MakeStateNodeId(newStateId);
        }
        if (link.ToStateId == oldStateId)
        {
            link.ToStateId = newStateId;
            link.ToNodeId = MakeStateNodeId(newStateId);
        }
    }

    if (entryStateId == oldStateId)
    {
        entryStateId = newStateId;
        SetEntryNode(sessionGraph, newStateId);
    }

    NormalizeLinks(sessionGraph.GraphData);
    component.SetStateMachineData(states, transitions, entryStateId);
}

void AnimationGraphPanel::UpdateState(SessionGraphState& sessionGraph, QEAnimationComponent& component, const std::string& stateId, const AnimationState& updatedState)
{
    auto states = component.GetAnimationStates();
    auto transitions = component.GetAnimationTransitions();
    std::string entryStateId = FindEntryNode(sessionGraph.GraphData) ? FindEntryNode(sessionGraph.GraphData)->StateId : std::string{};

    for (auto& state : states)
    {
        if (state.Id == stateId)
        {
            state = updatedState;
            break;
        }
    }

    component.SetStateMachineData(states, transitions, entryStateId);
}

void AnimationGraphPanel::UpdateTransition(SessionGraphState& sessionGraph, QEAnimationComponent& component, const std::string& linkId, const QETransition& updatedTransition)
{
    auto states = component.GetAnimationStates();
    auto transitions = component.GetAnimationTransitions();
    std::string entryStateId = FindEntryNode(sessionGraph.GraphData) ? FindEntryNode(sessionGraph.GraphData)->StateId : std::string{};
    auto* link = FindLinkById(sessionGraph.GraphData, linkId);
    if (!link)
        return;

    std::unordered_map<std::string, int> ordinalsByPair;
    for (auto& transition : transitions)
    {
        const std::string pairKey = transition.fromState + "->" + transition.toState;
        const int ordinal = ordinalsByPair[pairKey]++;
        if (transition.fromState == link->FromStateId &&
            transition.toState == link->ToStateId &&
            ordinal == link->TransitionOrdinal)
        {
            transition = updatedTransition;
            break;
        }
    }

    component.SetStateMachineData(states, transitions, entryStateId);
}

std::string AnimationGraphPanel::GenerateUniqueStateId(const SessionGraphState& sessionGraph) const
{
    std::unordered_set<std::string> usedIds;
    for (const auto& node : sessionGraph.GraphData.Nodes)
    {
        if (node.Kind == QEAnimationGraphNodeKind::State)
        {
            usedIds.insert(node.StateId);
        }
    }

    int suffix = _stateCounter;
    while (true)
    {
        const std::string candidate = "State_" + std::to_string(suffix);
        if (!usedIds.contains(candidate))
            return candidate;

        ++suffix;
    }
}
