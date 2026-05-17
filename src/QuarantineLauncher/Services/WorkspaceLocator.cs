using System.IO;
using System.Text.Json;

namespace QuarantineLauncher.Services;

internal static class WorkspaceLocator
{
    private const string SettingsFileName = "launcher.settings.json";
    private static Lazy<ResolvedLauncherPaths> _cachedPaths = new(ResolvePaths);

    public static string GetSettingsPath()
    {
        return Path.Combine(AppContext.BaseDirectory, SettingsFileName);
    }

    public static void Reload()
    {
        _cachedPaths = new Lazy<ResolvedLauncherPaths>(ResolvePaths);
    }

    public static EngineIntegrationPaths ResolveEngineIntegrationPaths()
    {
        return _cachedPaths.Value.EnginePaths;
    }

    public static string? FindEngineInstallationsRoot()
    {
        return _cachedPaths.Value.EngineInstallationsRoot;
    }

    public static string? FindEnginePackageFeedRoot()
    {
        return _cachedPaths.Value.EnginePackageFeedRoot;
    }

    public static string? FindEngineFeedIndexPath()
    {
        return _cachedPaths.Value.EngineFeedIndexPath;
    }

    public static string FindProjectsRoot()
    {
        return _cachedPaths.Value.ProjectsRoot;
    }

    public static string FindLauncherTemplateRoot()
    {
        return _cachedPaths.Value.TemplatesRoot;
    }

    public static string? FindConfiguredEditorPath()
    {
        return _cachedPaths.Value.EditorPath;
    }

    public static string? FindRepositoryRoot()
    {
        foreach (var startPath in EnumerateStartPaths())
        {
            var current = new DirectoryInfo(startPath);
            while (current is not null)
            {
                var cmakePath = Path.Combine(current.FullName, "CMakeLists.txt");
                var editorPath = Path.Combine(current.FullName, "src", "QuarantineEditor");
                if (File.Exists(cmakePath) && Directory.Exists(editorPath))
                {
                    return current.FullName;
                }

                current = current.Parent;
            }
        }

        return null;
    }

    private static ResolvedLauncherPaths ResolvePaths()
    {
        var settingsPath = Path.Combine(AppContext.BaseDirectory, SettingsFileName);
        var settings = LoadSettings(settingsPath);
        var settingsDirectory = Path.GetDirectoryName(settingsPath) ?? AppContext.BaseDirectory;
        var repositoryRoot = FindRepositoryRoot();

        var installationsRoot = ResolveEngineInstallationsRoot(settings, settingsDirectory, repositoryRoot);
        var packageFeedRoot = ResolveEnginePackageFeedRoot(settings, settingsDirectory, repositoryRoot);
        var feedIndexPath = ResolveEngineFeedIndexPath(settings, settingsDirectory, packageFeedRoot);
        var enginePaths = ResolveEngineIntegrationPaths(settings, settingsDirectory, repositoryRoot);
        var projectsRoot = ResolveProjectsRoot(settings, settingsDirectory, repositoryRoot);
        var templatesRoot = ResolveTemplatesRoot(settings, settingsDirectory, repositoryRoot);
        var editorPath = ResolveEditorPath(settings, settingsDirectory);

        return new ResolvedLauncherPaths(installationsRoot, packageFeedRoot, feedIndexPath, enginePaths, projectsRoot, templatesRoot, editorPath);
    }

    private static LauncherSettings LoadSettings(string settingsPath)
    {
        if (!File.Exists(settingsPath))
        {
            return new LauncherSettings();
        }

        using var stream = File.OpenRead(settingsPath);
        return JsonSerializer.Deserialize<LauncherSettings>(stream, new JsonSerializerOptions
        {
            PropertyNameCaseInsensitive = true
        }) ?? new LauncherSettings();
    }

    private static string ResolveProjectsRoot(
        LauncherSettings settings,
        string settingsDirectory,
        string? repositoryRoot)
    {
        if (!string.IsNullOrWhiteSpace(settings.ProjectsRoot))
        {
            return NormalizeConfiguredPath(settings.ProjectsRoot, settingsDirectory);
        }

        if (!string.IsNullOrWhiteSpace(repositoryRoot))
        {
            return Path.Combine(repositoryRoot, "QEProjects");
        }

        var documentsPath = Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments);
        return Path.Combine(documentsPath, "Quarantine Engine", "Projects");
    }

    private static string ResolveTemplatesRoot(
        LauncherSettings settings,
        string settingsDirectory,
        string? repositoryRoot)
    {
        if (!string.IsNullOrWhiteSpace(settings.TemplatesRoot))
        {
            var configuredPath = NormalizeConfiguredPath(settings.TemplatesRoot, settingsDirectory);
            if (Directory.Exists(configuredPath))
            {
                return configuredPath;
            }

            throw new DirectoryNotFoundException(
                $"Configured TemplatesRoot does not exist: '{configuredPath}'.");
        }

        var outputTemplatePath = Path.Combine(AppContext.BaseDirectory, "Templates", "DefaultProject");
        if (Directory.Exists(outputTemplatePath))
        {
            return outputTemplatePath;
        }

        if (!string.IsNullOrWhiteSpace(repositoryRoot))
        {
            var sourceTemplatePath = Path.Combine(
                repositoryRoot,
                "src",
                "QuarantineLauncher",
                "Templates",
                "DefaultProject");

            if (Directory.Exists(sourceTemplatePath))
            {
                return sourceTemplatePath;
            }
        }

        throw new DirectoryNotFoundException("Could not locate the default project template.");
    }

    private static EngineIntegrationPaths ResolveEngineIntegrationPaths(
        LauncherSettings settings,
        string settingsDirectory,
        string? repositoryRoot)
    {
        var engineRoot = !string.IsNullOrWhiteSpace(settings.EngineRoot)
            ? NormalizeConfiguredPath(settings.EngineRoot, settingsDirectory)
            : repositoryRoot;

        if (string.IsNullOrWhiteSpace(engineRoot))
        {
            throw new DirectoryNotFoundException(
                "Could not resolve EngineRoot. Configure EngineRoot in launcher.settings.json.");
        }

        var engineBuildDir = !string.IsNullOrWhiteSpace(settings.EngineBuildDir)
            ? NormalizeConfiguredPath(settings.EngineBuildDir, settingsDirectory)
            : Path.Combine(engineRoot, "build");

        return new EngineIntegrationPaths(engineRoot, engineBuildDir);
    }

    private static string? ResolveEngineInstallationsRoot(
        LauncherSettings settings,
        string settingsDirectory,
        string? repositoryRoot)
    {
        if (!string.IsNullOrWhiteSpace(settings.EngineInstallationsRoot))
        {
            return NormalizeConfiguredPath(settings.EngineInstallationsRoot, settingsDirectory);
        }

        var documentsPath = Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments);
        return Path.Combine(documentsPath, "Quarantine Engine", "Engines");
    }

    private static string? ResolveEnginePackageFeedRoot(
        LauncherSettings settings,
        string settingsDirectory,
        string? repositoryRoot)
    {
        if (!string.IsNullOrWhiteSpace(settings.EnginePackageFeedRoot))
        {
            return NormalizeConfiguredPath(settings.EnginePackageFeedRoot, settingsDirectory);
        }

        if (!string.IsNullOrWhiteSpace(repositoryRoot))
        {
            return Path.Combine(repositoryRoot, "artifacts", "packages", "quarantine-engine");
        }

        return null;
    }

    private static string? ResolveEngineFeedIndexPath(
        LauncherSettings settings,
        string settingsDirectory,
        string? packageFeedRoot)
    {
        if (!string.IsNullOrWhiteSpace(settings.EngineFeedIndexPath))
        {
            return NormalizeConfiguredPath(settings.EngineFeedIndexPath, settingsDirectory);
        }

        if (string.IsNullOrWhiteSpace(packageFeedRoot))
        {
            return null;
        }

        return Path.Combine(packageFeedRoot, "index.json");
    }

    private static string? ResolveEditorPath(LauncherSettings settings, string settingsDirectory)
    {
        if (string.IsNullOrWhiteSpace(settings.EditorPath))
        {
            return null;
        }

        return NormalizeConfiguredPath(settings.EditorPath, settingsDirectory);
    }

    private static string NormalizeConfiguredPath(string path, string baseDirectory)
    {
        if (Uri.TryCreate(path, UriKind.Absolute, out var absoluteUri) &&
            (absoluteUri.Scheme == Uri.UriSchemeHttp ||
             absoluteUri.Scheme == Uri.UriSchemeHttps))
        {
            return absoluteUri.ToString();
        }

        if (Path.IsPathRooted(path))
        {
            return Path.GetFullPath(path);
        }

        return Path.GetFullPath(Path.Combine(baseDirectory, path));
    }

    private static IEnumerable<string> EnumerateStartPaths()
    {
        yield return AppContext.BaseDirectory;
        yield return Directory.GetCurrentDirectory();
    }

    private sealed record ResolvedLauncherPaths(
        string? EngineInstallationsRoot,
        string? EnginePackageFeedRoot,
        string? EngineFeedIndexPath,
        EngineIntegrationPaths EnginePaths,
        string ProjectsRoot,
        string TemplatesRoot,
        string? EditorPath);
}
