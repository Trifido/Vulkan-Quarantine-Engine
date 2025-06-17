#pragma once

#ifndef QE_GAME_COMPONENT_H
#define QE_GAME_COMPONENT_H

#include <glm/gtx/hash.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <Numbered.h>
#include <Reflectable.h>

class QEGameObject;

class QEGameComponent : public SerializableComponent, public Numbered
{
    REFLECTABLE_COMPONENT(QEGameComponent)
protected:
    bool _QEBound = false;
    bool _QEStarted = false;
    bool _QEInitialized = false;
    bool _QEDestroyed = false;

public:
    QEGameObject* Owner = nullptr; // Pointer to the owner game object
    inline bool QEBound() const { return _QEBound; };
    inline bool QEStarted() const { return _QEStarted; };
    inline bool QEInitialized() const { return _QEInitialized; };
    inline bool QEDestroyed() const { return _QEDestroyed; };

public:
    QEGameComponent() {}
    virtual void QEStart();
    virtual void QEInit();
    virtual void QEUpdate();
    virtual void QEDestroy();

    void BindGameObject(QEGameObject* gameObject);
};

#endif
