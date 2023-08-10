#include "Animator.h"
#include <SynchronizationModule.h>

Animator::Animator()
{
    this->deviceModule = DeviceModule::getInstance();
	m_CurrentTime = 0.0;
    m_FinalBoneMatrices = std::make_shared<std::vector<glm::mat4>>(std::vector<glm::mat4>(NUM_BONES, glm::mat4(1.0f)));
    this->animationUniform_ptr = std::make_shared<AnimationUniform>();
}

void Animator::InitializeUBOAnimation(std::shared_ptr<ShaderModule> shader_ptr)
{
    auto reflect = shader_ptr->reflectShader;

    if (reflect.isUboAnimation)
    {
        this->animationUBO = std::make_shared<UniformBufferObject>();

        size_t position = 0;
        this->animationbuffer = new char[reflect.animationBufferSize];

        for each (auto name in reflect.animationUBOComponents)
        {
            if (name == "finalBonesMatrices")
            {
                this->WriteToAnimationBuffer(this->animationbuffer, position, reflect.animationBufferSize);
            }
        }

        this->animationUBO->CreateUniformBuffer(reflect.animationBufferSize, MAX_FRAMES_IN_FLIGHT, *deviceModule);
        this->UpdateUBOAnimation();
    }
}

void Animator::UpdateUBOAnimation()
{
    auto currentFrame = SynchronizationModule::GetCurrentFrame();
    void* data;
    vkMapMemory(deviceModule->device, this->animationUBO->uniformBuffersMemory[currentFrame], 0, this->animationUniformSize, 0, &data);
    memcpy(data, this->animationbuffer, this->animationUniformSize);
    vkUnmapMemory(deviceModule->device, this->animationUBO->uniformBuffersMemory[currentFrame]);
}

void Animator::WriteToAnimationBuffer(char* data, size_t& position, const size_t& sizeToCopy)
{
    memcpy(&animationbuffer[position], data, sizeToCopy);
    position += sizeToCopy;
    this->animationUniformSize += sizeToCopy;
}

void Animator::AddDescriptor(std::shared_ptr<DescriptorModule> descriptor)
{
    //this->descriptors.push_back(descriptor);
    //descriptors.back()->animationUniform = this->animationUniform_ptr;
}

void Animator::UpdateAnimation(float dt)
{
    m_DeltaTime = dt;
    if (m_CurrentAnimation != nullptr)
    {
        m_CurrentTime += m_CurrentAnimation->GetTicksPerSecond() * dt;
        m_CurrentTime = fmod(m_CurrentTime, m_CurrentAnimation->GetDuration());
        CalculateBoneTransform(&m_CurrentAnimation->GetRootNode(), glm::mat4(1.0f));

        memcpy(this->animationUniform_ptr->finalBonesMatrices, this->m_FinalBoneMatrices.get()->data(), sizeof(glm::mat4) * NUM_BONES);
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
    Bone* Bone = m_CurrentAnimation->FindBone(nodeName);                                    // Cuidado con este puntero, no se libera

    glm::mat4 nodeTransform = node->transformation;

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
        if (index < NUM_BONES)
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

void Animator::ChangeAnimation(std::shared_ptr<Animation> newAnimation)
{
    this->m_CurrentTime = 0.0f;
    this->m_DeltaTime = 0.0f;
    this->m_CurrentAnimation = newAnimation;
}
