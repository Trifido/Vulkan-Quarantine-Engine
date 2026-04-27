#include <QEProjectModuleAPI.h>

#include "Components/SampleGameplayComponent.h"

QE_PROJECT_MODULE_EXPORT void QE_RegisterProjectModule(const QEProjectModuleRegistrar* registrar)
{
    QE_REGISTER_PROJECT_COMPONENT(registrar, SampleGameplayComponent);
}
