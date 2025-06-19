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
    REFLECT_PROPERTY(bool, _QEBound)
    REFLECT_PROPERTY(bool, _QEStarted)
    REFLECT_PROPERTY(bool, _QEInitialized)
    REFLECT_PROPERTY(bool, _QEDestroyed)

public:
    QEGameObject* Owner = nullptr;
    inline bool QEBound() const { return _QEBound; };
    inline bool QEStarted() const { return _QEStarted; };
    inline bool QEInitialized() const { return _QEInitialized; };
    inline bool QEDestroyed() const { return _QEDestroyed; };

public:
    QEGameComponent()
    {
        _QEBound = false;
        _QEStarted = false;
        _QEInitialized = false;
        _QEDestroyed = false;
    }
    virtual void QEStart();
    virtual void QEInit();
    virtual void QEUpdate();
    virtual void QEDestroy();

    void BindGameObject(QEGameObject* gameObject);
};

#endif
