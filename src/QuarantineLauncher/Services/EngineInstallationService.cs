using System.IO;
using System.Text.Json;
using QuarantineLauncher.Models;

namespace QuarantineLauncher.Services;

public sealed class EngineInstallationService
{
    public IReadOnlyList<EngineInstallation> GetAvailableInstallations()
    {
        var results = new List<EngineInstallation>();
        var defaultEngineRoot = WorkspaceLocator.ResolveEngineIntegrationPaths().EngineRoot;

        var configuredEngine = WorkspaceLocator.ResolveEngineIntegrationPaths();
        results.Add(CreateFromConfiguredEngine(configuredEngine, defaultEngineRoot));

        AddInstallationsFromRoot(results, WorkspaceLocator.FindEngineInstallationsRoot(), defaultEngineRoot, managedInstallation: true);
        AddInstallationsFromRoot(results, WorkspaceLocator.FindEnginePackageFeedRoot(), defaultEngineRoot, managedInstallation: false);

        return results
            .OrderByDescending(item => item.IsInstalledPackage)
            .ThenByDescending(item => item.IsManagedInstallation)
            .ThenByDescending(item => item.Version, StringComparer.OrdinalIgnoreCase)
            .ThenBy(item => item.DisplayName, StringComparer.OrdinalIgnoreCase)
            .ToList();
    }

    private static void AddInstallationsFromRoot(
        ICollection<EngineInstallation> results,
        string? root,
        string defaultEngineRoot,
        bool managedInstallation)
    {
        if (string.IsNullOrWhiteSpace(root) || !Directory.Exists(root))
        {
            return;
        }

        foreach (var manifestPath in Directory.EnumerateFiles(root, "manifest.json", SearchOption.AllDirectories))
        {
            var installation = TryCreateFromManifest(manifestPath, managedInstallation);
            if (installation is null)
            {
                continue;
            }

            if (results.Any(existing =>
                    string.Equals(existing.RootPath, installation.RootPath, StringComparison.OrdinalIgnoreCase) ||
                    (string.Equals(existing.Version, installation.Version, StringComparison.OrdinalIgnoreCase) &&
                     string.Equals(existing.Platform, installation.Platform, StringComparison.OrdinalIgnoreCase) &&
                     existing.IsManagedInstallation)))
            {
                continue;
            }

            results.Add(ApplyDefaultFlag(installation, defaultEngineRoot));
        }
    }

    private static EngineInstallation CreateFromConfiguredEngine(EngineIntegrationPaths enginePaths, string defaultEngineRoot)
    {
        var isInstalledPackage = File.Exists(Path.Combine(enginePaths.EngineRoot, "metadata", "manifest.json"));
        var manifestPath = Path.Combine(enginePaths.EngineRoot, "metadata", "manifest.json");
        var manifest = isInstalledPackage ? TryReadManifest(manifestPath) : null;
        var name = isInstalledPackage
            ? $"Quarantine Engine {manifest?.Version ?? Path.GetFileName(enginePaths.EngineRoot)}"
            : "Current Development Workspace";

        return new EngineInstallation
        {
            DisplayName = name,
            ManifestPath = isInstalledPackage ? manifestPath : null,
            Version = isInstalledPackage ? manifest?.Version ?? Path.GetFileName(enginePaths.EngineRoot) : "workspace",
            Platform = manifest?.Platform ?? "win-x64",
            RootPath = enginePaths.EngineRoot,
            IsInstalledPackage = isInstalledPackage,
            IsManagedInstallation = false,
            IsDefault = string.Equals(enginePaths.EngineRoot, defaultEngineRoot, StringComparison.OrdinalIgnoreCase),
            Configurations = isInstalledPackage ? ReadConfigurations(manifestPath) : new[] { "Debug", "Release" },
            EnginePaths = enginePaths
        };
    }

    private static EngineInstallation? TryCreateFromManifest(string manifestPath, bool managedInstallation)
    {
        try
        {
            using var stream = File.OpenRead(manifestPath);
            var manifest = JsonSerializer.Deserialize<EngineManifest>(stream, new JsonSerializerOptions
            {
                PropertyNameCaseInsensitive = true
            });

            if (manifest is null || string.IsNullOrWhiteSpace(manifest.Version))
            {
                return null;
            }

            var rootPath = Directory.GetParent(Path.GetDirectoryName(manifestPath)!)?.FullName;
            if (string.IsNullOrWhiteSpace(rootPath))
            {
                return null;
            }

            var buildDir = Path.Combine(rootPath, "bin");
            return new EngineInstallation
            {
                DisplayName = $"Quarantine Engine {manifest.Version}",
                ManifestPath = manifestPath,
                Version = manifest.Version,
                Platform = string.IsNullOrWhiteSpace(manifest.Platform) ? "win-x64" : manifest.Platform,
                RootPath = rootPath,
                IsInstalledPackage = true,
                IsManagedInstallation = managedInstallation,
                IsDefault = false,
                Configurations = manifest.Configurations?.Count > 0 ? manifest.Configurations : new[] { "Release" },
                EnginePaths = new EngineIntegrationPaths(rootPath, buildDir)
            };
        }
        catch
        {
            return null;
        }
    }

    private static IReadOnlyList<string> ReadConfigurations(string manifestPath)
    {
        try
        {
            var manifest = TryReadManifest(manifestPath);
            return manifest?.Configurations?.Count > 0 ? manifest.Configurations : new[] { "Release" };
        }
        catch
        {
            return new[] { "Release" };
        }
    }

    private static EngineManifest? TryReadManifest(string manifestPath)
    {
        try
        {
            using var stream = File.OpenRead(manifestPath);
            return JsonSerializer.Deserialize<EngineManifest>(stream, new JsonSerializerOptions
            {
                PropertyNameCaseInsensitive = true
            });
        }
        catch
        {
            return null;
        }
    }

    private static EngineInstallation ApplyDefaultFlag(EngineInstallation installation, string defaultEngineRoot)
    {
        if (installation.IsDefault == string.Equals(installation.RootPath, defaultEngineRoot, StringComparison.OrdinalIgnoreCase))
        {
            return installation;
        }

        return new EngineInstallation
        {
            DisplayName = installation.DisplayName,
            ManifestPath = installation.ManifestPath,
            Version = installation.Version,
            Platform = installation.Platform,
            RootPath = installation.RootPath,
            IsInstalledPackage = installation.IsInstalledPackage,
            IsManagedInstallation = installation.IsManagedInstallation,
            IsDefault = string.Equals(installation.RootPath, defaultEngineRoot, StringComparison.OrdinalIgnoreCase),
            Configurations = installation.Configurations,
            EnginePaths = installation.EnginePaths
        };
    }

    private sealed class EngineManifest
    {
        public string? Version { get; set; }
        public string? Platform { get; set; }
        public List<string>? Configurations { get; set; }
    }
}
