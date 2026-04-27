#pragma once

#ifndef QE_PROJECT_MODULE_LOADER_H
#define QE_PROJECT_MODULE_LOADER_H

#include <filesystem>
#include <string>

#include "QEProjectModuleAPI.h"

QE_ENGINE_API bool QE_LoadProjectModuleForProject(const std::filesystem::path& projectPath, std::string* errorMessage = nullptr);
QE_ENGINE_API void QE_UnloadProjectModule();
QE_ENGINE_API bool QE_IsProjectModuleLoaded();

class QEProjectModuleLoader
{
public:
    static bool LoadForProject(const std::filesystem::path& projectPath, std::string* errorMessage = nullptr)
    {
        return QE_LoadProjectModuleForProject(projectPath, errorMessage);
    }

    static void Unload()
    {
        QE_UnloadProjectModule();
    }

    static bool IsLoaded()
    {
        return QE_IsProjectModuleLoaded();
    }
};


namespace QE
{
    using ::QEProjectModuleLoader;
} // namespace QE

#endif
