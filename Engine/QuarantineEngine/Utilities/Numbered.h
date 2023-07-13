#pragma once

#ifndef NUMBERED_H
#define NUMBERED_H

#include <string>
#include <algorithm>

class Numbered
{
public:
    std::string id;

protected:
    Numbered();
    virtual void CreateGameObjectID(size_t length);
};

#endif // !NUMBERED_H


