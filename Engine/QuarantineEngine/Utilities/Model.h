#pragma once
#ifndef MODEL_H
#define MODEL_H

#include <string>

#include "GeometryModule.h"

class Model : public GeometryModule
{
public:
    std::string path;

public:
    void loadModel(std::string pathfile);
};

#endif // !MODEL_H


