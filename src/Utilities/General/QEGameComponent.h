#pragma once

#ifndef QE_GAME_COMPONENT_H
#define QE_GAME_COMPONENT_H

#include <glm/gtx/hash.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <Numbered.h>

class QEGameComponent : Numbered
{
public:
    QEGameComponent() {}
    virtual void QEStart() = 0;
    virtual void QEUpdate() = 0;
    virtual void QERelease() = 0;
};

#endif
