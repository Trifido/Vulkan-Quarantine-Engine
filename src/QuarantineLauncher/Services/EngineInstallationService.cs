using System.IO;
using System.Text.Json;
using QuarantineLauncher.Models;

namespace QuarantineLauncher.Services;

public sealed class EngineInstallationService
{
    public IReadOnlyList<EngineInstallation> GetAvailableInstallations()
    {
        var results = new List<EngineInstallation>();

        var configuredEngine = WorkspaceLocator.ResolveEngineIntegrationPaths();
        results.Add(CreateFromConfiguredEngine(configuredEngine));

        var installationsRoot = WorkspaceLocator.FindEngineInstallationsRoot();
        if (!string.IsNullOrWhiteSpace(installationsRoot) && Directory.Exists(installationsRoot))
        {
            foreach (var manifestPath in Directory.EnumerateFiles(
                installationsRoot,
                "manifest.json",
                SearchOption.AllDirectories))
            {
                var installation = TryCreateFromManifest(manifestPath);
                if (installation is null)
                {
                    continue;
                }

                if (results.Any(existing =>
                        string.Equals(existing.RootPath, installation.RootPath, StringComparison.OrdinalIgnoreCase)))
                {
                    continue;
                }

                results.Add(installation);
            }
        }

        return results
            .OrderByDescending(item => item.IsInstalledPackage)
            .ThenByDescending(item => item.Version, StringComparer.OrdinalIgnoreCase)
            .ThenBy(item => item.DisplayName, StringComparer.OrdinalIgnoreCase)
            .ToList();
    }

    private static EngineInstallation CreateFromConfiguredEngine(EngineIntegrationPaths enginePaths)
    {
        var isInstalledPackage = File.Exists(Path.Combine(enginePaths.EngineRoot, "metadata", "manifest.json"));
        var name = isInstalledPackage
            ? $"Installed Engine ({Path.GetFileName(enginePaths.EngineRoot)})"
            : "Current Development Workspace";

        return new EngineInstallation
        {
            DisplayName = name,
            Version = isInstalledPackage ? Path.GetFileName(enginePaths.EngineRoot) : "workspace",
            Platform = "win-x64",
            RootPath = enginePaths.EngineRoot,
            IsInstalledPackage = isInstalledPackage,
            Configurations = isInstalledPackage ? ReadConfigurations(Path.Combine(enginePaths.EngineRoot, "metadata", "manifest.json")) : new[] { "Debug", "Release" },
            EnginePaths = enginePaths
        };
    }

    private static EngineInstallation? TryCreateFromManifest(string manifestPath)
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
                Version = manifest.Version,
                Platform = string.IsNullOrWhiteSpace(manifest.Platform) ? "win-x64" : manifest.Platform,
                RootPath = rootPath,
                IsInstalledPackage = true,
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
            using var stream = File.OpenRead(manifestPath);
            var manifest = JsonSerializer.Deserialize<EngineManifest>(stream, new JsonSerializerOptions
            {
                PropertyNameCaseInsensitive = true
            });

            return manifest?.Configurations?.Count > 0 ? manifest.Configurations : new[] { "Release" };
        }
        catch
        {
            return new[] { "Release" };
        }
    }

    private sealed class EngineManifest
    {
        public string? Version { get; set; }
        public string? Platform { get; set; }
        public List<string>? Configurations { get; set; }
    }
}
