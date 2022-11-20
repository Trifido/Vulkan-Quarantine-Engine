#include "Animator.h"

Animator::Animator()
{
	m_CurrentTime = 0.0;
    m_FinalBoneMatrices = std::make_shared<std::vector<glm::mat4>>(std::vector<glm::mat4>(100, glm::mat4(1.0f)));
    this->animationUniform_ptr = std::make_shared<AnimationUniform>();
}

void Animator::AddDescriptor(std::shared_ptr<DescriptorModule> descriptor)
{
    this->descriptors.push_back(descriptor);
    descriptors.back()->animationUniform = this->animationUniform_ptr;
}

void Animator::UpdateAnimation(float dt)
{
    m_DeltaTime = dt;
    if (m_CurrentAnimation != nullptr)
    {
        m_CurrentTime += m_CurrentAnimation->GetTicksPerSecond() * dt;
        m_CurrentTime = fmod(m_CurrentTime, m_CurrentAnimation->GetDuration());
        CalculateBoneTransform(&m_CurrentAnimation->GetRootNode(), glm::mat4(1.0f));

        //for(int i = 0; i < 100; i++)
        //    this->animationUniform_ptr->finalBonesMatrices[i] = m_FinalBoneMatrices->at(i);

        //memcpy(this->animationUniform_ptr->finalBonesMatrices, m_FinalBoneMatrices.get(), 100);
        //this->animationUniform_ptr->finalBonesMatrices[i] = m_FinalBoneMatrices->at(i);
    }
}

void Animator::PlayAnimation(std::shared_ptr<Animation> pAnimation)
{
    m_CurrentAnimation = pAnimation;
    m_CurrentTime = 0.0f;
}

void Animator::CalculateBoneTransform(const AnimationNode* node, glm::mat4 parentTransform)
{
    std::string nodeName = node->name;
    glm::mat4 nodeTransform = node->transformation;

    Bone* Bone = m_CurrentAnimation->FindBone(nodeName);

    if (Bone)
    {
        Bone->Update(m_CurrentTime);
        nodeTransform = Bone->GetLocalTransform();
    }

    glm::mat4 globalTransformation = parentTransform * nodeTransform;

    auto boneInfoMap = m_CurrentAnimation->GetBoneIDMap();
    if (boneInfoMap.find(nodeName) != boneInfoMap.end())
    {
        int index = boneInfoMap[nodeName].id;
        if (index < 100)
        {
            glm::mat4 offset = boneInfoMap[nodeName].offset;
            m_FinalBoneMatrices->at(index) = globalTransformation * offset;
        }
    }

    for (int i = 0; i < node->childrenCount; i++)
        CalculateBoneTransform(&node->children[i], globalTransformation);
}

std::shared_ptr<std::vector<glm::mat4>> Animator::GetFinalBoneMatrices()
{
    return m_FinalBoneMatrices;
}
