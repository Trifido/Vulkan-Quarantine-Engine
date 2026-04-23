#include "QEAnimationGraphRuntimeAdapter.h"

#include <QEAnimationComponent.h>

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{
    struct TransitionKey
    {
        std::string FromStateId;
        std::string ToStateId;
        int Ordinal = 0;

        bool operator==(const TransitionKey& other) const
        {
            return FromStateId == other.FromStateId &&
                ToStateId == other.ToStateId &&
                Ordinal == other.Ordinal;
        }
    };

    struct TransitionKeyHasher
    {
        size_t operator()(const TransitionKey& key) const
        {
            std::hash<std::string> hashString;
            std::hash<int> hashInt;

            size_t seed = hashString(key.FromStateId);
            seed ^= hashString(key.ToStateId) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= hashInt(key.Ordinal) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            return seed;
        }
    };

    std::string MakeStateNodeId(const std::string& stateId)
    {
        return "state:" + stateId;
    }

    std::string MakeLinkId(const std::string& fromStateId, const std::string& toStateId, int ordinal)
    {
        return "link:" + fromStateId + ":" + toStateId + ":" + std::to_string(ordinal);
    }

    std::unordered_map<std::string, QEAnimationGraphNode> BuildPreviousNodeMap(const QEAnimationGraphEditorData* previousEditorData)
    {
        std::unordered_map<std::string, QEAnimationGraphNode> previousNodes;
        if (!previousEditorData)
            return previousNodes;

        for (const auto& node : previousEditorData->Nodes)
        {
            previousNodes[node.Id] = node;
        }

        return previousNodes;
    }
}

namespace QEAnimationGraphRuntimeAdapter
{
    QEAnimationGraphEditorData BuildEditorData(
        const QEAnimationComponent& component,
        const QEAnimationGraphEditorData* previousEditorData)
    {
        QEAnimationGraphEditorData editorData;
        if (previousEditorData)
        {
            editorData.ViewState = previousEditorData->ViewState;
        }

        const auto previousNodes = BuildPreviousNodeMap(previousEditorData);
        const auto& states = component.GetAnimationStates();
        const auto& transitions = component.GetAnimationTransitions();
        const AnimationState currentState = component.GetCurrentState();

        QEAnimationGraphNode entryNode;
        entryNode.Id = "entry";
        entryNode.Kind = QEAnimationGraphNodeKind::Entry;
        entryNode.Title = "Entry";
        entryNode.StateId = currentState.Id;
        entryNode.Position = glm::vec2(-340.0f, 0.0f);
        entryNode.Size = glm::vec2(180.0f, 90.0f);

        if (auto it = previousNodes.find(entryNode.Id); it != previousNodes.end())
        {
            entryNode.Position = it->second.Position;
            entryNode.Size = it->second.Size;
        }

        editorData.Nodes.push_back(entryNode);

        for (size_t i = 0; i < states.size(); ++i)
        {
            const auto& state = states[i];

            QEAnimationGraphNode node;
            node.Id = MakeStateNodeId(state.Id);
            node.Kind = QEAnimationGraphNodeKind::State;
            node.Title = state.Id;
            node.StateId = state.Id;
            node.Position = glm::vec2(
                120.0f + static_cast<float>(i % 4) * 280.0f,
                static_cast<float>(i / 4) * 180.0f);

            if (auto it = previousNodes.find(node.Id); it != previousNodes.end())
            {
                node.Position = it->second.Position;
                node.Size = it->second.Size;
            }

            editorData.Nodes.push_back(node);
        }

        if (!currentState.Id.empty())
        {
            QEAnimationGraphLink entryLink;
            entryLink.Id = MakeLinkId("entry", currentState.Id, 0);
            entryLink.FromNodeId = "entry";
            entryLink.ToNodeId = MakeStateNodeId(currentState.Id);
            entryLink.ToStateId = currentState.Id;
            editorData.Links.push_back(entryLink);
        }

        std::unordered_map<std::string, int> transitionOrdinals;
        std::unordered_map<std::string, int> sourceSlotsByState;
        std::unordered_map<std::string, int> targetSlotsByState;
        for (const auto& transition : transitions)
        {
            const std::string ordinalKey = transition.fromState + "->" + transition.toState;
            const int ordinal = transitionOrdinals[ordinalKey]++;

            QEAnimationGraphLink link;
            link.Id = MakeLinkId(transition.fromState, transition.toState, ordinal);
            link.FromNodeId = MakeStateNodeId(transition.fromState);
            link.ToNodeId = MakeStateNodeId(transition.toState);
            link.FromStateId = transition.fromState;
            link.ToStateId = transition.toState;
            link.SourceSlot = sourceSlotsByState[transition.fromState]++;
            link.TargetSlot = targetSlotsByState[transition.toState]++;
            link.TransitionOrdinal = ordinal;
            editorData.Links.push_back(link);
        }

        return editorData;
    }

    void ApplyEditorData(
        QEAnimationComponent& component,
        const QEAnimationGraphEditorData& editorData)
    {
        std::unordered_map<std::string, AnimationState> existingStates;
        for (const auto& state : component.GetAnimationStates())
        {
            existingStates[state.Id] = state;
        }

        std::unordered_map<TransitionKey, QETransition, TransitionKeyHasher> existingTransitions;
        std::unordered_map<std::string, int> transitionOrdinals;
        for (const auto& transition : component.GetAnimationTransitions())
        {
            const std::string ordinalKey = transition.fromState + "->" + transition.toState;
            const int ordinal = transitionOrdinals[ordinalKey]++;
            existingTransitions[{transition.fromState, transition.toState, ordinal}] = transition;
        }

        std::vector<AnimationState> states;
        states.reserve(editorData.Nodes.size());

        std::string entryStateId;
        for (const auto& node : editorData.Nodes)
        {
            if (node.Kind == QEAnimationGraphNodeKind::Entry)
            {
                entryStateId = node.StateId;
                continue;
            }

            if (node.StateId.empty())
                continue;

            AnimationState state;
            if (auto it = existingStates.find(node.StateId); it != existingStates.end())
            {
                state = it->second;
            }
            else
            {
                state.Id = node.StateId;
                state.AnimationClip = node.StateId;
                state.Loop = true;
            }

            if (state.Id.empty())
            {
                state.Id = node.StateId;
            }

            states.push_back(state);
        }

        if (entryStateId.empty() && !states.empty())
        {
            entryStateId = states.front().Id;
        }

        std::vector<QETransition> transitions;
        transitions.reserve(editorData.Links.size());

        for (const auto& link : editorData.Links)
        {
            if (link.FromNodeId == "entry")
                continue;

            if (link.FromStateId.empty() || link.ToStateId.empty())
                continue;

            const TransitionKey key{ link.FromStateId, link.ToStateId, link.TransitionOrdinal };

            QETransition transition;
            if (auto it = existingTransitions.find(key); it != existingTransitions.end())
            {
                transition = it->second;
            }
            else
            {
                transition.fromState = link.FromStateId;
                transition.toState = link.ToStateId;
            }

            transition.fromState = link.FromStateId;
            transition.toState = link.ToStateId;
            transitions.push_back(transition);
        }

        component.SetStateMachineData(states, transitions, entryStateId);
    }
}
