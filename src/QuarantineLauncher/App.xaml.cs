using System.IO;
using System.Text;
using System.Windows;

namespace QuarantineLauncher;

public partial class App : Application
{
    private static readonly string LauncherLogDirectory = Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
        "QuarantineEngine",
        "Logs");

    private static readonly string LauncherLogPath = Path.Combine(LauncherLogDirectory, "launcher.log");

    protected override void OnStartup(StartupEventArgs e)
    {
        Directory.CreateDirectory(LauncherLogDirectory);
        Log("Launcher starting.");

        DispatcherUnhandledException += OnDispatcherUnhandledException;
        AppDomain.CurrentDomain.UnhandledException += OnCurrentDomainUnhandledException;

        try
        {
            base.OnStartup(e);
            Log("Launcher started successfully.");
        }
        catch (Exception ex)
        {
            LogException("Startup failure", ex);
            MessageBox.Show(
                $"Quarantine Engine failed to start.\n\n{ex.Message}\n\nLog: {LauncherLogPath}",
                "Quarantine Engine",
                MessageBoxButton.OK,
                MessageBoxImage.Error);
            throw;
        }
    }

    private void OnDispatcherUnhandledException(object sender, System.Windows.Threading.DispatcherUnhandledExceptionEventArgs e)
    {
        LogException("Dispatcher unhandled exception", e.Exception);
        MessageBox.Show(
            $"Quarantine Engine crashed.\n\n{e.Exception.Message}\n\nLog: {LauncherLogPath}",
            "Quarantine Engine",
            MessageBoxButton.OK,
            MessageBoxImage.Error);
    }

    private void OnCurrentDomainUnhandledException(object? sender, UnhandledExceptionEventArgs e)
    {
        if (e.ExceptionObject is Exception exception)
        {
            LogException("AppDomain unhandled exception", exception);
        }
        else
        {
            Log($"AppDomain unhandled exception: {e.ExceptionObject}");
        }
    }

    private static void Log(string message)
    {
        File.AppendAllText(
            LauncherLogPath,
            $"[{DateTime.Now:yyyy-MM-dd HH:mm:ss}] {message}{Environment.NewLine}",
            Encoding.UTF8);
    }

    private static void LogException(string title, Exception exception)
    {
        Log($"{title}: {exception}");
    }
}
