using System.IO;
using System.Diagnostics;

namespace QuarantineLauncher.Services;

public sealed class EditorLaunchService
{
    public string ResolveEditorPath()
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

    public void LaunchEditor(string projectPath)
    {
        var editorPath = ResolveEditorPath();

        var startInfo = new ProcessStartInfo
        {
            FileName = editorPath,
            WorkingDirectory = Path.GetDirectoryName(editorPath)!,
            UseShellExecute = true
        };

        startInfo.ArgumentList.Add(projectPath);

        Process.Start(startInfo);
    }

    private static IReadOnlyList<string> GetDevelopmentEditorCandidates()
    {
        var candidates = new List<string>
        {
            Path.Combine(AppContext.BaseDirectory, "QuarantineEditor.exe")
        };

        var repositoryRoot = WorkspaceLocator.FindRepositoryRoot();
        if (!string.IsNullOrWhiteSpace(repositoryRoot))
        {
            candidates.Add(Path.Combine(repositoryRoot, "build", "Release", "QuarantineEditor.exe"));
            candidates.Add(Path.Combine(repositoryRoot, "build", "Debug", "QuarantineEditor.exe"));
            candidates.Add(Path.Combine(repositoryRoot, "build", "RelWithDebInfo", "QuarantineEditor.exe"));
            candidates.Add(Path.Combine(repositoryRoot, "build", "MinSizeRel", "QuarantineEditor.exe"));
        }

        return candidates;
    }
}
