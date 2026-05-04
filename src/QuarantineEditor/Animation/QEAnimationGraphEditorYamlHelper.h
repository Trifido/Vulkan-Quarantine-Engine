#pragma once

#ifndef QE_ANIMATION_GRAPH_EDITOR_YAML_HELPER_H
#define QE_ANIMATION_GRAPH_EDITOR_YAML_HELPER_H

#include <yaml-cpp/yaml.h>

#include <QuarantineEditor/Animation/QEAnimationGraphEditorData.h>
#include <glm_yaml_conversions.h>

namespace QEAnimationGraphYaml
{
    inline const char* ToString(QEAnimationGraphNodeKind kind)
    {
        switch (kind)
        {
        case QEAnimationGraphNodeKind::Entry:
            return "Entry";
        case QEAnimationGraphNodeKind::State:
            return "State";
        }

        return "State";
    }

    inline QEAnimationGraphNodeKind NodeKindFromString(const std::string& value)
    {
        if (value == "Entry")
            return QEAnimationGraphNodeKind::Entry;

        return QEAnimationGraphNodeKind::State;
    }
}

namespace YAML
{
    template<>
    struct convert<QEAnimationGraphNode>
    {
        static Node encode(const QEAnimationGraphNode& rhs)
        {
            Node node;
            node["Id"] = rhs.Id;
            node["Kind"] = QEAnimationGraphYaml::ToString(rhs.Kind);
            node["Title"] = rhs.Title;
            node["StateId"] = rhs.StateId;
            node["Position"] = rhs.Position;
            node["Size"] = rhs.Size;
            node["InputSlotCount"] = rhs.InputSlotCount;
            node["OutputSlotCount"] = rhs.OutputSlotCount;
            return node;
        }

        static bool decode(const Node& node, QEAnimationGraphNode& rhs)
        {
            if (!node || !node.IsMap() || !node["Id"])
                return false;

            rhs.Id = node["Id"].as<std::string>();
            rhs.Kind = node["Kind"]
                ? QEAnimationGraphYaml::NodeKindFromString(node["Kind"].as<std::string>())
                : QEAnimationGraphNodeKind::State;
            rhs.Title = node["Title"] ? node["Title"].as<std::string>() : "";
            rhs.StateId = node["StateId"] ? node["StateId"].as<std::string>() : "";
            rhs.Position = node["Position"] ? node["Position"].as<glm::vec2>() : glm::vec2(0.0f);
            rhs.Size = node["Size"] ? node["Size"].as<glm::vec2>() : glm::vec2(220.0f, 110.0f);
            rhs.InputSlotCount = node["InputSlotCount"] ? node["InputSlotCount"].as<int>() : 1;
            rhs.OutputSlotCount = node["OutputSlotCount"] ? node["OutputSlotCount"].as<int>() : 1;
            return true;
        }
    };

    template<>
    struct convert<QEAnimationGraphLink>
    {
        static Node encode(const QEAnimationGraphLink& rhs)
        {
            Node node;
            node["Id"] = rhs.Id;
            node["FromNodeId"] = rhs.FromNodeId;
            node["ToNodeId"] = rhs.ToNodeId;
            node["FromStateId"] = rhs.FromStateId;
            node["ToStateId"] = rhs.ToStateId;
            node["SourceSlot"] = rhs.SourceSlot;
            node["TargetSlot"] = rhs.TargetSlot;
            node["TransitionOrdinal"] = rhs.TransitionOrdinal;
            return node;
        }

        static bool decode(const Node& node, QEAnimationGraphLink& rhs)
        {
            if (!node || !node.IsMap() || !node["Id"])
                return false;

            rhs.Id = node["Id"].as<std::string>();
            rhs.FromNodeId = node["FromNodeId"] ? node["FromNodeId"].as<std::string>() : "";
            rhs.ToNodeId = node["ToNodeId"] ? node["ToNodeId"].as<std::string>() : "";
            rhs.FromStateId = node["FromStateId"] ? node["FromStateId"].as<std::string>() : "";
            rhs.ToStateId = node["ToStateId"] ? node["ToStateId"].as<std::string>() : "";
            rhs.SourceSlot = node["SourceSlot"] ? node["SourceSlot"].as<int>() : 0;
            rhs.TargetSlot = node["TargetSlot"] ? node["TargetSlot"].as<int>() : 0;
            rhs.TransitionOrdinal = node["TransitionOrdinal"] ? node["TransitionOrdinal"].as<int>() : 0;
            return true;
        }
    };

    template<>
    struct convert<QEAnimationGraphViewState>
    {
        static Node encode(const QEAnimationGraphViewState& rhs)
        {
            Node node;
            node["Scroll"] = rhs.Scroll;
            node["Zoom"] = rhs.Zoom;
            return node;
        }

        static bool decode(const Node& node, QEAnimationGraphViewState& rhs)
        {
            if (!node || !node.IsMap())
                return false;

            rhs.Scroll = node["Scroll"] ? node["Scroll"].as<glm::vec2>() : glm::vec2(0.0f);
            rhs.Zoom = node["Zoom"] ? node["Zoom"].as<float>() : 1.0f;
            return true;
        }
    };

    template<>
    struct convert<QEAnimationGraphEditorData>
    {
        static Node encode(const QEAnimationGraphEditorData& rhs)
        {
            Node node;
            node["Version"] = rhs.Version;
            node["ViewState"] = rhs.ViewState;
            node["Nodes"] = rhs.Nodes;
            node["Links"] = rhs.Links;
            return node;
        }

        static bool decode(const Node& node, QEAnimationGraphEditorData& rhs)
        {
            if (!node || !node.IsMap())
                return false;

            rhs.Version = node["Version"] ? node["Version"].as<int>() : 1;
            rhs.ViewState = node["ViewState"] ? node["ViewState"].as<QEAnimationGraphViewState>() : QEAnimationGraphViewState{};
            rhs.Nodes = node["Nodes"] ? node["Nodes"].as<std::vector<QEAnimationGraphNode>>() : std::vector<QEAnimationGraphNode>{};
            rhs.Links = node["Links"] ? node["Links"].as<std::vector<QEAnimationGraphLink>>() : std::vector<QEAnimationGraphLink>{};
            return true;
        }
    };
}

#endif // !QE_ANIMATION_GRAPH_EDITOR_YAML_HELPER_H
