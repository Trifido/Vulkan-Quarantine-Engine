#include "__QE_PROJECT_NAME__GameApp.h"

#include <QEProjectManager.h>
#include <QEProjectModuleLoader.h>
#include <Logging/QELogMacros.h>

#include <filesystem>
#include <iostream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

namespace
{
    std::filesystem::path ResolveExecutableDirectory()
    {
#ifdef _WIN32
        wchar_t pathBuf[MAX_PATH];
        DWORD len = GetModuleFileNameW(nullptr, pathBuf, MAX_PATH);
        if (len == 0 || len == MAX_PATH)
        {
            return {};
        }

        return std::filesystem::path(pathBuf).parent_path();
#else
        return {};
#endif
    }
}

int main(int argc, char** argv)
{
    const std::filesystem::path executableDirectory = ResolveExecutableDirectory();
    if (executableDirectory.empty())
    {
        QE_LOG_ERROR_CAT("Execution", "Could not resolve the runtime executable directory.");
        return -1;
    }

    const std::filesystem::path projectPath = std::filesystem::weakly_canonical(executableDirectory / ".." / "..");

#ifdef _DEBUG
    const std::filesystem::path engineWorkingDirectory = std::filesystem::weakly_canonical(projectPath / ".." / ".." / "build" / "Debug");
#else
    const std::filesystem::path engineWorkingDirectory = std::filesystem::weakly_canonical(projectPath / ".." / ".." / "build" / "Release");
#endif

    std::error_code ec;
    std::filesystem::current_path(engineWorkingDirectory, ec);
    if (ec)
    {
        std::cerr << "Could not switch the working directory to '" << engineWorkingDirectory.string() << "': "
                  << ec.message() << "\n";
        return -1;
    }

    if (!QE::QEProjectManager::SetCurrentProjectPath(projectPath))
    {
        QE_LOG_ERROR_CAT_F("Execution", "Could not open project '{}'", projectPath.string());
        return -1;
    }

    std::string projectModuleLoadError;
    if (!QE::QEProjectModuleLoader::LoadForProject(projectPath, &projectModuleLoadError))
    {
        QE_LOG_ERROR_CAT_F("Execution", "{}", projectModuleLoadError);
        return -1;
    }

    QE::QEScene scene{};
    if (!QE::QEProjectManager::InitializeDefaultQEScene(scene))
    {
        QE_LOG_ERROR_CAT_F("Execution", "Could not initialize default scene for project '{}'", projectPath.string());
        return -1;
    }

    __QE_PROJECT_NAME__GameApp app;
    app.Run(scene);
    QE::QEProjectModuleLoader::Unload();
    return 0;
}
