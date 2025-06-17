#pragma once  
#include "QEGameComponent.h"  

class TestComponent : public QEGameComponent  
{
    REFLECTABLE_COMPONENT(TestComponent)
public:
    REFLECT_PROPERTY(std::string, Name)
};
