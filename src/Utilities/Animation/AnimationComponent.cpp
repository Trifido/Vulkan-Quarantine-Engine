#include "AnimationComponent.h"
#include <QEGameObject.h>

AnimationComponent::AnimationComponent()
{
    this->animator = std::make_shared<Animator>();
}

void AnimationComponent::AddAnimation(std::shared_ptr<Animation> animation_ptr)
{
    if (this->_animations.find(animation_ptr->name) == _animations.end())
    {
        this->_animations[animation_ptr->name] = animation_ptr;
    }
    else
    {
        this->_animations[animation_ptr->name + "_1"] = animation_ptr;
    }

    this->numAnimations++;

    animationVector.push_back(animation_ptr);
}

void AnimationComponent::AddAnimation(Animation animation)
{
    if (this->_animations.find(animation.name) == _animations.end())
    {
        this->_animations[animation.name] = std::make_shared<Animation>(animation);
    }
    else
    {
        this->_animations[animation.name + "_1"] = std::make_shared<Animation>(animation);
    }

    this->numAnimations++;

    animationVector.push_back(std::make_shared<Animation>(animation));
}

std::shared_ptr<Animation> AnimationComponent::GetAnimation(std::string name)
{
    auto it = _animations.find(name);

    if (it != _animations.end())
        return it->second;

    return nullptr;
}

void AnimationComponent::ChangeAnimation()
{
    idAnimation++;
    idAnimation %= numAnimations;

    this->animator->ChangeAnimation(animationVector.at(idAnimation));
}

void AnimationComponent::CleanLastResources()
{
    if (this->animator != nullptr)
    {
        this->animator->CleanAnimatorUBO();
        this->animator.reset();
        this->animator = nullptr;

        this->_animations.clear();
        this->numAnimations = 0;
    }
}

void AnimationComponent::QEStart()
{
    auto animationComponent = this->Owner->GetComponent<AnimationComponent>();
    if (animationComponent != nullptr)
    {
        auto meshComponent = this->Owner->GetComponent<QEGeometryComponent>();
        auto mesh = meshComponent->GetMesh();

        if (mesh->MeshData.empty())
        {
            return;
        }

        std::vector<std::string> idMeshes;
        for (unsigned int i = 0; i < mesh->MeshData.size(); i++)
        {
            idMeshes.push_back(std::to_string(i));
        }

        animationComponent->animator->InitializeComputeNodes(idMeshes);

        for (unsigned int i = 0; i < mesh->MeshData.size(); i++)
        {
            animationComponent->animator->SetVertexBufferInComputeNode(idMeshes[i], meshComponent->vertexBuffer[i], meshComponent->animationBuffer[i], mesh->MeshData[i].NumVertices);
        }

        animationComponent->animator->InitializeDescriptorsComputeNodes();
    }

    QEGameComponent::QEStart();
}

void AnimationComponent::QEUpdate()
{
}

void AnimationComponent::QEDestroy()
{
    QEGameComponent::QEDestroy();
}

void AnimationComponent::QEInit()
{
    auto animation = this->_animations.begin()->second;
    this->animator->PlayAnimation(animation);

    QEGameComponent::QEInit();
}
