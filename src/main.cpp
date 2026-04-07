#include "Editor/Core/QEEditorApp.h"
#include <QEProjectManager.h>
#include <Logging/QELogMacros.h>

#ifdef _WIN32
#include <windows.h>
#include <filesystem>
#include <iostream>
#endif

int main(int, char**)
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

    QEProjectManager::CreateQEProject("QETest");

    QEScene scene{};
    QEProjectManager::InitializeDefaultQEScene(scene);

    QEEditorApp app;
    app.Run(scene);

    return 0;
}
