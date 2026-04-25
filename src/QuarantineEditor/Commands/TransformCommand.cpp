#include "TransformCommand.h"

#include <GameObjectManager.h>
#include <QEGameObject.h>
#include <QETransform.h>

TransformCommand::TransformCommand(
    const std::string& gameObjectId,
    const TransformState& beforeState,
    const TransformState& afterState)
    : gameObjectId(gameObjectId)
    , beforeState(beforeState)
    , afterState(afterState)
{
}

void TransformCommand::Execute()
{
    ApplyState(afterState);
}

void TransformCommand::Undo()
{
    ApplyState(beforeState);
}

void TransformCommand::ApplyState(const TransformState& state)
{
    auto gameObject = GameObjectManager::getInstance()->GetGameObjectById(gameObjectId);
    if (!gameObject)
        return;

    auto transform = gameObject->GetComponent<QETransform>();
    if (!transform)
        return;

    transform->SetLocalPosition(state.Position);
    transform->SetLocalRotation(state.Rotation);
    transform->SetLocalScale(state.Scale);
}
