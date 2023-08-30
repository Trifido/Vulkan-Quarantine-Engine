#pragma once

#ifndef KEYBOARD_CONTROLLER_H
#define KEYBOARD_CONTROLLER_H

#include <stdio.h>
#include <list>

class IObserver {
public:
    virtual ~IObserver() {};
    virtual void Update(const __int8& message_from_subject) = 0;
};

class IKeyboardController
{
public:
    virtual ~IKeyboardController() {};
    virtual void Attach(IObserver* observer) = 0;
    virtual void Detach(IObserver* observer) = 0;
    virtual void Notify(__int8 keyNum) = 0;
};

class KeyboardController : IKeyboardController
{
private:
    static KeyboardController* instance;
    bool isEditorProfile = true;
    std::list<IObserver*> list_observer_;

public:
    static KeyboardController* getInstance();
    static void ResetInstance();
    void ReadKeyboardEvents();
    void cleanup();
    void CleanLastResources();

    virtual ~KeyboardController();
    void Attach(IObserver* observer) override;
    void Detach(IObserver* observer) override;
    void Notify(__int8 keyNum) override;
private:
    void ReadPolygonModeKeys();
};

#endif
