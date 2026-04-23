#pragma once

#ifndef ANIMATION_GRAPH_PANEL_H
#define ANIMATION_GRAPH_PANEL_H

#include "IEditorPanel.h"

#include <Editor/Animation/QEAnimationGraphEditorData.h>
#include <GraphEditor.h>
#include <QEAnimationStateData.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

struct EditorContext;
class EditorSelectionManager;
class QEGameObject;
class QEAnimationComponent;

class AnimationGraphPanel : public IEditorPanel
{
public:
    struct SessionGraphState
    {
        QEAnimationGraphEditorData GraphData;
        std::unordered_set<std::string> SelectedNodeIds;
        std::string SelectedLinkId;
    };

    AnimationGraphPanel(
        EditorContext* editorContext,
        EditorSelectionManager* selectionManager);

    const char* GetName() const override { return "Animation Graph"; }
    void Draw() override;

    void RequestContextMenu(const std::string& objectId, int nodeIndex, int inputSlotIndex, int outputSlotIndex);
    void ApplyGraphToComponent(QEAnimationComponent& component, SessionGraphState& sessionGraph);
    void SetEntryNode(SessionGraphState& sessionGraph, const std::string& stateId);

private:
    struct ContextMenuState
    {
        bool OpenRequested = false;
        std::string TargetObjectId;
        int NodeIndex = -1;
        int InputSlotIndex = -1;
        int OutputSlotIndex = -1;
    };

    EditorContext* editorContext = nullptr;
    EditorSelectionManager* selectionManager = nullptr;

    std::unordered_map<std::string, SessionGraphState> _sessionGraphs;
    ContextMenuState _contextMenuState;
    std::string _pendingSelectNodeId;
    int _stateCounter = 1;
    GraphEditor::Options _graphOptions;
    GraphEditor::FitOnScreen _fitRequest = GraphEditor::Fit_None;

private:
    SessionGraphState& GetOrCreateSessionGraph(const std::shared_ptr<QEGameObject>& gameObject, QEAnimationComponent& component);
    void DrawToolbar(const std::shared_ptr<QEGameObject>& gameObject, QEAnimationComponent& component, SessionGraphState& sessionGraph);
    void DrawGraphArea(const std::shared_ptr<QEGameObject>& gameObject, QEAnimationComponent& component, SessionGraphState& sessionGraph);
    void DrawContextMenu(const std::shared_ptr<QEGameObject>& gameObject, QEAnimationComponent& component, SessionGraphState& sessionGraph);
    void DrawPropertiesPane(QEAnimationComponent& component, SessionGraphState& sessionGraph);
    void RefreshGraphFromComponent(const std::shared_ptr<QEGameObject>& gameObject, QEAnimationComponent& component);

    void AddStateNode(SessionGraphState& sessionGraph);
    void RemoveStateNode(SessionGraphState& sessionGraph, const std::string& nodeId);
    void RemoveDanglingLinks(SessionGraphState& sessionGraph);
    void RenameState(SessionGraphState& sessionGraph, QEAnimationComponent& component, const std::string& oldStateId, const std::string& newStateId);
    void UpdateState(SessionGraphState& sessionGraph, QEAnimationComponent& component, const std::string& stateId, const AnimationState& updatedState);
    void UpdateTransition(SessionGraphState& sessionGraph, QEAnimationComponent& component, const std::string& linkId, const QETransition& updatedTransition);

    std::string GenerateUniqueStateId(const SessionGraphState& sessionGraph) const;
};

#endif // !ANIMATION_GRAPH_PANEL_H
