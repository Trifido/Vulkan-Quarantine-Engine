namespace QuarantineLauncher.Models;

public sealed class ProjectEntry
{
    public required string Name { get; init; }
    public required string FullPath { get; init; }
    public required string DefaultScenePath { get; init; }
    public required DateTime LastModifiedUtc { get; init; }
    public string? EngineDisplayName { get; init; }
    public string? EngineVersion { get; init; }

    public string LastModifiedDisplay => LastModifiedUtc.ToLocalTime().ToString("g");
    public string EngineDisplay => string.IsNullOrWhiteSpace(EngineDisplayName)
        ? "Engine not recorded"
        : EngineDisplayName;
    public string EngineSummaryDisplay => string.IsNullOrWhiteSpace(EngineVersion) || string.Equals(EngineVersion, "workspace", StringComparison.OrdinalIgnoreCase)
        ? EngineDisplay
        : $"{EngineDisplay} - {EngineVersion}";
}
