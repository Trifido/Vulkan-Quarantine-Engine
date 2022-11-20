#pragma once

#ifndef ANIMATION_MANAGER_H
#define ANIMATION_MANAGER_H

#include <Animation/AnimationComponent.h>
#include <GameObject.h>

class AnimationManager
{
private:
    std::unordered_map<std::string, std::shared_ptr<AnimationComponent>> _animationComponents;
    size_t numAnimationComponents = 0;

public:
    static AnimationManager* instance;

private:
    std::string CheckName(std::string nameGameObject);

public:
    static AnimationManager* getInstance();
    std::shared_ptr<AnimationComponent> GetAnimationComponent(std::string nameGameObject);
    void AddAnimationComponent(std::string nameGameObject, std::shared_ptr<AnimationComponent> animationComponent);
    void AddAnimationComponent(std::string nameGameObject, AnimationComponent animationComponent);
    void InitializeAnimations();
    void UpdateAnimations(float dt);
};

#endif
