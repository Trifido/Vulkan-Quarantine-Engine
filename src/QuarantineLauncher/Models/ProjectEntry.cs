namespace QuarantineLauncher.Models;

public sealed class ProjectEntry
{
    public required string Name { get; init; }
    public required string FullPath { get; init; }
    public required string DefaultScenePath { get; init; }
    public required DateTime LastModifiedUtc { get; init; }

    public string LastModifiedDisplay => LastModifiedUtc.ToLocalTime().ToString("g");
}
