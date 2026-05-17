using System.IO;
using System.IO.Compression;
using System.Net.Http;
using System.Text.Json;
using QuarantineLauncher.Models;

namespace QuarantineLauncher.Services;

public sealed class EnginePackageImportService
{
    private static readonly HttpClient HttpClient = new();

    public string InstallFromFeedPackage(EngineInstallation installation, bool overwriteExisting = false)
    {
        if (installation.ManifestPath is not null && File.Exists(installation.ManifestPath))
        {
            return InstallFromPackageManifest(installation.ManifestPath, overwriteExisting);
        }

        if (string.IsNullOrWhiteSpace(installation.PackageSource))
        {
            throw new InvalidOperationException("The selected feed package does not expose a local manifest or downloadable archive.");
        }

        var installationsRoot = WorkspaceLocator.FindEngineInstallationsRoot()
            ?? throw new DirectoryNotFoundException("EngineInstallationsRoot is not configured.");

        Directory.CreateDirectory(installationsRoot);

        var destinationRoot = Path.Combine(installationsRoot, installation.Version, installation.Platform);
        if (Directory.Exists(destinationRoot))
        {
            if (!overwriteExisting)
            {
                throw new InvalidOperationException(
                    $"Engine {installation.Version} ({installation.Platform}) is already installed.");
            }

            DeleteDirectorySafe(destinationRoot);
        }

        var temporaryArchivePath = TryPrepareArchive(installation.PackageSource);
        try
        {
            Directory.CreateDirectory(destinationRoot);
            ZipFile.ExtractToDirectory(temporaryArchivePath, destinationRoot, overwriteFiles: true);
        }
        catch
        {
            if (Directory.Exists(destinationRoot))
            {
                DeleteDirectorySafe(destinationRoot);
            }

            throw;
        }
        finally
        {
            if (!string.Equals(temporaryArchivePath, installation.PackageSource, StringComparison.OrdinalIgnoreCase) &&
                File.Exists(temporaryArchivePath))
            {
                File.Delete(temporaryArchivePath);
            }
        }

        var manifestPath = Path.Combine(destinationRoot, "metadata", "manifest.json");
        if (!File.Exists(manifestPath))
        {
            DeleteDirectorySafe(destinationRoot);
            throw new InvalidOperationException("The downloaded engine archive is missing metadata/manifest.json.");
        }

        return destinationRoot;
    }

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

    private static string TryPrepareArchive(string packageSource)
    {
        if (Uri.TryCreate(packageSource, UriKind.Absolute, out var packageUri))
        {
            if (packageUri.IsFile)
            {
                return packageUri.LocalPath;
            }

            if (packageUri.Scheme == Uri.UriSchemeHttp || packageUri.Scheme == Uri.UriSchemeHttps)
            {
                var temporaryArchivePath = Path.Combine(Path.GetTempPath(), $"{Guid.NewGuid():N}.zip");
                using var responseStream = HttpClient.GetStreamAsync(packageUri).GetAwaiter().GetResult();
                using var outputStream = File.Create(temporaryArchivePath);
                responseStream.CopyTo(outputStream);
                return temporaryArchivePath;
            }
        }

        if (File.Exists(packageSource))
        {
            return packageSource;
        }

        throw new FileNotFoundException("The package archive could not be located.", packageSource);
    }

    private sealed class EngineManifest
    {
        public string? Version { get; set; }
        public string? Platform { get; set; }
    }
}
