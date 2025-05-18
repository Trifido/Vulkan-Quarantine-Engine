#include "QEGameComponent.h"

void QEGameComponent::BindGameObject(QEGameObject* gameObject)
{
    this->Owner = gameObject;
}
