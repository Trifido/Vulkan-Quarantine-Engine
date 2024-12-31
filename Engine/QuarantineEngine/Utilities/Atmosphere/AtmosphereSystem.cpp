#include "AtmosphereSystem.h"

AtmosphereSystem::AtmosphereSystem()
{
    this->device_ptr = DeviceModule::getInstance();
}

AtmosphereSystem::~AtmosphereSystem()
{
    this->device_ptr = nullptr;
}

void AtmosphereSystem::createAtmosphere()
{
}

void AtmosphereSystem::cleanup()
{
}
