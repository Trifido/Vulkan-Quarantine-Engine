using System.IO;
using System.Diagnostics;

namespace QuarantineLauncher.Services;

public sealed class EditorLaunchService
{
    public string ResolveEditorPath()
    {
        var repositoryRoot = WorkspaceLocator.FindRepositoryRoot();
        var candidates = new[]
        {
            Path.Combine(repositoryRoot, "build", "Debug", "QuarantineEditor.exe"),
            Path.Combine(repositoryRoot, "build", "Release", "QuarantineEditor.exe"),
            Path.Combine(repositoryRoot, "build", "RelWithDebInfo", "QuarantineEditor.exe"),
            Path.Combine(repositoryRoot, "build", "MinSizeRel", "QuarantineEditor.exe")
        };

        return candidates.FirstOrDefault(File.Exists)
            ?? throw new FileNotFoundException(
                "Could not find QuarantineEditor.exe. Build the editor first with CMake/MSBuild.");
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
}
