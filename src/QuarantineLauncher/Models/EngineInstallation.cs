using QuarantineLauncher.Services;

namespace QuarantineLauncher.Models;

public sealed class EngineInstallation
{
    public required string DisplayName { get; init; }
    public string? ManifestPath { get; init; }
    public required string Version { get; init; }
    public required string Platform { get; init; }
    public required string RootPath { get; init; }
    public required bool IsInstalledPackage { get; init; }
    public required bool IsManagedInstallation { get; init; }
    public required bool IsDefault { get; init; }
    public bool IsInstalledInLauncher { get; init; }
    public required IReadOnlyList<string> Configurations { get; init; }
    public required EngineIntegrationPaths EnginePaths { get; init; }

    public string KindDisplay => IsInstalledPackage
        ? (IsManagedInstallation ? "Installed package" : "Package feed")
        : "Development workspace";

    public string AvailabilityDisplay => IsInstalledInLauncher ? "Installed" : "Available";

    public string Summary => IsInstalledPackage
        ? $"{Version} ({Platform})"
        : $"{Version} development workspace";

    public string StatusDisplay => IsDefault
        ? $"{KindDisplay} - Default"
        : KindDisplay;

    public override string ToString()
    {
        return IsDefault ? $"{DisplayName} [Default]" : DisplayName;
    }
}
