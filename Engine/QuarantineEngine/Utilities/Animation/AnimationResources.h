#pragma once

#ifndef ANIMATION_RESOURCES_IMPORTER_H
#define ANIMATION_RESOURCES_IMPORTER_H

#include <glm/glm.hpp>
#include <Bone.h>

struct BoneInfo
{
    int id;
    glm::mat4 offset;
};
struct AnimationNode
{
    glm::mat4 transformation;
    std::string name;
    int childrenCount;
    std::vector<AnimationNode> children;
};

struct AnimationData
{
    std::map<std::string, BoneInfo> m_BoneInfoMap;
    std::vector<Bone> m_Bones;
    AnimationNode animationNodeData;
    double m_Duration = 0.0;
    double m_TicksPerSecond = 0.0;
};

static inline glm::mat4 ConvertMatrixToGLMFormat(const aiMatrix4x4& from)
{
    glm::mat4 to;
    //the a,b,c,d in assimp is the row ; the 1,2,3,4 is the column
    to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
    to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
    to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
    to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
    return to;
}

#endif
