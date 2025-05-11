#pragma once
#ifndef ANIMATION_H
#define ANIMATION_H

#include <map>
#include <Bone.h>
#include <AnimationResources.h>

class Animation
{
public:
    AnimationData animationData;
    std::string name;

public:
    Animation(const AnimationData& animData);
    inline float GetTicksPerSecond() { return static_cast<float>(animationData.m_TicksPerSecond); }
    inline float GetDuration() { return static_cast<float>(animationData.m_Duration); }
    inline const AnimationNode& GetRootNode() { return animationData.animationNodeData; }
    Bone* FindBone(const std::string& name);
};

#endif
