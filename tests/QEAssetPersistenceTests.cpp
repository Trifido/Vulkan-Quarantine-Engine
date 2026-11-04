#include <Data/QEAssetSchema.h>
#include <Data/QEAtomicFile.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

namespace
{
std::string ReadAll(const fs::path& path)
{
    std::ifstream input(path, std::ios::binary);
    return std::string(
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>());
}
}

int main()
{
    const auto suffix = std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count());
    const fs::path testRoot = fs::current_path() /
        ("qe_asset_persistence_" + suffix);
    const fs::path targetPath = testRoot / "scene.qescene";

    std::error_code ec;
    fs::create_directories(testRoot, ec);
    if (ec)
        return 1;

    std::string error;
    if (!QE::Persistence::WriteTextFileAtomically(targetPath, "first", {}, &error))
    {
        std::cerr << "Initial atomic write failed: " << error << '\n';
        return 2;
    }
    if (ReadAll(targetPath) != "first")
        return 3;

    if (!QE::Persistence::WriteTextFileAtomically(targetPath, "second", {}, &error))
    {
        std::cerr << "Replacement atomic write failed: " << error << '\n';
        return 4;
    }
    if (ReadAll(targetPath) != "second")
        return 5;
    if (ReadAll(QE::Persistence::GetBackupPath(targetPath)) != "first")
        return 6;

    fs::remove(QE::Persistence::GetBackupPath(targetPath), ec);
    fs::create_directory(QE::Persistence::GetBackupPath(targetPath), ec);
    if (ec)
        return 7;

    if (QE::Persistence::WriteTextFileAtomically(targetPath, "must-not-replace", {}, &error))
        return 8;
    if (ReadAll(targetPath) != "second")
        return 9;

    for (const auto& entry : fs::directory_iterator(testRoot))
    {
        if (entry.path().filename().string().find(".tmp.") != std::string::npos)
            return 10;
    }

    YAML::Node legacy;
    legacy["Payload"] = true;
    uint32_t sourceVersion = 99;
    if (!QE::AssetSchema::ValidateReadable(
        legacy, "Scene", QE::AssetSchema::SceneVersion, &sourceVersion, &error))
        return 11;
    if (sourceVersion != QE::AssetSchema::LegacyVersion)
        return 12;

    YAML::Node current;
    QE::AssetSchema::Stamp(current, "Scene", QE::AssetSchema::SceneVersion);
    if (!QE::AssetSchema::ValidateReadable(
        current, "Scene", QE::AssetSchema::SceneVersion, &sourceVersion, &error))
        return 13;
    if (sourceVersion != QE::AssetSchema::SceneVersion)
        return 14;

    YAML::Node future;
    QE::AssetSchema::Stamp(future, "Scene", QE::AssetSchema::SceneVersion + 1);
    if (QE::AssetSchema::ValidateReadable(
        future, "Scene", QE::AssetSchema::SceneVersion, &sourceVersion, &error))
        return 15;

    YAML::Node wrongType;
    QE::AssetSchema::Stamp(wrongType, "Material", QE::AssetSchema::MaterialVersion);
    if (QE::AssetSchema::ValidateReadable(
        wrongType, "Scene", QE::AssetSchema::SceneVersion, &sourceVersion, &error))
        return 16;

    YAML::Node incomplete;
    incomplete["SchemaVersion"] = QE::AssetSchema::SceneVersion;
    if (QE::AssetSchema::ValidateReadable(
        incomplete, "Scene", QE::AssetSchema::SceneVersion, &sourceVersion, &error))
        return 17;

    fs::remove_all(testRoot, ec);
    return 0;
}
