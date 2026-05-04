#pragma once

#ifndef QE_PROJECT_MODULE_API_H
#define QE_PROJECT_MODULE_API_H

#include <memory>

#include "Reflectable.h"

class QEGameComponent;

#if defined(_WIN32)
#if defined(QuarantineEngine_EXPORTS)
#define QE_ENGINE_API __declspec(dllexport)
#else
#define QE_ENGINE_API __declspec(dllimport)
#endif
#else
#define QE_ENGINE_API
#endif

using QEProjectComponentFactory = std::unique_ptr<QEGameComponent>(*)();
using QEProjectComponentMetaFactory = QEMetaType * (*)();
using QEProjectRegisterComponentFn = void(*)(const char* typeName, QEProjectComponentFactory factory, QEProjectComponentMetaFactory metaFactory);

struct QEProjectModuleRegistrar
{
    QEProjectRegisterComponentFn RegisterComponent = nullptr;
};

using QERegisterProjectModuleFn = void(*)(const QEProjectModuleRegistrar* registrar);

#if defined(_WIN32)
#define QE_PROJECT_MODULE_EXPORT extern "C" __declspec(dllexport)
#else
#define QE_PROJECT_MODULE_EXPORT extern "C"
#endif

#define QE_REGISTER_PROJECT_COMPONENT(RegistrarPtr, ComponentType)                              \
    do                                                                                          \
    {                                                                                           \
        if ((RegistrarPtr) != nullptr && (RegistrarPtr)->RegisterComponent != nullptr)          \
        {                                                                                       \
            (RegistrarPtr)->RegisterComponent(                                                  \
                #ComponentType,                                                                 \
                &ComponentType::createInstance,                                                 \
                &ComponentType::staticMeta);                                                    \
        }                                                                                       \
    } while (false)


namespace QE
{
    using ::QEProjectComponentFactory;
    using ::QEProjectComponentMetaFactory;
    using ::QEProjectModuleRegistrar;
    using ::QEProjectRegisterComponentFn;
    using ::QERegisterProjectModuleFn;
} // namespace QE

#endif
