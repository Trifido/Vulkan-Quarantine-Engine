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
        var feedIndexLoaded = AddInstallationsFromIndex(results, WorkspaceLocator.FindEngineFeedIndexPath(), defaultEngineRoot);
        if (!feedIndexLoaded)
        {
            AddInstallationsFromRoot(results, WorkspaceLocator.FindEnginePackageFeedRoot(), defaultEngineRoot, managedInstallation: false);
        }

        return results
            .OrderByDescending(item => item.IsInstalledPackage)
            .ThenByDescending(item => item.IsManagedInstallation)
            .ThenByDescending(item => item.Version, StringComparer.OrdinalIgnoreCase)
            .ThenBy(item => item.DisplayName, StringComparer.OrdinalIgnoreCase)
            .ToList();
    }

    private static bool AddInstallationsFromIndex(
        ICollection<EngineInstallation> results,
        string? indexPath,
        string defaultEngineRoot)
    {
        if (string.IsNullOrWhiteSpace(indexPath) || !File.Exists(indexPath))
        {
            return false;
        }

        try
        {
            using var stream = File.OpenRead(indexPath);
            var index = JsonSerializer.Deserialize<EngineFeedIndex>(stream, new JsonSerializerOptions
            {
                PropertyNameCaseInsensitive = true
            });

            if (index?.Packages is null || index.Packages.Count == 0)
            {
                return false;
            }

            var indexDirectory = Path.GetDirectoryName(indexPath) ?? string.Empty;
            var addedAny = false;

            foreach (var package in index.Packages)
            {
                var installation = TryCreateFromFeedPackage(package, indexDirectory);
                if (installation is null || HasEquivalentInstallation(results, installation))
                {
                    continue;
                }

                results.Add(ApplyDefaultFlag(installation, defaultEngineRoot));
                addedAny = true;
            }

            return addedAny;
        }
        catch
        {
            return false;
        }
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
                    HasEquivalentInstallation(existing, installation)))
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
            IsInstalledInLauncher = false,
            Configurations = isInstalledPackage ? ReadConfigurations(manifestPath) : new[] { "Debug", "Release" },
            EnginePaths = enginePaths
        };
    }

    private static EngineInstallation? TryCreateFromManifest(string manifestPath, bool managedInstallation)
    {
        try
        {
            var manifest = TryReadManifest(manifestPath);

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
                IsInstalledInLauncher = managedInstallation,
                Configurations = manifest.Configurations?.Count > 0 ? manifest.Configurations : new[] { "Release" },
                EnginePaths = new EngineIntegrationPaths(rootPath, buildDir)
            };
        }
        catch
        {
            return null;
        }
    }

    private static EngineInstallation? TryCreateFromFeedPackage(EngineFeedPackage package, string indexDirectory)
    {
        if (string.IsNullOrWhiteSpace(package.Manifest))
        {
            return null;
        }

        var manifestPath = Path.GetFullPath(Path.Combine(indexDirectory, package.Manifest));
        if (!File.Exists(manifestPath))
        {
            return null;
        }

        var installation = TryCreateFromManifest(manifestPath, managedInstallation: false);
        if (installation is null)
        {
            return null;
        }

        if (string.IsNullOrWhiteSpace(package.DisplayName))
        {
            return installation;
        }

        return new EngineInstallation
        {
            DisplayName = package.DisplayName,
            ManifestPath = installation.ManifestPath,
            Version = installation.Version,
            Platform = installation.Platform,
            RootPath = installation.RootPath,
            IsInstalledPackage = installation.IsInstalledPackage,
            IsManagedInstallation = installation.IsManagedInstallation,
            IsDefault = installation.IsDefault,
            IsInstalledInLauncher = installation.IsInstalledInLauncher,
            Configurations = installation.Configurations,
            EnginePaths = installation.EnginePaths
        };
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

    private static bool HasEquivalentInstallation(
        IEnumerable<EngineInstallation> results,
        EngineInstallation installation)
    {
        return results.Any(existing => HasEquivalentInstallation(existing, installation));
    }

    private static bool HasEquivalentInstallation(
        EngineInstallation existing,
        EngineInstallation installation)
    {
        return string.Equals(existing.RootPath, installation.RootPath, StringComparison.OrdinalIgnoreCase) ||
               (string.Equals(existing.Version, installation.Version, StringComparison.OrdinalIgnoreCase) &&
                string.Equals(existing.Platform, installation.Platform, StringComparison.OrdinalIgnoreCase) &&
                existing.IsManagedInstallation == installation.IsManagedInstallation);
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
            IsInstalledInLauncher = installation.IsInstalledInLauncher,
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

    private sealed class EngineFeedIndex
    {
        public List<EngineFeedPackage>? Packages { get; set; }
    }

    private sealed class EngineFeedPackage
    {
        public string? DisplayName { get; set; }
        public string? Manifest { get; set; }
    }
}
