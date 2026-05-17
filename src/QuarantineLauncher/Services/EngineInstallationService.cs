using System.IO;
using System.Net.Http;
using System.Text.Json;
using QuarantineLauncher.Models;

namespace QuarantineLauncher.Services;

public sealed class EngineInstallationService
{
    private static readonly HttpClient HttpClient = new();

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
        if (string.IsNullOrWhiteSpace(indexPath))
        {
            return false;
        }

        try
        {
            var (index, indexDirectory, indexUri) = LoadFeedIndex(indexPath);

            if (index?.Packages is null || index.Packages.Count == 0)
            {
                return false;
            }

            var addedAny = false;

            foreach (var package in index.Packages)
            {
                var installation = TryCreateFromFeedPackage(package, indexDirectory, indexUri);
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

    private static (EngineFeedIndex? Index, string IndexDirectory, Uri? IndexUri) LoadFeedIndex(string indexPathOrUrl)
    {
        if (Uri.TryCreate(indexPathOrUrl, UriKind.Absolute, out var indexUri) &&
            (indexUri.Scheme == Uri.UriSchemeHttp || indexUri.Scheme == Uri.UriSchemeHttps))
        {
            using var stream = HttpClient.GetStreamAsync(indexUri).GetAwaiter().GetResult();
            var index = JsonSerializer.Deserialize<EngineFeedIndex>(stream, new JsonSerializerOptions
            {
                PropertyNameCaseInsensitive = true
            });

            return (index, string.Empty, indexUri);
        }

        if (!File.Exists(indexPathOrUrl))
        {
            return (null, string.Empty, null);
        }

        using var localStream = File.OpenRead(indexPathOrUrl);
        var localIndex = JsonSerializer.Deserialize<EngineFeedIndex>(localStream, new JsonSerializerOptions
        {
            PropertyNameCaseInsensitive = true
        });

        return (localIndex, Path.GetDirectoryName(indexPathOrUrl) ?? string.Empty, null);
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
            PackageSource = null,
            Version = isInstalledPackage ? manifest?.Version ?? Path.GetFileName(enginePaths.EngineRoot) : "workspace",
            Channel = manifest?.Channel,
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
                PackageSource = null,
                Version = manifest.Version,
                Channel = manifest.Channel,
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

    private static EngineInstallation? TryCreateFromFeedPackage(EngineFeedPackage package, string indexDirectory, Uri? indexUri)
    {
        var resolvedManifestPath = ResolveFeedReference(package.Manifest, indexDirectory, indexUri);
        var resolvedArchivePath = ResolveFeedReference(package.Archive, indexDirectory, indexUri);

        if (string.IsNullOrWhiteSpace(resolvedManifestPath) &&
            string.IsNullOrWhiteSpace(resolvedArchivePath) &&
            (string.IsNullOrWhiteSpace(package.Version) || string.IsNullOrWhiteSpace(package.Platform)))
        {
            return null;
        }

        EngineInstallation? installation = null;

        if (!string.IsNullOrWhiteSpace(resolvedManifestPath) && File.Exists(resolvedManifestPath))
        {
            installation = TryCreateFromManifest(resolvedManifestPath, managedInstallation: false);
        }

        if (installation is null && string.IsNullOrWhiteSpace(package.Version))
        {
            return null;
        }

        installation ??= CreateFromFeedMetadata(package, resolvedManifestPath, resolvedArchivePath);
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
            PackageSource = resolvedArchivePath ?? installation.PackageSource,
            Version = installation.Version,
            Channel = installation.Channel ?? package.Channel,
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

    private static EngineInstallation? CreateFromFeedMetadata(
        EngineFeedPackage package,
        string? resolvedManifestPath,
        string? resolvedArchivePath)
    {
        if (string.IsNullOrWhiteSpace(package.Version))
        {
            return null;
        }

        var displayName = string.IsNullOrWhiteSpace(package.DisplayName)
            ? $"Quarantine Engine {package.Version}"
            : package.DisplayName;
        var platform = string.IsNullOrWhiteSpace(package.Platform) ? "win-x64" : package.Platform;
        var sourcePath = resolvedArchivePath ?? resolvedManifestPath ?? displayName;

        return new EngineInstallation
        {
            DisplayName = displayName,
            ManifestPath = File.Exists(resolvedManifestPath ?? string.Empty) ? resolvedManifestPath : null,
            PackageSource = resolvedArchivePath,
            Version = package.Version,
            Channel = package.Channel,
            Platform = platform,
            RootPath = sourcePath,
            IsInstalledPackage = true,
            IsManagedInstallation = false,
            IsDefault = false,
            IsInstalledInLauncher = false,
            Configurations = new[] { "Release" },
            EnginePaths = new EngineIntegrationPaths(sourcePath, Path.Combine(sourcePath, "bin"))
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
            PackageSource = installation.PackageSource,
            Version = installation.Version,
            Channel = installation.Channel,
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
        public string? Channel { get; set; }
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
        public string? Version { get; set; }
        public string? Channel { get; set; }
        public string? Platform { get; set; }
        public string? Manifest { get; set; }
        public string? Archive { get; set; }
    }

    private static string? ResolveFeedReference(string? reference, string indexDirectory, Uri? indexUri)
    {
        if (string.IsNullOrWhiteSpace(reference))
        {
            return null;
        }

        if (Uri.TryCreate(reference, UriKind.Absolute, out var absoluteUri))
        {
            return absoluteUri.IsFile ? absoluteUri.LocalPath : absoluteUri.ToString();
        }

        if (indexUri is not null)
        {
            return new Uri(indexUri, reference).ToString();
        }

        return Path.GetFullPath(Path.Combine(indexDirectory, reference));
    }
}
