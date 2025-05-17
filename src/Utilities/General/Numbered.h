#pragma once

#ifndef NUMBERED_H
#define NUMBERED_H

#include <string>
#include <algorithm>

class Numbered
{
public:
    std::string id;
    static const int ID_LENGTH = 12;
protected:
    Numbered();
    Numbered(const std::string& id) : id(id) {}
    virtual void CreateGameObjectID(size_t length);
};

#endif // !NUMBERED_H


