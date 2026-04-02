#include "QETextureImporter.h"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>



static bool RunProcessAndWait(
    const std::string& exePath,
    const std::string& arguments,
    DWORD& exitCode)
{
    std::string cmdLine = "\"" + exePath + "\" " + arguments;

    STARTUPINFOA si{};
    si.cb = sizeof(si);

    PROCESS_INFORMATION pi{};

    // CreateProcess puede modificar el buffer, así que necesita char*
    std::vector<char> buffer(cmdLine.begin(), cmdLine.end());
    buffer.push_back('\0');

    BOOL ok = CreateProcessA(
        nullptr,                // application name
        buffer.data(),          // command line
        nullptr,                // process attrs
        nullptr,                // thread attrs
        FALSE,                  // inherit handles
        0,                      // creation flags
        nullptr,                // env
        nullptr,                // current dir
        &si,
        &pi
    );

    if (!ok)
    {
        exitCode = GetLastError();
        return false;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD processExitCode = 0;
    GetExitCodeProcess(pi.hProcess, &processExitCode);
    exitCode = processExitCode;

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    return true;
}


std::string QETextureImporter::Quote(const std::string& s)
{
    return "\"" + s + "\"";
}

std::string QETextureImporter::BuildImportedPath(const std::string& sourcePath)
{
    namespace fs = std::filesystem;

    fs::path src = fs::absolute(sourcePath).lexically_normal();
    fs::path srcParent = src.parent_path();

    fs::path importedDir;
    if (srcParent.filename() == "Textures")
        importedDir = srcParent.parent_path() / "ImportedTextures";
    else
        importedDir = srcParent / "ImportedTextures";

    fs::create_directories(importedDir);

    fs::path outFile = importedDir / src.stem();
    outFile += ".ktx2";

    return fs::absolute(outFile).lexically_normal().string();
}

QETextureImportResult QETextureImporter::ImportToKtx2(
    const std::string& sourcePath,
    const std::string& outputPath,
    const QETextureImportSettings& settings)
{
    namespace fs = std::filesystem;

    QETextureImportResult result;
    result.sourcePath = fs::absolute(sourcePath).lexically_normal().string();
    result.importedPath = fs::absolute(outputPath).lexically_normal().string();

    if (!fs::exists(result.sourcePath))
    {
        result.error = "Source texture does not exist: " + result.sourcePath;
        return result;
    }

    if (fs::exists(result.importedPath) && !settings.overwrite)
    {
        result.success = true;
        return result;
    }

    fs::create_directories(fs::path(result.importedPath).parent_path());

    const std::string toktxExe =
        fs::absolute("../../extern/KTX-Software/bin/toktx.exe")
        .lexically_normal()
        .string();

    const int testRc = std::system((Quote(toktxExe) + " --version").c_str());
    std::cout << "[TextureImporter] toktx test rc: " << testRc << std::endl;

    if (!fs::exists(toktxExe))
    {
        result.error = "toktx.exe not found: " + toktxExe;
        return result;
    }

    std::ostringstream args;
    args << "--t2 ";

    if (settings.generateMipmaps)
        args << "--genmipmap ";

    args << Quote(result.importedPath) << " ";
    args << Quote(result.sourcePath);

    std::cout << "[TextureImporter] Toktx:  " << toktxExe << std::endl;
    std::cout << "[TextureImporter] Source: " << result.sourcePath << std::endl;
    std::cout << "[TextureImporter] Output: " << result.importedPath << std::endl;
    std::cout << "[TextureImporter] Args:   " << args.str() << std::endl;

    const bool sourceExists = fs::exists(result.sourcePath);
    const bool outDirExists = fs::exists(fs::path(result.importedPath).parent_path());

    std::cout << "[TextureImporter] Source exists: " << sourceExists << std::endl;
    std::cout << "[TextureImporter] Output dir exists: " << outDirExists << std::endl;

    DWORD rc = 0;
    bool launchOk = RunProcessAndWait(toktxExe, args.str(), rc);

    if (!launchOk)
    {
        result.error = "CreateProcessA failed. GetLastError = " + std::to_string(rc);
        return result;
    }

    if (rc != 0)
    {
        result.error = "toktx failed with exit code: " + std::to_string(rc);
        return result;
    }

    if (!fs::exists(result.importedPath))
    {
        result.error = "toktx finished but output file was not created: " + result.importedPath;
        return result;
    }

    result.success = true;
    return result;
}
