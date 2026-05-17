using System.Collections.ObjectModel;
using System.ComponentModel;
using System.IO;
using System.Runtime.CompilerServices;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using QuarantineLauncher.Models;
using QuarantineLauncher.Services;

namespace QuarantineLauncher;

public partial class MainWindow : Window, INotifyPropertyChanged
{
    private readonly ProjectRepository _projectRepository = new();
    private readonly EngineInstallationService _engineInstallationService = new();
    private readonly EnginePackageImportService _enginePackageImportService = new();
    private readonly EditorLaunchService _editorLaunchService = new();
    private ProjectEntry? _selectedProject;
    private EngineInstallation? _selectedEngineInstallation;
    private EngineInstallation? _selectedFeedEngine;
    private EngineInstallation? _selectedInstalledEngine;
    private bool _isInstallVersionPanelOpen;
    private bool _isUninstallVersionPanelOpen;
    private string _statusMessage = "Listo.";

    public MainWindow()
    {
        InitializeComponent();
        DataContext = this;
        ProjectsRootDisplay = $"Workspace projects: {_projectRepository.ProjectsRoot}";
        LoadEngineInstallations();
        RefreshProjects();
    }

    public ObservableCollection<ProjectEntry> Projects { get; } = new();
    public ObservableCollection<EngineInstallation> EngineInstallations { get; } = new();
    public ObservableCollection<EngineInstallation> ProjectEngines { get; } = new();
    public ObservableCollection<EngineInstallation> FeedEngines { get; } = new();
    public ObservableCollection<EngineInstallation> InstalledManagedEngines { get; } = new();

    public string ProjectsRootDisplay { get; }

    public string SelectedEngineDisplay => SelectedEngineInstallation is null
        ? "No engine selected."
        : $"Engine: {SelectedEngineInstallation.DisplayName} ({SelectedEngineInstallation.StatusDisplay})";

    public string EngineRootDisplay => SelectedEngineInstallation is null
        ? "Engine root: -"
        : $"Engine root: {SelectedEngineInstallation.RootPath}";

    public string EngineInstallationsRootDisplay => $"Installed engines: {WorkspaceLocator.FindEngineInstallationsRoot() ?? "-"}";
    public bool CanInstallSelectedEngine => SelectedFeedEngine is { ManifestPath: not null } or { PackageSource: not null };
    public bool CanUninstallSelectedEngine => SelectedInstalledEngine is { IsManagedInstallation: true, IsDefault: false };
    public string InstallSelectedEngineButtonLabel => SelectedFeedEngine?.IsInstalledInLauncher == true
        ? "Reinstall Selected"
        : "Install Selected";
    public string FeedVersionsSummary => FeedEngines.Count == 0
        ? "No published versions were found in the configured feed."
        : $"{FeedEngines.Count} published version(s) available in the feed.";
    public string InstalledVersionsSummary => InstalledManagedEngines.Count == 0
        ? "No launcher-managed engine versions are currently installed."
        : $"{InstalledManagedEngines.Count} installed version(s) managed by the launcher.";

    public bool IsInstallVersionPanelOpen
    {
        get => _isInstallVersionPanelOpen;
        set
        {
            if (_isInstallVersionPanelOpen == value)
                return;

            _isInstallVersionPanelOpen = value;
            OnPropertyChanged();
        }
    }

    public bool IsUninstallVersionPanelOpen
    {
        get => _isUninstallVersionPanelOpen;
        set
        {
            if (_isUninstallVersionPanelOpen == value)
                return;

            _isUninstallVersionPanelOpen = value;
            OnPropertyChanged();
        }
    }

    public EngineInstallation? SelectedEngineInstallation
    {
        get => _selectedEngineInstallation;
        set
        {
            if (ReferenceEquals(_selectedEngineInstallation, value))
                return;

            _selectedEngineInstallation = value;
            OnPropertyChanged();
            OnPropertyChanged(nameof(SelectedEngineDisplay));
            OnPropertyChanged(nameof(EngineRootDisplay));
            OnPropertyChanged(nameof(EngineInstallationsRootDisplay));
            OnPropertyChanged(nameof(CanInstallSelectedEngine));
            OnPropertyChanged(nameof(CanUninstallSelectedEngine));
        }
    }

    public EngineInstallation? SelectedFeedEngine
    {
        get => _selectedFeedEngine;
        set
        {
            if (ReferenceEquals(_selectedFeedEngine, value))
                return;

            _selectedFeedEngine = value;
            OnPropertyChanged();
            OnPropertyChanged(nameof(CanInstallSelectedEngine));
            OnPropertyChanged(nameof(InstallSelectedEngineButtonLabel));
        }
    }

    public EngineInstallation? SelectedInstalledEngine
    {
        get => _selectedInstalledEngine;
        set
        {
            if (ReferenceEquals(_selectedInstalledEngine, value))
                return;

            _selectedInstalledEngine = value;
            OnPropertyChanged();
            OnPropertyChanged(nameof(CanUninstallSelectedEngine));
        }
    }

    public ProjectEntry? SelectedProject
    {
        get => _selectedProject;
        set
        {
            if (ReferenceEquals(_selectedProject, value))
                return;

            _selectedProject = value;
            OnPropertyChanged();
            UpdateActionButtons();
        }
    }

    public string StatusMessage
    {
        get => _statusMessage;
        set
        {
            if (_statusMessage == value)
                return;

            _statusMessage = value;
            OnPropertyChanged();
        }
    }

    public event PropertyChangedEventHandler? PropertyChanged;

    private void LoadEngineInstallations()
    {
        var previousRootPath = SelectedEngineInstallation?.RootPath;
        var previousFeedRootPath = SelectedFeedEngine?.RootPath;
        var previousInstalledRootPath = SelectedInstalledEngine?.RootPath;

        EngineInstallations.Clear();
        ProjectEngines.Clear();
        FeedEngines.Clear();
        InstalledManagedEngines.Clear();

        var installations = _engineInstallationService.GetAvailableInstallations().ToList();
        var installedVersionKeys = installations
            .Where(item => item.IsManagedInstallation)
            .Select(GetVersionKey)
            .ToHashSet(StringComparer.OrdinalIgnoreCase);

        foreach (var installation in installations)
        {
            EngineInstallations.Add(installation);

            if (!installation.IsInstalledPackage || installation.IsManagedInstallation)
            {
                ProjectEngines.Add(installation);
            }

            if (installation.IsInstalledPackage && !installation.IsManagedInstallation)
            {
                FeedEngines.Add(WithInstalledFlag(installation, installedVersionKeys.Contains(GetVersionKey(installation))));
            }

            if (installation.IsManagedInstallation)
            {
                InstalledManagedEngines.Add(installation);
            }
        }

        SelectedEngineInstallation = ProjectEngines.FirstOrDefault(item =>
            string.Equals(item.RootPath, previousRootPath, StringComparison.OrdinalIgnoreCase))
            ?? ProjectEngines.FirstOrDefault(item => item.IsDefault)
            ?? ProjectEngines.FirstOrDefault();

        SelectedFeedEngine = FeedEngines.FirstOrDefault(item =>
            string.Equals(item.RootPath, previousFeedRootPath, StringComparison.OrdinalIgnoreCase))
            ?? FeedEngines.FirstOrDefault();

        SelectedInstalledEngine = InstalledManagedEngines.FirstOrDefault(item =>
            string.Equals(item.RootPath, previousInstalledRootPath, StringComparison.OrdinalIgnoreCase))
            ?? InstalledManagedEngines.FirstOrDefault();

        OnPropertyChanged(nameof(FeedVersionsSummary));
        OnPropertyChanged(nameof(InstalledVersionsSummary));
        OnPropertyChanged(nameof(InstallSelectedEngineButtonLabel));
        OnPropertyChanged(nameof(CanInstallSelectedEngine));
        OnPropertyChanged(nameof(CanUninstallSelectedEngine));
    }

    private void RefreshProjects()
    {
        var previousSelection = SelectedProject?.Name;
        Projects.Clear();

        foreach (var project in _projectRepository.GetProjects())
        {
            Projects.Add(project);
        }

        SelectedProject = Projects.FirstOrDefault(project =>
            string.Equals(project.Name, previousSelection, StringComparison.OrdinalIgnoreCase))
            ?? Projects.FirstOrDefault();

        StatusMessage = Projects.Count == 0
            ? "No projects yet. Create your first one from the panel on the right."
            : $"Loaded {Projects.Count} project(s).";
    }

    private void UpdateActionButtons()
    {
        var hasSelection = SelectedProject is not null;
        OpenProjectButton.IsEnabled = hasSelection;
        DeleteProjectButton.IsEnabled = hasSelection;
        ToolbarOpenProjectButton.IsEnabled = hasSelection;
        ToolbarDeleteProjectButton.IsEnabled = hasSelection;
    }

    private void CreateProject_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            if (SelectedEngineInstallation is null)
            {
                throw new InvalidOperationException("Select an engine installation before creating a project.");
            }

            var project = _projectRepository.CreateProject(
                ProjectNameTextBox.Text,
                SelectedEngineInstallation.EnginePaths,
                SelectedEngineInstallation);
            RefreshProjects();
            SelectedProject = Projects.FirstOrDefault(item =>
                string.Equals(item.Name, project.Name, StringComparison.OrdinalIgnoreCase));
            ProjectNameTextBox.Clear();
            StatusMessage = $"Project '{project.Name}' was created with {SelectedEngineInstallation.DisplayName}.";
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private void DeleteProject_Click(object sender, RoutedEventArgs e)
    {
        if (SelectedProject is null)
            return;

        var result = MessageBox.Show(
            $"Project '{SelectedProject.Name}' and all its contents will be deleted.\n\nDo you want to continue?",
            "Delete Project",
            MessageBoxButton.YesNo,
            MessageBoxImage.Warning);

        if (result != MessageBoxResult.Yes)
            return;

        try
        {
            var deletedProjectName = SelectedProject.Name;
            _projectRepository.DeleteProject(deletedProjectName);
            RefreshProjects();
            StatusMessage = $"Project '{deletedProjectName}' was deleted successfully.";
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private void OpenProject_Click(object sender, RoutedEventArgs e)
    {
        OpenSelectedProject();
    }

    private void OpenSelectedProject()
    {
        if (SelectedProject is null)
            return;

        try
        {
            _editorLaunchService.LaunchEditor(SelectedProject.FullPath);
            StatusMessage = $"Opening QuarantineEditor with '{SelectedProject.Name}'.";
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private void RefreshProjects_Click(object sender, RoutedEventArgs e)
    {
        LoadEngineInstallations();
        RefreshProjects();
    }

    private void OpenInstallVersionPanel_Click(object sender, RoutedEventArgs e)
    {
        IsUninstallVersionPanelOpen = false;
        IsInstallVersionPanelOpen = true;
    }

    private void InstallSelectedEngine_Click(object sender, RoutedEventArgs e)
    {
        if (SelectedFeedEngine is not { ManifestPath: not null } && SelectedFeedEngine?.PackageSource is null)
        {
            ShowError("Select a package version from the feed to install it.");
            return;
        }

        var selectedEngineName = SelectedFeedEngine.DisplayName;
        var selectedFeedEngine = SelectedFeedEngine;

        try
        {
            _enginePackageImportService.InstallFromFeedPackage(selectedFeedEngine);
            LoadEngineInstallations();
            IsInstallVersionPanelOpen = false;
            StatusMessage = $"Installed engine package '{selectedEngineName}'.";
        }
        catch (InvalidOperationException ex) when (ex.Message.Contains("already installed", StringComparison.OrdinalIgnoreCase))
        {
            var result = MessageBox.Show(
                this,
                $"{ex.Message}\n\nDo you want to overwrite the existing installation?",
                "Overwrite Engine Installation",
                MessageBoxButton.YesNo,
                MessageBoxImage.Question);

            if (result != MessageBoxResult.Yes)
            {
                return;
            }

            _enginePackageImportService.InstallFromFeedPackage(selectedFeedEngine, overwriteExisting: true);
            LoadEngineInstallations();
            IsInstallVersionPanelOpen = false;
            StatusMessage = $"Reinstalled engine package '{selectedEngineName}'.";
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private void ProjectsListBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        SelectedProject = ProjectsListBox.SelectedItem as ProjectEntry;
    }

    private void ProjectsListBox_MouseDoubleClick(object sender, MouseButtonEventArgs e)
    {
        if (SelectedProject is not null)
        {
            OpenSelectedProject();
        }
    }

    private void EngineInstallationsComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        SelectedEngineInstallation = EngineInstallationsComboBox.SelectedItem as EngineInstallation;
    }

    private void FeedEnginesListBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        SelectedFeedEngine = FeedEnginesListBox.SelectedItem as EngineInstallation;
    }

    private void InstalledEnginesListBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        SelectedInstalledEngine = InstalledEnginesListBox.SelectedItem as EngineInstallation;
    }

    private void OpenUninstallVersionPanel_Click(object sender, RoutedEventArgs e)
    {
        IsInstallVersionPanelOpen = false;
        IsUninstallVersionPanelOpen = true;
    }

    private void UninstallSelectedEngine_Click(object sender, RoutedEventArgs e)
    {
        if (SelectedInstalledEngine is null)
        {
            return;
        }

        if (!SelectedInstalledEngine.IsManagedInstallation)
        {
            ShowError("Only installed engines can be uninstalled from the launcher.");
            return;
        }

        if (SelectedInstalledEngine.IsDefault)
        {
            ShowError("Set another engine as default before uninstalling this installation.");
            return;
        }

        var result = MessageBox.Show(
            this,
            $"Engine '{SelectedInstalledEngine.DisplayName}' will be removed from the installed engines folder.\n\nDo you want to continue?",
            "Uninstall Engine",
            MessageBoxButton.YesNo,
            MessageBoxImage.Warning);

        if (result != MessageBoxResult.Yes)
        {
            return;
        }

        try
        {
            var removedEngineName = SelectedInstalledEngine.DisplayName;
            _enginePackageImportService.Uninstall(SelectedInstalledEngine);
            LoadEngineInstallations();
            IsUninstallVersionPanelOpen = false;
            StatusMessage = $"Engine '{removedEngineName}' was uninstalled successfully.";
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private void CloseInstallVersionPanel_Click(object sender, RoutedEventArgs e)
    {
        IsInstallVersionPanelOpen = false;
    }

    private void CloseUninstallVersionPanel_Click(object sender, RoutedEventArgs e)
    {
        IsUninstallVersionPanelOpen = false;
    }

    private void ShowError(string message)
    {
        StatusMessage = message;
        MessageBox.Show(this, message, "Quarantine Engine", MessageBoxButton.OK, MessageBoxImage.Error);
    }

    private static string GetVersionKey(EngineInstallation installation)
    {
        return $"{installation.Version}|{installation.Platform}";
    }

    private static EngineInstallation WithInstalledFlag(EngineInstallation installation, bool isInstalledInLauncher)
    {
        if (installation.IsInstalledInLauncher == isInstalledInLauncher)
        {
            return installation;
        }

        return new EngineInstallation
        {
            DisplayName = installation.DisplayName,
            ManifestPath = installation.ManifestPath,
            PackageSource = installation.PackageSource,
            Version = installation.Version,
            Platform = installation.Platform,
            RootPath = installation.RootPath,
            IsInstalledPackage = installation.IsInstalledPackage,
            IsManagedInstallation = installation.IsManagedInstallation,
            IsDefault = installation.IsDefault,
            IsInstalledInLauncher = isInstalledInLauncher,
            Configurations = installation.Configurations,
            EnginePaths = installation.EnginePaths
        };
    }

    private void OnPropertyChanged([CallerMemberName] string? propertyName = null)
    {
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
    }
}
