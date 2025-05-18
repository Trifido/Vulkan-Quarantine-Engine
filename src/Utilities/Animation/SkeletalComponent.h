#pragma once
#ifndef SKELETAL_COMPONENT_H
#define SKELETAL_COMPONENT_H

#include <QEGameComponent.h>
#include <AnimationResources.h>

class SkeletalComponent : QEGameComponent
{
public:
    size_t numBones;
    std::unordered_map<std::string, BoneInfo> m_BoneInfoMap;

public:
    void QEStart() override;
    void QEUpdate() override;
    void QERelease() override;
};

#endif
