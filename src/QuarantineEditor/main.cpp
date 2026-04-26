#include "QuarantineEditor/Core/QEEditorApp.h"
#include <QEProjectManager.h>
#include <Logging/QELogMacros.h>

#ifdef _WIN32
#include <windows.h>
#include <filesystem>
#include <iostream>
#endif

int main(int argc, char** argv)
{
#ifdef _WIN32
    wchar_t pathBuf[MAX_PATH];
    DWORD len = GetModuleFileNameW(nullptr, pathBuf, MAX_PATH);
    if (len == 0 || len == MAX_PATH)
    {
        QE_LOG_ERROR_CAT("Execution", "Error resolving the path to the exe file");
        return -1;
    }

    std::filesystem::path exePath(pathBuf);
    exePath = exePath.parent_path();

    std::error_code ec;
    std::filesystem::current_path(exePath, ec);
    if (ec)
    {
        std::wcerr << L"No se pudo cambiar el Working Directory a '" << exePath.wstring()
            << L"': " << ec.message().c_str() << L"\n";
        return -1;
    }

    std::wcout << L"Working Directory fijado a: " << exePath.wstring() << L"\n\n";
#endif

    if (argc < 2 || argv[1] == nullptr)
    {
        QE_LOG_ERROR_CAT("Execution", "No project path provided. Launch QuarantineEditor from QuarantineLauncher.");
        return -1;
    }

    const std::filesystem::path projectPath = argv[1];
    if (!QE::QEProjectManager::SetCurrentProjectPath(projectPath))
    {
        QE_LOG_ERROR_CAT_F("Execution", "Could not open project '{}'", projectPath.string());
        return -1;
    }

    QE::QEScene scene{};
    if (!QE::QEProjectManager::InitializeDefaultQEScene(scene))
    {
        QE_LOG_ERROR_CAT_F("Execution", "Could not initialize default scene for project '{}'", projectPath.string());
        return -1;
    }

    QEEditorApp app;
    app.Run(scene);

    return 0;
}
