#include "QEProjectModuleLoader.h"

#include "QEProjectModuleAPI.h"

#include <Logging/QELogMacros.h>
#include <QEGameComponent.h>

#include <algorithm>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace
{
#ifdef _WIN32
    HMODULE g_loadedProjectModule = nullptr;
#endif
    std::vector<std::string> g_registeredProjectComponentNames;

    void SetError(std::string* errorMessage, const std::string& message)
    {
        if (errorMessage != nullptr)
        {
            *errorMessage = message;
        }
    }

    void RegisterProjectComponentImpl(const char* typeName, QEProjectComponentFactory factory, QEProjectComponentMetaFactory metaFactory)
    {
        if (typeName == nullptr || factory == nullptr || metaFactory == nullptr)
        {
            return;
        }

        QEMetaType* meta = metaFactory();
        if (meta == nullptr)
        {
            return;
        }

        registerMetaType(typeName, meta);
        getFactoryRegistry()[typeName] = factory;

        const std::string registeredName = typeName;
        if (std::find(g_registeredProjectComponentNames.begin(), g_registeredProjectComponentNames.end(), registeredName)
            == g_registeredProjectComponentNames.end())
        {
            g_registeredProjectComponentNames.push_back(registeredName);
        }
    }

    std::filesystem::path ResolveProjectModulePath(const std::filesystem::path& projectPath)
    {
        const std::string configurationFolder =
#ifdef _DEBUG
            "Debug";
#else
            "Release";
#endif

        return projectPath / "Binaries" / configurationFolder / (projectPath.filename().string() + "Game.dll");
    }
}

bool QE_LoadProjectModuleForProject(const std::filesystem::path& projectPath, std::string* errorMessage)
{
#ifdef _WIN32
    QE_UnloadProjectModule();

    const std::filesystem::path modulePath = ResolveProjectModulePath(projectPath);
    if (!std::filesystem::exists(modulePath))
    {
        SetError(errorMessage, "Could not find the project game module at '" + modulePath.string() + "'.");
        return false;
    }

    g_loadedProjectModule = LoadLibraryW(modulePath.c_str());
    if (g_loadedProjectModule == nullptr)
    {
        SetError(errorMessage, "Could not load the project game module '" + modulePath.string() + "'.");
        return false;
    }

    auto registerModule = reinterpret_cast<QERegisterProjectModuleFn>(
        GetProcAddress(g_loadedProjectModule, "QE_RegisterProjectModule"));

    if (registerModule == nullptr)
    {
        SetError(errorMessage, "The project game module does not export QE_RegisterProjectModule.");
        FreeLibrary(g_loadedProjectModule);
        g_loadedProjectModule = nullptr;
        return false;
    }

    QEProjectModuleRegistrar registrar{};
    registrar.RegisterComponent = &RegisterProjectComponentImpl;
    registerModule(&registrar);

    QE_LOG_INFO_CAT_F("ProjectModule", "Loaded project game module '{}'", modulePath.string());
    return true;
#else
    (void)projectPath;
    SetError(errorMessage, "Project module loading is currently only implemented on Windows.");
    return false;
#endif
}

void QE_UnloadProjectModule()
{
    for (const std::string& componentName : g_registeredProjectComponentNames)
    {
        getFactoryRegistry().erase(componentName);
        getMetaRegistry().erase(componentName);
    }

    g_registeredProjectComponentNames.clear();

#ifdef _WIN32
    if (g_loadedProjectModule != nullptr)
    {
        FreeLibrary(g_loadedProjectModule);
        g_loadedProjectModule = nullptr;
    }
#endif
}

bool QE_IsProjectModuleLoaded()
{
#ifdef _WIN32
    return g_loadedProjectModule != nullptr;
#else
    return false;
#endif
}
