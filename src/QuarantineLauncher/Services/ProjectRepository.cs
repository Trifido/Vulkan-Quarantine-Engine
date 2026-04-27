using System.IO;
using System.Text;
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
        MaterializeVisualStudioTemplate(projectPath, normalizedName);

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

    private static void MaterializeVisualStudioTemplate(string projectPath, string projectName)
    {
        var placeholders = new Dictionary<string, string>(StringComparer.Ordinal)
        {
            ["__QE_PROJECT_NAME__"] = projectName,
            ["__QE_GAME_PROJECT_GUID__"] = Guid.NewGuid().ToString().ToUpperInvariant(),
            ["__QE_RUNTIME_PROJECT_GUID__"] = Guid.NewGuid().ToString().ToUpperInvariant()
        };

        RenameTemplatePaths(projectPath, placeholders);
        ReplaceTemplateContent(projectPath, placeholders);
    }

    private static void RenameTemplatePaths(string rootPath, IReadOnlyDictionary<string, string> placeholders)
    {
        var entries = Directory.EnumerateFileSystemEntries(rootPath, "*", SearchOption.AllDirectories)
            .OrderByDescending(path => path.Length)
            .ToList();

        foreach (var originalPath in entries)
        {
            var directory = Path.GetDirectoryName(originalPath);
            var name = Path.GetFileName(originalPath);
            var renamed = ReplaceTokens(name, placeholders);

            if (directory is null || string.Equals(name, renamed, StringComparison.Ordinal))
            {
                continue;
            }

            var targetPath = Path.Combine(directory, renamed);
            if (Directory.Exists(originalPath))
            {
                Directory.Move(originalPath, targetPath);
            }
            else if (File.Exists(originalPath))
            {
                File.Move(originalPath, targetPath);
            }
        }
    }

    private static void ReplaceTemplateContent(string rootPath, IReadOnlyDictionary<string, string> placeholders)
    {
        foreach (var filePath in Directory.EnumerateFiles(rootPath, "*", SearchOption.AllDirectories))
        {
            var extension = Path.GetExtension(filePath);
            if (!IsTextTemplateFile(extension, Path.GetFileName(filePath)))
            {
                continue;
            }

            var content = File.ReadAllText(filePath, Encoding.UTF8);
            var updatedContent = ReplaceTokens(content, placeholders);
            if (!string.Equals(content, updatedContent, StringComparison.Ordinal))
            {
                File.WriteAllText(filePath, updatedContent, new UTF8Encoding(false));
            }
        }
    }

    private static bool IsTextTemplateFile(string extension, string fileName)
    {
        return extension.Equals(".sln", StringComparison.OrdinalIgnoreCase) ||
               extension.Equals(".vcxproj", StringComparison.OrdinalIgnoreCase) ||
               extension.Equals(".filters", StringComparison.OrdinalIgnoreCase) ||
               extension.Equals(".props", StringComparison.OrdinalIgnoreCase) ||
               extension.Equals(".cpp", StringComparison.OrdinalIgnoreCase) ||
               extension.Equals(".h", StringComparison.OrdinalIgnoreCase) ||
               extension.Equals(".hpp", StringComparison.OrdinalIgnoreCase) ||
               extension.Equals(".qemat", StringComparison.OrdinalIgnoreCase) ||
               extension.Equals(".qescene", StringComparison.OrdinalIgnoreCase) ||
               extension.Equals(".qeeditor", StringComparison.OrdinalIgnoreCase) ||
               string.Equals(fileName, ".gitignore", StringComparison.OrdinalIgnoreCase);
    }

    private static string ReplaceTokens(string value, IReadOnlyDictionary<string, string> placeholders)
    {
        var result = value;
        foreach (var entry in placeholders)
        {
            result = result.Replace(entry.Key, entry.Value, StringComparison.Ordinal);
        }

        return result;
    }
}
