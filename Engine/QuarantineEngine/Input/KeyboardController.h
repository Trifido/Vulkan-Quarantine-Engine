#pragma once

#ifndef KEYBOARD_CONTROLLER_H
#define KEYBOARD_CONTROLLER_H

#include <stdio.h>

[event_source(native)]
class KeyboardController
{
public:
    __event void PolygonModeEvent(__int8 polygonType);
    static KeyboardController* instance;
private:
    bool isEditorProfile = true;
public:
    static KeyboardController* getInstance();
    void ReadKeyboardEvents();
    void cleanup();
private:
    void ReadPolygonModeKeys();
};

#endif
