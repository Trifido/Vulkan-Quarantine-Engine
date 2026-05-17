using System.IO;
using System.Text.Json;

namespace QuarantineLauncher.Services;

public sealed class LauncherSettingsStore
{
    public LauncherSettings Load()
    {
        var settingsPath = WorkspaceLocator.GetSettingsPath();
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

    public void Save(LauncherSettings settings)
    {
        var settingsPath = WorkspaceLocator.GetSettingsPath();
        var json = JsonSerializer.Serialize(settings, new JsonSerializerOptions
        {
            WriteIndented = true
        });

        File.WriteAllText(settingsPath, json);
        WorkspaceLocator.Reload();
    }
}
