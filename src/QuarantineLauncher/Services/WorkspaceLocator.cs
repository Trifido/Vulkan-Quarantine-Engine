using System.IO;

namespace QuarantineLauncher.Services;

internal static class WorkspaceLocator
{
    public static string FindRepositoryRoot()
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

        throw new DirectoryNotFoundException("Could not locate the Vulkan-Quarantine-Engine repository root.");
    }

    public static string FindLauncherTemplateRoot()
    {
        var outputTemplatePath = Path.Combine(AppContext.BaseDirectory, "Templates", "DefaultProject");
        if (Directory.Exists(outputTemplatePath))
        {
            return outputTemplatePath;
        }

        var repositoryRoot = FindRepositoryRoot();
        var sourceTemplatePath = Path.Combine(repositoryRoot, "src", "QuarantineLauncher", "Templates", "DefaultProject");
        if (Directory.Exists(sourceTemplatePath))
        {
            return sourceTemplatePath;
        }

        throw new DirectoryNotFoundException("Could not locate the default project template.");
    }

    public static string FindProjectsRoot()
    {
        return Path.Combine(FindRepositoryRoot(), "QEProjects");
    }

    private static IEnumerable<string> EnumerateStartPaths()
    {
        yield return AppContext.BaseDirectory;
        yield return Directory.GetCurrentDirectory();
    }
}
