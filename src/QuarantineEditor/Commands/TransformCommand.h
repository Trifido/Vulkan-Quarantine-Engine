#pragma once

#include "IEditorCommand.h"

#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

struct TransformState
{
    glm::vec3 Position{ 0.0f };
    glm::quat Rotation{ 1.0f, 0.0f, 0.0f, 0.0f };
    glm::vec3 Scale{ 1.0f };
};

class TransformCommand : public IEditorCommand
{
public:
    TransformCommand(
        const std::string& gameObjectId,
        const TransformState& beforeState,
        const TransformState& afterState);

    void Execute() override;
    void Undo() override;

private:
    void ApplyState(const TransformState& state);

private:
    std::string gameObjectId;
    TransformState beforeState;
    TransformState afterState;
};
