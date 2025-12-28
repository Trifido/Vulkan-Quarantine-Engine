#pragma once

#ifndef ANIMATION_YAML_HELPER_H
#define ANIMATION_YAML_HELPER_H

#include <yaml-cpp/yaml.h>
#include <glm/glm.hpp>
#include "glm_yaml_conversions.h"
#include <QEAnimationResources.h>
#include <Bone.h>
#include <QEAnimationStateData.h>

namespace QEAnimYaml
{
    // --- QEOp <-> string ---
    inline const char* ToString(QEOp op)
    {
        switch (op) {
        case QEOp::Equal:        return "Equal";
        case QEOp::NotEqual:     return "NotEqual";
        case QEOp::Greater:      return "Greater";
        case QEOp::Less:         return "Less";
        case QEOp::GreaterEqual: return "GreaterEqual";
        case QEOp::LessEqual:    return "LessEqual";
        }
        return "Equal";
    }

    inline QEOp OpFromString(const std::string& s)
    {
        if (s == "Equal")        return QEOp::Equal;
        if (s == "NotEqual")     return QEOp::NotEqual;
        if (s == "Greater")      return QEOp::Greater;
        if (s == "Less")         return QEOp::Less;
        if (s == "GreaterEqual") return QEOp::GreaterEqual;
        if (s == "LessEqual")    return QEOp::LessEqual;
        return QEOp::Equal;
    }
}

namespace YAML
{
    // --- AnimationNode ---
    template<>
    struct convert<AnimationNode>
    {
        static Node encode(const AnimationNode& an)
        {
            Node node;
            node["name"] = an.name;
            node["transformation"] = an.transformation; // usa tu convert<glm::mat4>
            node["childrenCount"] = an.childrenCount;

            Node childrenNode;
            for (const auto& c : an.children)
            {
                childrenNode.push_back(c); // recursivo: convert<AnimationNode> será usado
            }
            node["children"] = childrenNode;
            return node;
        }

        static bool decode(const Node& node, AnimationNode& an)
        {
            if (!node.IsMap()) return false;

            if (node["name"]) an.name = node["name"].as<std::string>();
            if (node["transformation"]) an.transformation = node["transformation"].as<glm::mat4>();
            if (node["childrenCount"]) an.childrenCount = node["childrenCount"].as<int>();

            an.children.clear();
            if (node["children"] && node["children"].IsSequence())
            {
                for (const auto& cnode : node["children"])
                {
                    AnimationNode child = cnode.as<AnimationNode>(); // recursión
                    an.children.push_back(std::move(child));
                }
            }
            return true;
        }
    };

    // --- BoneInfo ---
    template<>
    struct convert<BoneInfo>
    {
        static Node encode(const BoneInfo& b)
        {
            Node node;
            node["id"] = b.id;
            node["offset"] = b.offset; // usa tu convert<glm::mat4>
            return node;
        }

        static bool decode(const Node& node, BoneInfo& b)
        {
            if (!node.IsMap()) return false;

            if (node["id"])
            {
                b.id = node["id"].as<int>();
            }
            else
            {
                b.id = -1; // o el valor por defecto que prefieras
            }

            if (node["offset"])
            {
                b.offset = node["offset"].as<glm::mat4>();
            }
            else {
                b.offset = glm::mat4(1.0f);
            }

            return true;
        }
    };

    // --- Key types ---
    template<> struct convert<KeyPosition> {
        static Node encode(const KeyPosition& kp) {
            Node n;
            n["position"] = kp.position;  // vec3
            n["timeStamp"] = kp.timeStamp; // float
            return n;
        }
        static bool decode(const Node& n, KeyPosition& kp) {
            if (!n.IsMap()) return false;
            if (n["position"])  kp.position = n["position"].as<glm::vec3>();
            if (n["timeStamp"]) kp.timeStamp = n["timeStamp"].as<float>();
            return true;
        }
    };

    template<> struct convert<KeyRotation> {
        static Node encode(const KeyRotation& kr) {
            Node n;
            n["orientation"] = kr.orientation; // quat (guardas [x,y,z,w], decod: {w,x,y,z})
            n["timeStamp"] = kr.timeStamp;
            return n;
        }
        static bool decode(const Node& n, KeyRotation& kr) {
            if (!n.IsMap()) return false;
            if (n["orientation"]) kr.orientation = n["orientation"].as<glm::quat>();
            if (n["timeStamp"])   kr.timeStamp = n["timeStamp"].as<float>();
            return true;
        }
    };

    template<> struct convert<KeyScale> {
        static Node encode(const KeyScale& ks) {
            Node n;
            n["scale"] = ks.scale;     // vec3
            n["timeStamp"] = ks.timeStamp; // float
            return n;
        }
        static bool decode(const Node& n, KeyScale& ks) {
            if (!n.IsMap()) return false;
            if (n["scale"])     ks.scale = n["scale"].as<glm::vec3>();
            if (n["timeStamp"]) ks.timeStamp = n["timeStamp"].as<float>();
            return true;
        }
    };

    template<> struct convert<Bone> {
        static Node encode(const Bone& b) {
            Node n;
            n["name"] = b.m_Name;
            n["id"] = b.m_ID;
            n["localTransform"] = b.m_LocalTransform; // mat4

            // keyframes
            Node positions;
            positions.SetStyle(EmitterStyle::Block);
            for (const auto& kp : b.m_Positions) positions.push_back(kp);
            n["positions"] = positions;

            Node rotations;
            rotations.SetStyle(EmitterStyle::Block);
            for (const auto& kr : b.m_Rotations) rotations.push_back(kr);
            n["rotations"] = rotations;

            Node scales;
            scales.SetStyle(EmitterStyle::Block);
            for (const auto& ks : b.m_Scales) scales.push_back(ks);
            n["scales"] = scales;

            // Los contadores son derivados; si quieres, puedes guardarlos también:
            n["numPositions"] = static_cast<int>(b.m_Positions.size());
            n["numRotations"] = static_cast<int>(b.m_Rotations.size());
            n["numScalings"] = static_cast<int>(b.m_Scales.size());

            return n;
        }

        static bool decode(const Node& n, Bone& b) {
            if (!n.IsMap()) return false;

            if (n["name"])           b.m_Name = n["name"].as<std::string>();
            if (n["id"])             b.m_ID = n["id"].as<int>();
            if (n["localTransform"]) b.m_LocalTransform = n["localTransform"].as<glm::mat4>();

            b.m_Positions.clear();
            if (n["positions"] && n["positions"].IsSequence()) {
                for (const auto& item : n["positions"]) {
                    b.m_Positions.push_back(item.as<KeyPosition>());
                }
            }

            b.m_Rotations.clear();
            if (n["rotations"] && n["rotations"].IsSequence()) {
                for (const auto& item : n["rotations"]) {
                    b.m_Rotations.push_back(item.as<KeyRotation>());
                }
            }

            b.m_Scales.clear();
            if (n["scales"] && n["scales"].IsSequence()) {
                for (const auto& item : n["scales"]) {
                    b.m_Scales.push_back(item.as<KeyScale>());
                }
            }

            // Recalcular contadores (más robusto que confiar en YAML):
            b.m_NumPositions = static_cast<int>(b.m_Positions.size());
            b.m_NumRotations = static_cast<int>(b.m_Rotations.size());
            b.m_NumScalings = static_cast<int>(b.m_Scales.size());

            return true;
        }
    };

    template<> struct convert<AnimationState> {
        static Node encode(const AnimationState& rhs)
        {
            Node node;
            node["Id"] = rhs.Id;
            node["Loop"] = rhs.Loop;
            node["AnimationClip"] = rhs.AnimationClip;
            return node;
        }

        static bool decode(const Node& node, AnimationState& rhs)
        {
            if (!node || !node.IsMap()) return false;

            // Id es obligatorio
            if (!node["Id"]) return false;
            rhs.Id = node["Id"].as<std::string>();

            // opcionales con default
            rhs.Loop = node["Loop"] ? node["Loop"].as<bool>() : true;
            rhs.AnimationClip = node["AnimationClip"] ? node["AnimationClip"].as<std::string>() : "";

            return true;
        }
    };

    template<>
    struct convert<QECondition>
    {
        static Node encode(const QECondition& rhs)
        {
            Node node;
            node["param"] = rhs.param;
            node["op"] = QEAnimYaml::ToString(rhs.op);
            node["value"] = rhs.value;
            return node;
        }

        static bool decode(const Node& node, QECondition& rhs)
        {
            if (!node || !node.IsMap()) return false;
            if (!node["param"] || !node["op"]) return false;

            rhs.param = node["param"].as<std::string>();
            rhs.op = QEAnimYaml::OpFromString(node["op"].as<std::string>());
            rhs.value = node["value"] ? node["value"].as<float>() : 0.0f;
            return true;
        }
    };

    template<>
    struct convert<QETransition>
    {
        static Node encode(const QETransition& rhs)
        {
            Node node;
            node["from"] = rhs.fromState;
            node["to"] = rhs.toState;
            node["priority"] = rhs.priority;
            node["hasExitTime"] = rhs.hasExitTime;
            node["exitTimeNormalized"] = rhs.exitTimeNormalized;

            // conditions: seq
            Node conds(NodeType::Sequence);
            for (const auto& c : rhs.conditions) conds.push_back(c); // uses convert<QECondition>
            node["conditions"] = conds;

            return node;
        }

        static bool decode(const Node& node, QETransition& rhs)
        {
            if (!node || !node.IsMap()) return false;
            if (!node["from"] || !node["to"]) return false;

            rhs.fromState = node["from"].as<std::string>();
            rhs.toState = node["to"].as<std::string>();
            rhs.priority = node["priority"] ? node["priority"].as<int>() : 0;
            rhs.hasExitTime = node["hasExitTime"] ? node["hasExitTime"].as<bool>() : false;
            rhs.exitTimeNormalized = node["exitTimeNormalized"] ? node["exitTimeNormalized"].as<float>() : 1.0f;

            rhs.conditions.clear();
            if (auto conds = node["conditions"]; conds && conds.IsSequence())
            {
                rhs.conditions = conds.as<std::vector<QECondition>>(); // uses convert<QECondition>
            }

            return true;
        }
    };
}

#endif // !ANIMATION_YAML_HELPER_H
