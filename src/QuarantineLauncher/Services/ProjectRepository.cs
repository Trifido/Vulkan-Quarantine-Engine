using System.IO;
using System.Security;
using System.Text;
using System.Text.Json;
using System.Xml.Linq;
using QuarantineLauncher.Models;

namespace QuarantineLauncher.Services;

public sealed class ProjectRepository
{
    private const string DefaultSceneRelativePath = @"QEScenes\default.qescene";
    private const string ProjectMetadataFileName = "qe.project.json";

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

    public ProjectEntry CreateProject(
        string projectName,
        EngineIntegrationPaths? enginePathsOverride = null,
        EngineInstallation? engineInstallation = null)
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
        MaterializeEngineIntegrationProps(projectPath, enginePathsOverride ?? WorkspaceLocator.ResolveEngineIntegrationPaths());
        MaterializeProjectMetadata(projectPath, engineInstallation);

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
        var metadata = ReadProjectMetadata(projectPath) ?? ReadProjectMetadataFromEngineProps(projectPath);
        return new ProjectEntry
        {
            Name = directoryInfo.Name,
            FullPath = directoryInfo.FullName,
            DefaultScenePath = scenePath,
            LastModifiedUtc = directoryInfo.LastWriteTimeUtc,
            EngineDisplayName = metadata?.DisplayName,
            EngineVersion = metadata?.Version
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

    private static void MaterializeEngineIntegrationProps(
        string projectPath,
        EngineIntegrationPaths enginePaths)
    {
        var propsPath = Path.Combine(projectPath, "VisualStudio", "QuarantineEngine.Local.props");
        var installLayout = IsInstalledEnginePackage(enginePaths.EngineRoot);

        var engineRoot = enginePaths.EngineRoot;
        var engineBinaryDir = installLayout
            ? Path.Combine(engineRoot, "bin")
            : enginePaths.EngineBuildDir;
        var engineBuildDir = installLayout
            ? engineBinaryDir
            : enginePaths.EngineBuildDir;

        var includeDirectories = installLayout
            ? BuildInstalledPackageIncludeDirectories(engineRoot)
            : BuildDevelopmentWorkspaceIncludeDirectories(engineRoot, enginePaths.EngineBuildDir);

        var libraryDependencies = installLayout
            ? BuildInstalledPackageLibraryDependencies(engineRoot)
            : BuildDevelopmentWorkspaceLibraryDependencies(enginePaths.EngineBuildDir);

        var content = $"""
<?xml version="1.0" encoding="utf-8"?>
<Project ToolsVersion="Current" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <PropertyGroup>
    <QuarantineEngineRoot>{EscapeXml(engineRoot)}</QuarantineEngineRoot>
    <QuarantineEngineBuildDir>{EscapeXml(EnsureTrailingSeparator(engineBuildDir))}</QuarantineEngineBuildDir>
    <QEEngineBinaryDir>{EscapeXml(EnsureTrailingSeparator(engineBinaryDir))}$(Configuration)\</QEEngineBinaryDir>
    <QEEngineIncludeDirectories>{EscapeXml(includeDirectories)}</QEEngineIncludeDirectories>
    <QEEngineLibraryDependencies>{EscapeXml(libraryDependencies)}</QEEngineLibraryDependencies>
  </PropertyGroup>
</Project>
""";

        File.WriteAllText(propsPath, content, new UTF8Encoding(false));
    }

    private static void MaterializeProjectMetadata(string projectPath, EngineInstallation? engineInstallation)
    {
        if (engineInstallation is null)
        {
            return;
        }

        var metadata = new ProjectEngineMetadata
        {
            DisplayName = engineInstallation.DisplayName,
            Version = engineInstallation.Version,
            RootPath = engineInstallation.RootPath,
            Platform = engineInstallation.Platform
        };

        var metadataPath = Path.Combine(projectPath, ProjectMetadataFileName);
        var json = JsonSerializer.Serialize(metadata, new JsonSerializerOptions
        {
            WriteIndented = true
        });

        File.WriteAllText(metadataPath, json, new UTF8Encoding(false));
    }

    private static ProjectEngineMetadata? ReadProjectMetadata(string projectPath)
    {
        var metadataPath = Path.Combine(projectPath, ProjectMetadataFileName);
        if (!File.Exists(metadataPath))
        {
            return null;
        }

        try
        {
            var json = File.ReadAllText(metadataPath, Encoding.UTF8);
            return JsonSerializer.Deserialize<ProjectEngineMetadata>(json, new JsonSerializerOptions
            {
                PropertyNameCaseInsensitive = true
            });
        }
        catch
        {
            return null;
        }
    }

    private static ProjectEngineMetadata? ReadProjectMetadataFromEngineProps(string projectPath)
    {
        var propsPath = Path.Combine(projectPath, "VisualStudio", "QuarantineEngine.Local.props");
        if (!File.Exists(propsPath))
        {
            return null;
        }

        try
        {
            var document = XDocument.Load(propsPath);
            var propertyGroup = document.Root?.Element(document.Root.Name.Namespace + "PropertyGroup");
            if (propertyGroup is null)
            {
                return null;
            }

            var engineRoot = propertyGroup.Element(document.Root!.Name.Namespace + "QuarantineEngineRoot")?.Value;
            if (string.IsNullOrWhiteSpace(engineRoot))
            {
                return null;
            }

            var manifestPath = Path.Combine(engineRoot, "metadata", "manifest.json");
            if (!File.Exists(manifestPath))
            {
                return new ProjectEngineMetadata
                {
                    DisplayName = "Current Development Workspace",
                    Version = "workspace",
                    RootPath = engineRoot,
                    Platform = "win-x64"
                };
            }

            var json = File.ReadAllText(manifestPath, Encoding.UTF8);
            var metadata = JsonSerializer.Deserialize<ProjectEngineMetadata>(json, new JsonSerializerOptions
            {
                PropertyNameCaseInsensitive = true
            });

            if (metadata is null)
            {
                return null;
            }

            metadata.DisplayName ??= $"Quarantine Engine {metadata.Version}";
            metadata.RootPath ??= engineRoot;
            metadata.Platform ??= "win-x64";
            return metadata;
        }
        catch
        {
            return null;
        }
    }

    private static bool IsInstalledEnginePackage(string engineRoot)
    {
        var manifestPath = Path.Combine(engineRoot, "metadata", "manifest.json");
        return File.Exists(manifestPath);
    }

    private static string BuildDevelopmentWorkspaceIncludeDirectories(string engineRoot, string engineBuildDir)
    {
        return string.Join(';', new[]
        {
            $@"{engineRoot}\src",
            $@"{engineRoot}\src\QuarantineEngine",
            $@"{engineRoot}\src\QuarantineEngine\App",
            $@"{engineRoot}\src\QuarantineEngine\Data",
            $@"{engineRoot}\src\QuarantineEngine\Data\Dtos",
            $@"{engineRoot}\src\QuarantineEngine\Draw",
            $@"{engineRoot}\src\QuarantineEngine\GraphicsPipeline",
            $@"{engineRoot}\src\QuarantineEngine\GUI",
            $@"{engineRoot}\src\QuarantineEngine\Helpers",
            $@"{engineRoot}\src\QuarantineEngine\Input",
            $@"{engineRoot}\src\QuarantineEngine\Logging",
            $@"{engineRoot}\src\QuarantineEngine\Memory",
            $@"{engineRoot}\src\QuarantineEngine\Presentation",
            $@"{engineRoot}\src\QuarantineEngine\RayTracing",
            $@"{engineRoot}\src\QuarantineEngine\SetUp",
            $@"{engineRoot}\src\QuarantineEngine\Templates",
            $@"{engineRoot}\src\QuarantineEngine\Utilities",
            $@"{engineRoot}\src\QuarantineEngine\Utilities\Animation",
            $@"{engineRoot}\src\QuarantineEngine\Utilities\Atmosphere",
            $@"{engineRoot}\src\QuarantineEngine\Utilities\Camera",
            $@"{engineRoot}\src\QuarantineEngine\Utilities\Compute",
            $@"{engineRoot}\src\QuarantineEngine\Utilities\Controller",
            $@"{engineRoot}\src\QuarantineEngine\Utilities\DebugSystem",
            $@"{engineRoot}\src\QuarantineEngine\Utilities\General",
            $@"{engineRoot}\src\QuarantineEngine\Utilities\Geometry",
            $@"{engineRoot}\src\QuarantineEngine\Utilities\Light",
            $@"{engineRoot}\src\QuarantineEngine\Utilities\Material",
            $@"{engineRoot}\src\QuarantineEngine\Utilities\Particles",
            $@"{engineRoot}\src\QuarantineEngine\Utilities\Physics",
            $@"{engineRoot}\extern",
            $@"{engineRoot}\extern\assimp\include",
            $@"{engineRoot}\extern\imgui",
            $@"{engineRoot}\extern\imgui\backends",
            $@"{engineRoot}\extern\jolt",
            $@"{engineRoot}\extern\meshoptimizer\src",
            $@"{engineRoot}\extern\SPIRV-Reflect",
            $@"{engineBuildDir}\_deps\glfw-src\include",
            $@"{engineBuildDir}\_deps\glm-src",
            $@"{engineBuildDir}\_deps\stb-src",
            $@"{engineBuildDir}\_deps\yaml-cpp-src\include",
            $@"{engineBuildDir}\_deps\ktx-src\include"
        });
    }

    private static string BuildInstalledPackageIncludeDirectories(string engineRoot)
    {
        return string.Join(';', new[]
        {
            $@"{engineRoot}\src",
            $@"{engineRoot}\src\QuarantineEngine",
            $@"{engineRoot}\src\QuarantineEngine\App",
            $@"{engineRoot}\src\QuarantineEngine\Data",
            $@"{engineRoot}\src\QuarantineEngine\Data\Dtos",
            $@"{engineRoot}\src\QuarantineEngine\Draw",
            $@"{engineRoot}\src\QuarantineEngine\GraphicsPipeline",
            $@"{engineRoot}\src\QuarantineEngine\GUI",
            $@"{engineRoot}\src\QuarantineEngine\Helpers",
            $@"{engineRoot}\src\QuarantineEngine\Input",
            $@"{engineRoot}\src\QuarantineEngine\Logging",
            $@"{engineRoot}\src\QuarantineEngine\Memory",
            $@"{engineRoot}\src\QuarantineEngine\Presentation",
            $@"{engineRoot}\src\QuarantineEngine\RayTracing",
            $@"{engineRoot}\src\QuarantineEngine\SetUp",
            $@"{engineRoot}\src\QuarantineEngine\Templates",
            $@"{engineRoot}\src\QuarantineEngine\Utilities",
            $@"{engineRoot}\src\QuarantineEngine\Utilities\Animation",
            $@"{engineRoot}\src\QuarantineEngine\Utilities\Atmosphere",
            $@"{engineRoot}\src\QuarantineEngine\Utilities\Camera",
            $@"{engineRoot}\src\QuarantineEngine\Utilities\Compute",
            $@"{engineRoot}\src\QuarantineEngine\Utilities\Controller",
            $@"{engineRoot}\src\QuarantineEngine\Utilities\DebugSystem",
            $@"{engineRoot}\src\QuarantineEngine\Utilities\General",
            $@"{engineRoot}\src\QuarantineEngine\Utilities\Geometry",
            $@"{engineRoot}\src\QuarantineEngine\Utilities\Light",
            $@"{engineRoot}\src\QuarantineEngine\Utilities\Material",
            $@"{engineRoot}\src\QuarantineEngine\Utilities\Particles",
            $@"{engineRoot}\src\QuarantineEngine\Utilities\Physics",
            $@"{engineRoot}\extern",
            $@"{engineRoot}\extern\assimp\include",
            $@"{engineRoot}\extern\imgui",
            $@"{engineRoot}\extern\imgui\backends",
            $@"{engineRoot}\extern\jolt",
            $@"{engineRoot}\extern\meshoptimizer\src",
            $@"{engineRoot}\extern\SPIRV-Reflect",
            $@"{engineRoot}\_deps\glfw-src\include",
            $@"{engineRoot}\_deps\glm-src",
            $@"{engineRoot}\_deps\stb-src",
            $@"{engineRoot}\_deps\yaml-cpp-src\include",
            $@"{engineRoot}\_deps\ktx-src\include"
        });
    }

    private static string BuildDevelopmentWorkspaceLibraryDependencies(string engineBuildDir)
    {
        return string.Join(';', new[]
        {
            $@"{engineBuildDir}\$(Configuration)\QuarantineEngine$(QEDebugSuffix).lib",
            $@"{engineBuildDir}\extern\assimp\lib\$(Configuration)\$(QEAssimpLibraryName)",
            $@"{engineBuildDir}\extern\jolt\Build\$(Configuration)\Jolt.lib",
            $@"{engineBuildDir}\extern\meshoptimizer\$(Configuration)\meshoptimizer$(QEDebugSuffix).lib",
            $@"{engineBuildDir}\_deps\yaml-cpp-build\$(Configuration)\yaml-cpp$(QEDebugSuffix).lib",
            $@"{engineBuildDir}\_deps\ktx-build\$(Configuration)\ktx.lib",
            $@"{engineBuildDir}\_deps\glfw-build\src\$(Configuration)\glfw3$(QEDebugSuffix).lib",
            $@"{engineBuildDir}\$(Configuration)\SPIRV-Reflect$(QEDebugSuffix).lib",
            $@"{engineBuildDir}\$(Configuration)\imgui$(QEDebugSuffix).lib"
        });
    }

    private static string BuildInstalledPackageLibraryDependencies(string engineRoot)
    {
        return string.Join(';', new[]
        {
            $@"{engineRoot}\lib\$(Configuration)\QuarantineEngine$(QEDebugSuffix).lib",
            $@"{engineRoot}\lib\$(Configuration)\$(QEAssimpLibraryName)",
            $@"{engineRoot}\lib\$(Configuration)\Jolt.lib",
            $@"{engineRoot}\lib\$(Configuration)\meshoptimizer$(QEDebugSuffix).lib",
            $@"{engineRoot}\lib\$(Configuration)\yaml-cpp$(QEDebugSuffix).lib",
            $@"{engineRoot}\lib\$(Configuration)\ktx.lib",
            $@"{engineRoot}\lib\$(Configuration)\glfw3$(QEDebugSuffix).lib",
            $@"{engineRoot}\lib\$(Configuration)\SPIRV-Reflect$(QEDebugSuffix).lib",
            $@"{engineRoot}\lib\$(Configuration)\imgui$(QEDebugSuffix).lib"
        });
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
               extension.Equals(".json", StringComparison.OrdinalIgnoreCase) ||
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

    private static string EnsureTrailingSeparator(string path)
    {
        if (path.EndsWith(Path.DirectorySeparatorChar) || path.EndsWith(Path.AltDirectorySeparatorChar))
        {
            return path;
        }

        return path + Path.DirectorySeparatorChar;
    }

    private static string EscapeXml(string value)
    {
        return SecurityElement.Escape(value) ?? value;
    }
}
