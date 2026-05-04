#include "QEGameComponent.h"

void QEGameComponent::BindGameObject(QEGameObject* gameObject)
{
    if (gameObject == NULL)
        return;

    Owner = gameObject;
    _QEBound = true;
}

void QEGameComponent::QEStart()
{
    if (!_QEStarted)
    {
        _QEStarted = true;
    }
}

void QEGameComponent::QEInit()
{
    if (_QEStarted && !_QEInitialized)
    {
        _QEInitialized = true;
    }
}

void QEGameComponent::QEUpdate()
{
}

void QEGameComponent::QEDestroy()
{
    if (!_QEDestroyed)
    {
        _QEDestroyed = true;
        _QEStarted = _QEInitialized = _QEBound = false;
    }
}
