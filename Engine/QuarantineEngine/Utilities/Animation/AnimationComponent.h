#pragma once

#ifndef ANIMATION_COMPONENT_H
#define ANIMATION_COMPONENT_H
#include "GameComponent.h"
#include <Animation/Animation.h>
#include <Animator.h>

class AnimationComponent : GameComponent
{
public:
    std::shared_ptr<Animator> animator;

private:
    std::unordered_map<std::string, std::shared_ptr<Animation>> _animations;
    size_t numAnimations = 0;

    friend class AnimationManager;
public:
    AnimationComponent();
    void AddAnimation(std::shared_ptr<Animation> animation_ptr);
    void AddAnimation(Animation animation);
    std::shared_ptr<Animation> GetAnimation(std::string name);

    //DEMO
    std::vector<std::shared_ptr<Animation>> animationVector;
    int idAnimation = 0;
    void ChangeAnimation();
    void CleanLastResources();
};

#endif
