#pragma once

#ifndef QE_ANIMATION_GRAPH_EDITOR_DATA_H
#define QE_ANIMATION_GRAPH_EDITOR_DATA_H

#include <glm/glm.hpp>

#include <string>
#include <vector>

enum class QEAnimationGraphNodeKind
{
    Entry,
    State
};

struct QEAnimationGraphNode
{
    std::string Id;
    QEAnimationGraphNodeKind Kind = QEAnimationGraphNodeKind::State;

    // Human-facing label shown in the editor node.
    std::string Title;

    // For state nodes this maps directly to AnimationState::Id.
    // For the entry node it represents the target state.
    std::string StateId;

    // Layout-only editor data.
    glm::vec2 Position = glm::vec2(0.0f);
    glm::vec2 Size = glm::vec2(220.0f, 110.0f);
};

struct QEAnimationGraphLink
{
    std::string Id;

    // Node references used by the graph widget.
    std::string FromNodeId;
    std::string ToNodeId;

    // State references used to map the link back to runtime transitions.
    std::string FromStateId;
    std::string ToStateId;

    int SourceSlot = 0;
    int TargetSlot = 0;

    // Order hint for repeated transitions between the same states.
    int TransitionOrdinal = 0;
};

struct QEAnimationGraphViewState
{
    glm::vec2 Scroll = glm::vec2(0.0f);
    float Zoom = 1.0f;
};

struct QEAnimationGraphEditorData
{
    int Version = 1;
    QEAnimationGraphViewState ViewState;
    std::vector<QEAnimationGraphNode> Nodes;
    std::vector<QEAnimationGraphLink> Links;
};

#endif // !QE_ANIMATION_GRAPH_EDITOR_DATA_H
