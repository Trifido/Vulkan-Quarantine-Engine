using System.IO;
using System.Text.Json;

namespace QuarantineLauncher.Services;

public sealed class EnginePackageImportService
{
    public string InstallFromPackageManifest(string manifestPath, bool overwriteExisting = false)
    {
        if (!File.Exists(manifestPath))
        {
            throw new FileNotFoundException("The selected manifest file does not exist.", manifestPath);
        }

        var packageRoot = Directory.GetParent(Path.GetDirectoryName(manifestPath)!)?.FullName;
        if (string.IsNullOrWhiteSpace(packageRoot))
        {
            throw new InvalidOperationException("Could not resolve the package root from the selected manifest.");
        }

        var manifest = LoadManifest(manifestPath);
        if (string.IsNullOrWhiteSpace(manifest.Version) || string.IsNullOrWhiteSpace(manifest.Platform))
        {
            throw new InvalidOperationException("The selected manifest is missing version or platform information.");
        }

        var installationsRoot = WorkspaceLocator.FindEngineInstallationsRoot()
            ?? throw new DirectoryNotFoundException("EngineInstallationsRoot is not configured.");

        Directory.CreateDirectory(installationsRoot);

        var destinationRoot = Path.Combine(installationsRoot, manifest.Version, manifest.Platform);
        if (Directory.Exists(destinationRoot))
        {
            if (!overwriteExisting)
            {
                throw new InvalidOperationException(
                    $"Engine {manifest.Version} ({manifest.Platform}) is already installed.");
            }

            DeleteDirectorySafe(destinationRoot);
        }

        CopyDirectory(packageRoot, destinationRoot);
        return destinationRoot;
    }

    public void Uninstall(Models.EngineInstallation installation)
    {
        if (!installation.IsManagedInstallation)
        {
            throw new InvalidOperationException("Only installed engine packages can be uninstalled.");
        }

        if (!Directory.Exists(installation.RootPath))
        {
            return;
        }

        DeleteDirectorySafe(installation.RootPath);
    }

    private static EngineManifest LoadManifest(string manifestPath)
    {
        using var stream = File.OpenRead(manifestPath);
        return JsonSerializer.Deserialize<EngineManifest>(stream, new JsonSerializerOptions
        {
            PropertyNameCaseInsensitive = true
        }) ?? throw new InvalidOperationException("The selected manifest could not be parsed.");
    }

    private static void CopyDirectory(string sourceDirectory, string targetDirectory)
    {
        Directory.CreateDirectory(targetDirectory);

        foreach (var sourceFilePath in Directory.EnumerateFiles(sourceDirectory, "*", SearchOption.AllDirectories))
        {
            var relativePath = Path.GetRelativePath(sourceDirectory, sourceFilePath);
            var destinationPath = Path.Combine(targetDirectory, relativePath);
            Directory.CreateDirectory(Path.GetDirectoryName(destinationPath)!);
            File.Copy(sourceFilePath, destinationPath, overwrite: true);
        }
    }

    private static void DeleteDirectorySafe(string rootPath)
    {
        if (!Directory.Exists(rootPath))
        {
            return;
        }

        foreach (var filePath in Directory.EnumerateFiles(rootPath, "*", SearchOption.AllDirectories))
        {
            File.SetAttributes(filePath, FileAttributes.Normal);
        }

        foreach (var directoryPath in Directory.EnumerateDirectories(rootPath, "*", SearchOption.AllDirectories)
                     .OrderByDescending(path => path.Length))
        {
            File.SetAttributes(directoryPath, FileAttributes.Normal);
        }

        File.SetAttributes(rootPath, FileAttributes.Normal);
        Directory.Delete(rootPath, recursive: true);
    }

    private sealed class EngineManifest
    {
        public string? Version { get; set; }
        public string? Platform { get; set; }
    }
}
