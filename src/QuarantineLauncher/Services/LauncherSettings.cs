namespace QuarantineLauncher.Services;

public sealed class LauncherSettings
{
    public string? EngineInstallationsRoot { get; set; }

    public string? EngineRoot { get; set; }

    public string? EngineBuildDir { get; set; }

    public string? EditorPath { get; set; }

    public string? ProjectsRoot { get; set; }

    public string? TemplatesRoot { get; set; }
}
