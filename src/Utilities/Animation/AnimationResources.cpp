#include "QEAnimationResources.h"
#include <AnimationYamlHelper.h>

YAML::Node AnimationData::Serialize(const AnimationData& data)
{
    YAML::Node node;
    node["animationName"] = data.animationName;
    node["m_Duration"] = data.m_Duration;
    node["m_TicksPerSecond"] = data.m_TicksPerSecond;

    // serializar m_BoneInfoMap
    {
        YAML::Node mapNode;
        for (const auto& [name, info] : data.m_BoneInfoMap) {
            mapNode[name] = info; // usa convert<BoneInfo>
        }
        node["m_BoneInfoMap"] = mapNode;
    }

    // serializar m_Bones
    {
        YAML::Node mapNode;
        for (const auto& [name, bone] : data.m_Bones) {
            mapNode[name] = bone; // usa convert<Bone>
        }
        node["m_Bones"] = mapNode;
    }

    // serializar animationNodeData
    node["animationNodeData"] = data.animationNodeData; // usa convert<AnimationNode>

    return node;
}

AnimationData AnimationData::Deserialize(const YAML::Node& node)
{
    AnimationData data;
    if (!node || !node.IsMap()) return data;

    if (node["animationName"])
        data.animationName = node["animationName"].as<std::string>();

    if (node["m_Duration"])
        data.m_Duration = node["m_Duration"].as<double>();

    if (node["m_TicksPerSecond"])
        data.m_TicksPerSecond = node["m_TicksPerSecond"].as<double>();

    // deserializar m_BoneInfoMap
    if (node["m_BoneInfoMap"] && node["m_BoneInfoMap"].IsMap()) {
        for (auto it : node["m_BoneInfoMap"]) {
            std::string name = it.first.as<std::string>();
            BoneInfo info = it.second.as<BoneInfo>();
            data.m_BoneInfoMap[name] = info;
        }
    }

    // deserializar m_Bones
    if (node["m_Bones"] && node["m_Bones"].IsMap()) {
        for (auto it : node["m_Bones"]) {
            std::string name = it.first.as<std::string>();
            Bone bone = it.second.as<Bone>();
            data.m_Bones[name] = bone;
        }
    }

    // deserializar animationNodeData
    if (node["animationNodeData"]) {
        data.animationNodeData = node["animationNodeData"].as<AnimationNode>();
    }

    return data;
}
