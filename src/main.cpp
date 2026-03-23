#include "Editor/Core/QEEditorApp.h"
#include <QEProjectManager.h>

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
        std::cerr << "Error al resolver la ruta del exe\n";
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

    if (!QEProjectManager::CreateQEProject("QEExamplePBR"))
    {
        std::cout << "Project already exists or could not be created.\n";
    }

    QEScene scene{};
    QEProjectManager::InitializeDefaultQEScene(scene);

    QEEditorApp app;
    app.Run(scene);

    system("pause");
    return 0;
}
