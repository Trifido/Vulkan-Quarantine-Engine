#include "AnimationComponent.h"

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
