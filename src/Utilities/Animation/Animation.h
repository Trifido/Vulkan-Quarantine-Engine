#pragma once
#ifndef ANIMATION_H
#define ANIMATION_H

#include <map>
#include <Bone.h>
#include <QEAnimationResources.h>

class Animation
{
public:
    AnimationData animationData;
    std::string name;

public:
    Animation(const AnimationData& animData);
    float GetTicksPerSecond() const;
    float GetDuration() const;
    const AnimationNode& GetRootNode() const;
    Bone* FindBone(const std::string& name);
    const Bone* FindBone(const std::string& name) const;
};

#endif
