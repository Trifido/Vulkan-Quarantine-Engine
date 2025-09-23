#pragma once
#ifndef BONE_IMPORTER_H
#define BONE_IMPORTER_H

#include <glm/gtx/quaternion.hpp>
#include <vector>
#include <string>
#include <assimp/anim.h>

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
    float GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime);
    glm::mat4 InterpolatePosition(float animationTime);
    glm::mat4 InterpolateRotation(float animationTime);
    glm::mat4 InterpolateScaling(float animationTime);
public:
    Bone();
    Bone(const std::string& name, int ID, const aiNodeAnim* channel);
    void Update(float animationTime);
    glm::mat4 GetLocalTransform();
    std::string GetBoneName();
    int GetBoneID();
    int GetPositionIndex(float animationTime);
    int GetRotationIndex(float animationTime);
    int GetScaleIndex(float animationTime);

    friend struct YAML::convert<Bone>;
};

#endif
