using System.IO;
using System.IO.Compression;
using System.Net.Http;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;
using QuarantineLauncher.Models;

namespace QuarantineLauncher.Services;

public sealed class EnginePackageImportService
{
    private static readonly HttpClient HttpClient = new();

    public Task<string> InstallFromFeedPackageAsync(
        EngineInstallation installation,
        bool overwriteExisting = false,
        IProgress<EnginePackageInstallProgress>? progress = null,
        CancellationToken cancellationToken = default)
    {
        return installation.ManifestPath is not null && File.Exists(installation.ManifestPath)
            ? Task.Run(() =>
            {
                progress?.Report(new EnginePackageInstallProgress("Copying local package...", null));
                var result = InstallFromPackageManifest(installation.ManifestPath, overwriteExisting);
                progress?.Report(new EnginePackageInstallProgress("Package installed.", 1.0));
                return result;
            }, cancellationToken)
            : InstallFromArchiveSourceAsync(installation, overwriteExisting, progress, cancellationToken);
    }

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

    private static async Task<string> InstallFromArchiveSourceAsync(
        EngineInstallation installation,
        bool overwriteExisting,
        IProgress<EnginePackageInstallProgress>? progress,
        CancellationToken cancellationToken)
    {
        if (string.IsNullOrWhiteSpace(installation.PackageSource))
        {
            throw new InvalidOperationException("The selected feed package does not expose a downloadable archive.");
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

        progress?.Report(new EnginePackageInstallProgress("Downloading engine package...", 0.0));
        var temporaryArchivePath = await DownloadArchiveAsync(installation.PackageSource, progress, cancellationToken);

        try
        {
            progress?.Report(new EnginePackageInstallProgress("Extracting engine package...", null));
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

        progress?.Report(new EnginePackageInstallProgress("Package installed.", 1.0));
        return destinationRoot;
    }

    private static async Task<string> DownloadArchiveAsync(
        string packageSource,
        IProgress<EnginePackageInstallProgress>? progress,
        CancellationToken cancellationToken)
    {
        if (Uri.TryCreate(packageSource, UriKind.Absolute, out var packageUri))
        {
            if (packageUri.IsFile)
            {
                progress?.Report(new EnginePackageInstallProgress("Using local package archive...", 1.0));
                return packageUri.LocalPath;
            }

            if (packageUri.Scheme == Uri.UriSchemeHttp || packageUri.Scheme == Uri.UriSchemeHttps)
            {
                var temporaryArchivePath = Path.Combine(Path.GetTempPath(), $"{Guid.NewGuid():N}.zip");
                using var response = await HttpClient.GetAsync(packageUri, HttpCompletionOption.ResponseHeadersRead, cancellationToken);
                response.EnsureSuccessStatusCode();

                var totalBytes = response.Content.Headers.ContentLength;
                await using var responseStream = await response.Content.ReadAsStreamAsync(cancellationToken);
                await using var outputStream = File.Create(temporaryArchivePath);

                var buffer = new byte[81920];
                long totalRead = 0;
                int bytesRead;

                while ((bytesRead = await responseStream.ReadAsync(buffer, cancellationToken)) > 0)
                {
                    await outputStream.WriteAsync(buffer.AsMemory(0, bytesRead), cancellationToken);
                    totalRead += bytesRead;

                    double? fraction = totalBytes is > 0 ? (double)totalRead / totalBytes.Value : null;
                    progress?.Report(new EnginePackageInstallProgress("Downloading engine package...", fraction));
                }

                return temporaryArchivePath;
            }
        }

        if (File.Exists(packageSource))
        {
            progress?.Report(new EnginePackageInstallProgress("Using local package archive...", 1.0));
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
