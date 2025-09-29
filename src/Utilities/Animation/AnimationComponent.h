#pragma once

#ifndef ANIMATION_COMPONENT_H
#define ANIMATION_COMPONENT_H
#include "QEGameComponent.h"
#include <Animation/Animation.h>
#include <Animator.h>

struct AnimationState
{
    string Id;
    bool Loop;
    string AnimationClip;
    string FromState;
    string ToState;
};

class AnimationComponent : public QEGameComponent
{
public:
    std::shared_ptr<Animator> animator;

private:
    std::unordered_map<std::string, AnimationState> _states;
    std::unordered_map<std::string, std::shared_ptr<Animation>> _animations;
    size_t numAnimations = 0;
    AnimationState currentState;

public:
    AnimationComponent();
    void AddAnimation(std::shared_ptr<Animation> animation_ptr);
    void AddAnimation(Animation animation);
    std::shared_ptr<Animation> GetAnimation(std::string name);

    void AddAnimationState(AnimationState state, bool isEntryState = false);

    //DEMO
    std::vector<std::shared_ptr<Animation>> animationVector;
    int idAnimation = 0;
    void ChangeAnimation();
    void CleanLastResources();

    void QEStart() override;
    void QEInit() override;
    void QEUpdate() override;
    void QEDestroy() override;
};

#endif
