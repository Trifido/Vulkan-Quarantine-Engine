using QuarantineLauncher.Services;

namespace QuarantineLauncher.Models;

public sealed class EngineInstallation
{
    public required string DisplayName { get; init; }
    public required string Version { get; init; }
    public required string Platform { get; init; }
    public required string RootPath { get; init; }
    public required bool IsInstalledPackage { get; init; }
    public required IReadOnlyList<string> Configurations { get; init; }
    public required EngineIntegrationPaths EnginePaths { get; init; }

    public string Summary => IsInstalledPackage
        ? $"{Version} ({Platform})"
        : $"{Version} development workspace";

    public override string ToString()
    {
        return DisplayName;
    }
}
