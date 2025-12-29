#include "Bone.h"

static glm::mat4 ComposeTRS(const BoneTRS& trs)
{
    return glm::translate(glm::mat4(1.0f), trs.t) * glm::toMat4(trs.r) * glm::scale(glm::mat4(1.0f), trs.s);
}

float Bone::GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime) const
{
    float framesDiff = nextTimeStamp - lastTimeStamp;
    if (std::abs(framesDiff) < 1e-8f) return 0.0f;
    return (animationTime - lastTimeStamp) / framesDiff;
}

glm::mat4 Bone::InterpolatePosition(float animationTime)
{
    if (m_NumPositions == 1)
        return glm::translate(glm::mat4(1.0f), m_Positions[0].position);

    int p0Index = GetPositionIndex(animationTime);
    int p1Index = std::min(p0Index + 1, m_NumPositions - 1);

    assert(p0Index >= 0 && p0Index < m_NumPositions);
    assert(p1Index >= 0 && p1Index < m_NumPositions);

    float denom = (m_Positions[p1Index].timeStamp - m_Positions[p0Index].timeStamp);
    float scaleFactor = (denom != 0.0f) ? (animationTime - m_Positions[p0Index].timeStamp) / denom : 0.0f;

    glm::vec3 finalPosition = glm::mix(m_Positions[p0Index].position, m_Positions[p1Index].position, scaleFactor);
    return glm::translate(glm::mat4(1.0f), finalPosition);
}

glm::mat4 Bone::InterpolateRotation(float animationTime)
{
    if (1 == m_NumRotations)
    {
        auto rotation = glm::normalize(m_Rotations[0].orientation);
        return glm::toMat4(rotation);
    }

    int p0Index = GetRotationIndex(animationTime);
    int p1Index = std::min(p0Index + 1, m_NumRotations - 1);
    float scaleFactor = GetScaleFactor(m_Rotations[p0Index].timeStamp,
        m_Rotations[p1Index].timeStamp, animationTime);
    glm::quat finalRotation = glm::slerp(m_Rotations[p0Index].orientation,
        m_Rotations[p1Index].orientation, scaleFactor);
    finalRotation = glm::normalize(finalRotation);
    return glm::toMat4(finalRotation);
}

glm::mat4 Bone::InterpolateScaling(float animationTime)
{
    if (1 == m_NumScalings)
        return glm::scale(glm::mat4(1.0f), m_Scales[0].scale);

    int p0Index = GetScaleIndex(animationTime);
    int p1Index = std::min(p0Index + 1, m_NumScalings - 1);
    float scaleFactor = GetScaleFactor(m_Scales[p0Index].timeStamp,
        m_Scales[p1Index].timeStamp, animationTime);
    glm::vec3 finalScale = glm::mix(m_Scales[p0Index].scale, m_Scales[p1Index].scale
        , scaleFactor);
    return glm::scale(glm::mat4(1.0f), finalScale);
}

Bone::Bone()
{
}

Bone::Bone(const std::string& name, int ID, const aiNodeAnim* channel) :
	m_Name(name),
	m_ID(ID),
	m_LocalTransform(1.0f)
{
	m_NumPositions = channel->mNumPositionKeys;

	for (int positionIndex = 0; positionIndex < m_NumPositions; ++positionIndex)
	{
		aiVector3D aiPosition = channel->mPositionKeys[positionIndex].mValue;
		float timeStamp = static_cast<float>(channel->mPositionKeys[positionIndex].mTime);
		KeyPosition data;
		data.position = glm::vec3(aiPosition.x, aiPosition.y, aiPosition.z);
		data.timeStamp = timeStamp;
		m_Positions.push_back(data);
	}

	m_NumRotations = channel->mNumRotationKeys;
	for (int rotationIndex = 0; rotationIndex < m_NumRotations; ++rotationIndex)
	{
		aiQuaternion aiOrientation = channel->mRotationKeys[rotationIndex].mValue;
		float timeStamp = static_cast<float>(channel->mRotationKeys[rotationIndex].mTime);
		KeyRotation data;
		data.orientation = glm::quat(aiOrientation.w, aiOrientation.x, aiOrientation.y, aiOrientation.z);
		data.timeStamp = timeStamp;
		m_Rotations.push_back(data);
	}

	m_NumScalings = channel->mNumScalingKeys;
	for (int keyIndex = 0; keyIndex < m_NumScalings; ++keyIndex)
	{
		aiVector3D scale = channel->mScalingKeys[keyIndex].mValue;
		float timeStamp = static_cast<float>(channel->mScalingKeys[keyIndex].mTime);
		KeyScale data;
		data.scale = glm::vec3(scale.x, scale.y, scale.z);
		data.timeStamp = timeStamp;
		m_Scales.push_back(data);
	}
}

void Bone::Update(float animationTime)
{
    glm::mat4 translation = InterpolatePosition(animationTime);
    glm::mat4 rotation = InterpolateRotation(animationTime);
    glm::mat4 scale = InterpolateScaling(animationTime);
    m_LocalTransform = translation * rotation * scale;
}

glm::mat4 Bone::GetLocalTransform()
{
    return m_LocalTransform;
}

std::string Bone::GetBoneName()
{
    return m_Name;
}

int Bone::GetBoneID()
{
    return m_ID;
}

int Bone::GetPositionIndex(float animationTime) const
{
    for (int index = 0; index < m_NumPositions - 1; ++index)
    {
        if (animationTime < m_Positions[index + 1].timeStamp)
            return index;
    }

    return m_NumPositions - 1;
}

int Bone::GetRotationIndex(float animationTime) const
{
    for (int index = 0; index < m_NumRotations - 1; ++index)
    {
        if (animationTime < m_Rotations[index + 1].timeStamp)
            return index;
    }

    return m_NumRotations - 1;
}

int Bone::GetScaleIndex(float animationTime) const
{
    for (int index = 0; index < m_NumScalings - 1; ++index)
    {
        if (animationTime < m_Scales[index + 1].timeStamp)
            return index;
    }

    return m_NumScalings - 1;
}

glm::vec3 Bone::SamplePosition(float animationTime) const
{
    if (m_NumPositions == 1) return m_Positions[0].position;

    int p0 = GetPositionIndex(animationTime);
    int p1 = std::min(p0 + 1, m_NumPositions - 1);
    float f = GetScaleFactor(m_Positions[p0].timeStamp, m_Positions[p1].timeStamp, animationTime);
    return glm::mix(m_Positions[p0].position, m_Positions[p1].position, f);
}

glm::quat Bone::SampleRotation(float animationTime) const
{
    if (m_NumRotations == 1) return glm::normalize(m_Rotations[0].orientation);

    int r0 = GetRotationIndex(animationTime);
    int r1 = std::min(r0 + 1, m_NumRotations - 1);
    float f = GetScaleFactor(m_Rotations[r0].timeStamp, m_Rotations[r1].timeStamp, animationTime);

    glm::quat a = m_Rotations[r0].orientation;
    glm::quat b = m_Rotations[r1].orientation;

    // evita el “camino largo”
    if (glm::dot(a, b) < 0.0f) b = -b;

    return glm::normalize(glm::slerp(a, b, f));
}

glm::vec3 Bone::SampleScale(float animationTime) const
{
    if (m_NumScalings == 1) return m_Scales[0].scale;

    int s0 = GetScaleIndex(animationTime);
    int s1 = std::min(s0 + 1, m_NumScalings - 1);
    float f = GetScaleFactor(m_Scales[s0].timeStamp, m_Scales[s1].timeStamp, animationTime);
    return glm::mix(m_Scales[s0].scale, m_Scales[s1].scale, f);
}

BoneTRS Bone::SampleTRS(float animationTime) const {
    BoneTRS out;
    out.t = SamplePosition(animationTime);
    out.r = SampleRotation(animationTime);
    out.s = SampleScale(animationTime);
    out.valid = true;
    return out;
}
