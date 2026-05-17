using System.IO;
using System.Diagnostics;
using QuarantineLauncher.Models;

namespace QuarantineLauncher.Services;

public sealed class EditorLaunchService
{
    public bool TryResolveEditorPath(ProjectEntry? project, out string? editorPath, out string? errorMessage)
    {
        try
        {
            editorPath = ResolveEditorPath(project);
            errorMessage = null;
            return true;
        }
        catch (Exception ex)
        {
            editorPath = null;
            errorMessage = ex.Message;
            return false;
        }
    }

    public string ResolveEditorPath(ProjectEntry? project = null)
    {
        var packagedEditorPath = ResolvePackagedEditorPath(project);
        if (!string.IsNullOrWhiteSpace(packagedEditorPath))
        {
            return packagedEditorPath;
        }

        return ResolveFallbackEditorPath();
    }

    public string ResolveFallbackEditorPath()
    {
        var configuredEditorPath = WorkspaceLocator.FindConfiguredEditorPath();
        if (!string.IsNullOrWhiteSpace(configuredEditorPath))
        {
            if (File.Exists(configuredEditorPath))
            {
                return configuredEditorPath;
            }

            throw new FileNotFoundException(
                $"Configured EditorPath was not found: '{configuredEditorPath}'.");
        }

        var candidates = GetDevelopmentEditorCandidates();

        return candidates.FirstOrDefault(File.Exists)
            ?? throw new FileNotFoundException(
                "Could not find QuarantineEditor.exe. Configure EditorPath in launcher.settings.json or build the editor in the development workspace.");
    }

    public void LaunchEditor(ProjectEntry project)
    {
        var editorPath = ResolveEditorPath(project);
        var workingDirectory = ResolveWorkingDirectory(editorPath, project);

        var startInfo = new ProcessStartInfo
        {
            FileName = editorPath,
            WorkingDirectory = workingDirectory,
            UseShellExecute = true
        };

        startInfo.ArgumentList.Add(project.FullPath);

        Process.Start(startInfo);
    }

    private static string? ResolvePackagedEditorPath(ProjectEntry? project)
    {
        if (string.IsNullOrWhiteSpace(project?.EngineRootPath))
        {
            return null;
        }

        var editorRoot = Path.Combine(project.EngineRootPath, "editor");
        if (!Directory.Exists(editorRoot))
        {
            return null;
        }

        foreach (var configuration in new[] { "Release", "Debug", "RelWithDebInfo", "MinSizeRel" })
        {
            var candidate = Path.Combine(editorRoot, configuration, "QuarantineEditor.exe");
            if (File.Exists(candidate))
            {
                return candidate;
            }
        }

        return null;
    }

    private static string ResolveWorkingDirectory(string editorPath, ProjectEntry? project)
    {
        if (!string.IsNullOrWhiteSpace(project?.EngineRootPath))
        {
            var packagedEditorRoot = Path.Combine(project.EngineRootPath, "editor");
            var editorDirectory = Path.GetDirectoryName(editorPath);
            if (!string.IsNullOrWhiteSpace(editorDirectory) &&
                editorDirectory.StartsWith(packagedEditorRoot, StringComparison.OrdinalIgnoreCase))
            {
                return project.EngineRootPath;
            }
        }

        return Path.GetDirectoryName(editorPath)!;
    }

    private static IReadOnlyList<string> GetDevelopmentEditorCandidates()
    {
        var candidates = new List<string>
        {
            Path.Combine(AppContext.BaseDirectory, "QuarantineEditor.exe")
        };

        var enginePaths = WorkspaceLocator.ResolveEngineIntegrationPaths();
        AddCandidatesFromBuildRoot(candidates, enginePaths.EngineBuildDir);

        var repositoryRoot = WorkspaceLocator.FindRepositoryRoot();
        if (!string.IsNullOrWhiteSpace(repositoryRoot))
        {
            candidates.Add(Path.Combine(repositoryRoot, "build", "Release", "QuarantineEditor.exe"));
            candidates.Add(Path.Combine(repositoryRoot, "build", "Debug", "QuarantineEditor.exe"));
            candidates.Add(Path.Combine(repositoryRoot, "build", "RelWithDebInfo", "QuarantineEditor.exe"));
            candidates.Add(Path.Combine(repositoryRoot, "build", "MinSizeRel", "QuarantineEditor.exe"));
            AddCandidatesFromBuildRoot(candidates, Path.Combine(repositoryRoot, "build"));
        }

        return candidates
            .Distinct(StringComparer.OrdinalIgnoreCase)
            .ToList();
    }

    private static void AddCandidatesFromBuildRoot(ICollection<string> candidates, string? buildRoot)
    {
        if (string.IsNullOrWhiteSpace(buildRoot) || !Directory.Exists(buildRoot))
        {
            return;
        }

        candidates.Add(Path.Combine(buildRoot, "Release", "QuarantineEditor.exe"));
        candidates.Add(Path.Combine(buildRoot, "Debug", "QuarantineEditor.exe"));
        candidates.Add(Path.Combine(buildRoot, "RelWithDebInfo", "QuarantineEditor.exe"));
        candidates.Add(Path.Combine(buildRoot, "MinSizeRel", "QuarantineEditor.exe"));

        foreach (var candidate in Directory.EnumerateFiles(buildRoot, "QuarantineEditor.exe", SearchOption.AllDirectories))
        {
            candidates.Add(candidate);
        }
    }
}
