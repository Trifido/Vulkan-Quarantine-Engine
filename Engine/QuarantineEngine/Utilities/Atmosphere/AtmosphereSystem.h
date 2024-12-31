#pragma once
#ifndef ATMOSPHERE_SYSTEM_H
#define ATMOSPHERE_SYSTEM_H

#include <DeviceModule.h>
#include <QESingleton.h>

class AtmosphereSystem : public QESingleton<AtmosphereSystem>
{
private:
    friend class QESingleton<AtmosphereSystem>; // Permitir acceso al constructor

    DeviceModule* device_ptr;

public:
    AtmosphereSystem();
    ~AtmosphereSystem();

    void createAtmosphere();
    void cleanup();
};

#endif // !ATMOSPHERE_SYSTEM_H


