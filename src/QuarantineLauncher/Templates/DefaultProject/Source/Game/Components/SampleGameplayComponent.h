#pragma once

#include <QEGameComponent.h>

class SampleGameplayComponent : public QEGameComponent
{
public:
    REFLECTABLE_COMPONENT(SampleGameplayComponent)
    REFLECT_PROPERTY(float, RotationSpeed)

public:
    SampleGameplayComponent();

    void QEUpdate() override;
};
