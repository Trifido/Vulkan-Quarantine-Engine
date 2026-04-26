using System.IO;
using QuarantineLauncher.Models;

namespace QuarantineLauncher.Services;

public sealed class ProjectRepository
{
    private const string DefaultSceneRelativePath = @"QEScenes\default.qescene";

    public string ProjectsRoot { get; } = WorkspaceLocator.FindProjectsRoot();

    public IReadOnlyList<ProjectEntry> GetProjects()
    {
        Directory.CreateDirectory(ProjectsRoot);

        return Directory.EnumerateDirectories(ProjectsRoot)
            .Select(BuildProjectEntry)
            .Where(project => project is not null)
            .Cast<ProjectEntry>()
            .OrderBy(project => project.Name, StringComparer.OrdinalIgnoreCase)
            .ToList();
    }

    public ProjectEntry CreateProject(string projectName)
    {
        var normalizedName = NormalizeProjectName(projectName);
        var projectPath = GetProjectPath(normalizedName);

        if (Directory.Exists(projectPath))
        {
            throw new InvalidOperationException($"Project '{normalizedName}' already exists.");
        }

        var templateRoot = WorkspaceLocator.FindLauncherTemplateRoot();

        Directory.CreateDirectory(projectPath);
        CopyDirectory(templateRoot, projectPath);
        Directory.CreateDirectory(Path.Combine(projectPath, "QEAssets", "QEModels"));

        return BuildProjectEntry(projectPath)
            ?? throw new InvalidOperationException("The project template did not produce a valid project.");
    }

    public void DeleteProject(string projectName)
    {
        var normalizedName = NormalizeProjectName(projectName);
        var projectPath = GetProjectPath(normalizedName);

        if (!Directory.Exists(projectPath))
        {
            throw new DirectoryNotFoundException($"Project '{normalizedName}' does not exist.");
        }

        EnsurePathInsideProjectsRoot(projectPath);
        Directory.Delete(projectPath, recursive: true);
    }

    public string GetProjectPath(string projectName)
    {
        return Path.Combine(ProjectsRoot, NormalizeProjectName(projectName));
    }

    private ProjectEntry? BuildProjectEntry(string projectPath)
    {
        var scenePath = Path.Combine(projectPath, DefaultSceneRelativePath);
        if (!File.Exists(scenePath))
        {
            return null;
        }

        var directoryInfo = new DirectoryInfo(projectPath);
        return new ProjectEntry
        {
            Name = directoryInfo.Name,
            FullPath = directoryInfo.FullName,
            DefaultScenePath = scenePath,
            LastModifiedUtc = directoryInfo.LastWriteTimeUtc
        };
    }

    private string NormalizeProjectName(string projectName)
    {
        var trimmedName = projectName.Trim();
        if (string.IsNullOrWhiteSpace(trimmedName))
        {
            throw new InvalidOperationException("Project name cannot be empty.");
        }

        if (trimmedName.IndexOfAny(Path.GetInvalidFileNameChars()) >= 0)
        {
            throw new InvalidOperationException("Project name contains invalid characters.");
        }

        return trimmedName;
    }

    private void EnsurePathInsideProjectsRoot(string projectPath)
    {
        var normalizedRoot = Path.GetFullPath(ProjectsRoot)
            .TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar)
            + Path.DirectorySeparatorChar;

        var normalizedProjectPath = Path.GetFullPath(projectPath)
            .TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar)
            + Path.DirectorySeparatorChar;

        if (!normalizedProjectPath.StartsWith(normalizedRoot, StringComparison.OrdinalIgnoreCase))
        {
            throw new InvalidOperationException("Refusing to operate outside QEProjects.");
        }
    }

    private static void CopyDirectory(string sourceDirectory, string targetDirectory)
    {
        Directory.CreateDirectory(targetDirectory);

        foreach (var sourceFilePath in Directory.EnumerateFiles(sourceDirectory, "*", SearchOption.AllDirectories))
        {
            var relativePath = Path.GetRelativePath(sourceDirectory, sourceFilePath);
            var destinationPath = Path.Combine(targetDirectory, relativePath);
            Directory.CreateDirectory(Path.GetDirectoryName(destinationPath)!);
            File.Copy(sourceFilePath, destinationPath, overwrite: false);
        }
    }
}
