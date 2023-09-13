#include "Animator.h"
#include <SynchronizationModule.h>
#include <ShaderManager.h>

Animator::Animator()
{
    this->deviceModule = DeviceModule::getInstance();
    this->computeNodeManager = ComputeNodeManager::getInstance();

	m_CurrentTime = 0.0;
    m_FinalBoneMatrices = std::make_shared<std::vector<glm::mat4>>(std::vector<glm::mat4>(NUM_BONES, glm::mat4(1.0f)));
    //this->animationUniform_ptr = std::make_shared<AnimationUniform>();

    //this->computeNode->FillComputeBuffer(this->numParticles, sizeof(PBRVertex), particles.data());
}

//void Animator::AssignAnimationBuffer(std::shared_ptr<DescriptorBuffer> descriptorBuffer)
//{
//    descriptorBuffer->animationUBO = this->animationUBO;
//    descriptorBuffer->animationUniformSize = this->animationUniformSize;
//}

//void Animator::InitializeUBOAnimation(std::shared_ptr<ShaderModule> shader_ptr)
//{
    //auto reflect = shader_ptr->reflectShader;

    //this->animationUBO = std::make_shared<UniformBufferObject>();

    //size_t position = 0;
    //this->animationbuffer = new char[reflect.animationBufferSize];

    //for each (auto name in reflect.animationUBOComponents)
    //{
    //    if (name == "finalBonesMatrices")
    //    {
    //        this->WriteToAnimationBuffer(reinterpret_cast<char*>(&this->animationUniform_ptr->finalBonesMatrices), position, reflect.animationBufferSize);
    //    }
    //}

    //this->animationUBO->CreateUniformBuffer(reflect.animationBufferSize, MAX_FRAMES_IN_FLIGHT, *deviceModule);
    //this->UpdateUBOAnimation();
    
//}

void Animator::InitializeComputeNodes(std::vector<std::string> idChilds)
{
    auto shaderManager = ShaderManager::getInstance();
    for (uint32_t i = 0; i < idChilds.size(); i++)
    {
        this->computeNodes[idChilds[i]] = std::make_shared<ComputeNode>(shaderManager->GetShader("default_skinning"));
        this->computeNodeManager->AddComputeNode("default_skinning", this->computeNodes[idChilds[i]]);
    }
}

void Animator::UpdateUBOAnimation()
{
    auto currentFrame = SynchronizationModule::GetCurrentFrame();
    //void* data;
    //vkMapMemory(deviceModule->device, this->animationUBO->uniformBuffersMemory[currentFrame], 0, this->animationUniformSize, 0, &data);
    //memcpy(data, static_cast<const void*>(this->m_FinalBoneMatrices.get()->data()), this->animationUniformSize);
    //vkUnmapMemory(deviceModule->device, this->animationUBO->uniformBuffersMemory[currentFrame]);

    //for each (auto cn in computeNodes)
    //{
    //    void* data;
    //    vkMapMemory(deviceModule->device, cn.second->computeDescriptor->ubos["UniformAnimation"]->uniformBuffersMemory[currentFrame], 0, cn.second->computeDescriptor->uboSizes["UniformAnimation"], 0, &data);
    //    memcpy(data, static_cast<const void*>(this->m_FinalBoneMatrices.get()->data()), cn.second->computeDescriptor->uboSizes["UniformAnimation"]);
    //    vkUnmapMemory(deviceModule->device, cn.second->computeDescriptor->ubos["UniformAnimation"]->uniformBuffersMemory[currentFrame]);
    //}
}

void Animator::SetVertexBufferInComputeNode(std::string id, VkBuffer vertexBuffer, uint32_t numElements)
{
    this->computeNodes[id]->NElements = numElements;
    this->computeNodes[id]->computeDescriptor->InitializeSSBOData();

    uint32_t numSSBOs = this->computeNodes[id]->computeDescriptor->ssboData.size();

    uint32_t bufferSize = sizeof(PBRAnimationVertex) * numElements;
    this->computeNodes[id]->computeDescriptor->ssboData[0]->CreateSSBO(bufferSize, MAX_FRAMES_IN_FLIGHT, *deviceModule);
    this->computeNodes[id]->computeDescriptor->ssboSize[0] = bufferSize;

    bufferSize = sizeof(AnimationVertex) * numElements;
    this->computeNodes[id]->computeDescriptor->ssboData[1]->CreateSSBO(bufferSize, MAX_FRAMES_IN_FLIGHT, *deviceModule);
    this->computeNodes[id]->computeDescriptor->ssboSize[1] = bufferSize;

    this->computeNodes[id]->FillComputeBuffer(vertexBuffer, bufferSize);

    for (int currentFrame = 0; currentFrame < MAX_FRAMES_IN_FLIGHT; currentFrame++)
    {
        void* data;
        vkMapMemory(deviceModule->device, this->computeNodes[id]->computeDescriptor->ubos["UniformVertexParam"]->uniformBuffersMemory[currentFrame], 0, this->computeNodes[id]->computeDescriptor->uboSizes["UniformVertexParam"], 0, &data);
        memcpy(data, static_cast<const void*>(&this->computeNodes[id]->NElements), this->computeNodes[id]->computeDescriptor->uboSizes["UniformVertexParam"]);
        vkUnmapMemory(deviceModule->device, this->computeNodes[id]->computeDescriptor->ubos["UniformVertexParam"]->uniformBuffersMemory[currentFrame]);
    }

    for (int i = 0; i < 200; i++)
    {
        this->m_FinalBoneMatrices->push_back(glm::mat4(1.0f));
    }

    for (int currentFrame = 0; currentFrame < MAX_FRAMES_IN_FLIGHT; currentFrame++)
    {
        void* data;
        vkMapMemory(deviceModule->device, this->computeNodes[id]->computeDescriptor->ubos["UniformAnimation"]->uniformBuffersMemory[currentFrame], 0, this->computeNodes[id]->computeDescriptor->uboSizes["UniformAnimation"], 0, &data);
        memcpy(data, static_cast<const void*>(this->m_FinalBoneMatrices.get()->data()), this->computeNodes[id]->computeDescriptor->uboSizes["UniformAnimation"]);
        vkUnmapMemory(deviceModule->device, this->computeNodes[id]->computeDescriptor->ubos["UniformAnimation"]->uniformBuffersMemory[currentFrame]);
    }
}

//void Animator::WriteToAnimationBuffer(char* data, size_t& position, const size_t& sizeToCopy)
//{
//    memcpy(&animationbuffer[position], data, sizeToCopy);
//    position += sizeToCopy;
//    this->animationUniformSize += sizeToCopy;
//}

std::shared_ptr<ComputeNode> Animator::GetComputeNode(std::string id)
{
    return this->computeNodes[id];
}

void Animator::UpdateAnimation(float dt)
{
    //m_DeltaTime = dt;
    //if (m_CurrentAnimation != nullptr)
    //{
    //    m_CurrentTime += m_CurrentAnimation->GetTicksPerSecond() * dt;
    //    m_CurrentTime = fmod(m_CurrentTime, m_CurrentAnimation->GetDuration());
    //    CalculateBoneTransform(&m_CurrentAnimation->GetRootNode(), glm::mat4(1.0f));
    //    this->UpdateUBOAnimation();
    //}
}

void Animator::PlayAnimation(std::shared_ptr<Animation> pAnimation)
{
    m_CurrentAnimation = pAnimation;
    m_CurrentTime = 0.0f;
}

void Animator::CalculateBoneTransform(const AnimationNode* node, glm::mat4 parentTransform)
{
    std::string nodeName = node->name;
    auxiliarBone = m_CurrentAnimation->FindBone(nodeName);

    glm::mat4 nodeTransform = node->transformation;

    if (auxiliarBone)
    {
        auxiliarBone->Update(m_CurrentTime);
        nodeTransform = auxiliarBone->GetLocalTransform();
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

void Animator::CleanAnimatorUBO()
{
    //for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    //{
    //    // Material UBO
    //    //if (this->animationUBO != nullptr)
    //    //{
    //    //    vkDestroyBuffer(deviceModule->device, this->animationUBO->uniformBuffers[i], nullptr);
    //    //    vkFreeMemory(deviceModule->device, this->animationUBO->uniformBuffersMemory[i], nullptr);
    //    //}
    //}

    this->auxiliarBone = nullptr;

    //this->animationUBO.reset();
    //this->animationUBO = nullptr;

    //delete [] this->animationbuffer;
    //this->animationbuffer = nullptr;

    m_FinalBoneMatrices.reset();
    m_FinalBoneMatrices = nullptr;

    m_CurrentAnimation.reset();
    m_CurrentAnimation = nullptr;
}
