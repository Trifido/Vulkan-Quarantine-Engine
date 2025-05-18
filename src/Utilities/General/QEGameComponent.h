#pragma once

#ifndef QE_GAME_COMPONENT_H
#define QE_GAME_COMPONENT_H

#include <glm/gtx/hash.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <Numbered.h>

class QEGameObject;

class QEGameComponent : Numbered
{
public:
    QEGameObject* Owner = nullptr; // Pointer to the owner game object
public:
    QEGameComponent() {}
    virtual void QEStart() = 0;
    virtual void QEUpdate() = 0;
    virtual void QERelease() = 0;

    void BindGameObject(QEGameObject* gameObject);
};

#endif
