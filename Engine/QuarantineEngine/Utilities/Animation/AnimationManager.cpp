#include "AnimationManager.h"

AnimationManager* AnimationManager::instance = nullptr;

std::string AnimationManager::CheckName(std::string nameGameObject)
{
    std::unordered_map<std::string, std::shared_ptr<AnimationComponent>>::const_iterator got;
    std::string newName = nameGameObject;
    unsigned int id = 0;

    do
    {
        got = _animationComponents.find(newName);

        if (got != _animationComponents.end())
        {
            id++;
            newName = nameGameObject + "_" + std::to_string(id);
        }
    } while (got != _animationComponents.end());

    return newName;
}

AnimationManager* AnimationManager::getInstance()
{
    if (instance == NULL)
        instance = new AnimationManager();
    else
        std::cout << "Getting existing instance of Animation Manager" << std::endl;

    return instance;
}

std::shared_ptr<AnimationComponent> AnimationManager::GetAnimationComponent(std::string nameGameObject)
{
    auto it = _animationComponents.find(nameGameObject);

    if (it != _animationComponents.end())
        return it->second;

    return nullptr;
}

void AnimationManager::AddAnimationComponent(std::string nameGameObject, std::shared_ptr<AnimationComponent> animationComponent)
{
    std::string name = CheckName(nameGameObject);
    _animationComponents[name] = animationComponent;
}

void AnimationManager::AddAnimationComponent(std::string nameGameObject, AnimationComponent animationComponent)
{
    std::string name = CheckName(nameGameObject);
    std::shared_ptr<AnimationComponent> mat_ptr = std::make_shared<AnimationComponent>(animationComponent);
    _animationComponents[name] = mat_ptr;
}

void AnimationManager::InitializeAnimations()
{
    for (auto& it : _animationComponents)
    {
        auto animation = it.second->_animations.begin()->second;
        it.second->animator->PlayAnimation(animation);
    }
}

void AnimationManager::UpdateAnimations(float dt)
{
    for (auto& it : _animationComponents)
    {
        it.second->animator->UpdateAnimation(dt);
    }
}
