#pragma once
#ifndef SKELETAL_COMPONENT_H
#define SKELETAL_COMPONENT_H

#include <GameComponent.h>
#include <map>
#include <AnimationResources.h>

class SkeletalComponent : GameComponent
{
public:
    size_t numBones;
    std::map<std::string, BoneInfo> m_BoneInfoMap;
};

#endif
