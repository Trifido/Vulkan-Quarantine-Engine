#pragma once
#ifndef BONE_IMPORTER_H
#define BONE_IMPORTER_H

#include <glm/gtx/quaternion.hpp>
#include <vector>
#include <string>
#include <assimp/anim.h>

namespace YAML {
    template <typename T>
    struct convert;   // forward declaration
}

struct KeyPosition
{
    glm::vec3 position;
    float timeStamp;
};

struct KeyRotation
{
    glm::quat orientation;
    float timeStamp;
};

struct KeyScale
{
    glm::vec3 scale;
    float timeStamp;
};

struct BoneTRS {
    glm::vec3 t{ 0,0,0 };
    glm::quat r{ 1,0,0,0 };   // identidad
    glm::vec3 s{ 1,1,1 };     // identidad
    bool valid{ false };
};

class Bone
{
private:
    std::vector<KeyPosition> m_Positions;
    std::vector<KeyRotation> m_Rotations;
    std::vector<KeyScale> m_Scales;
    int m_NumPositions;
    int m_NumRotations;
    int m_NumScalings;

    glm::mat4 m_LocalTransform;
    std::string m_Name;
    int m_ID;

private:
    float GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime) const;
    glm::mat4 InterpolatePosition(float animationTime);
    glm::mat4 InterpolateRotation(float animationTime);
    glm::mat4 InterpolateScaling(float animationTime);
public:
    Bone();
    Bone(const std::string& name, int ID, const aiNodeAnim* channel);

    BoneTRS SampleTRS(float animationTime) const;
    glm::vec3 SamplePosition(float t) const;
    glm::quat SampleRotation(float t) const;
    glm::vec3 SampleScale(float t) const;

    void Update(float animationTime);
    glm::mat4 GetLocalTransform();
    std::string GetBoneName();
    int GetBoneID();
    int GetPositionIndex(float animationTime) const;
    int GetRotationIndex(float animationTime) const;
    int GetScaleIndex(float animationTime) const;

    friend struct YAML::convert<Bone>;
};

#endif
