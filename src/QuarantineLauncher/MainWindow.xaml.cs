using System.Collections.ObjectModel;
using System.ComponentModel;
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
    private readonly EditorLaunchService _editorLaunchService = new();
    private ProjectEntry? _selectedProject;
    private EngineInstallation? _selectedEngineInstallation;
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

    public string ProjectsRootDisplay { get; }

    public string SelectedEngineDisplay => SelectedEngineInstallation is null
        ? "No engine selected."
        : $"Engine: {SelectedEngineInstallation.DisplayName}";

    public string EngineRootDisplay => SelectedEngineInstallation is null
        ? "Engine root: -"
        : $"Engine root: {SelectedEngineInstallation.RootPath}";

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
        EngineInstallations.Clear();
        foreach (var installation in _engineInstallationService.GetAvailableInstallations())
        {
            EngineInstallations.Add(installation);
        }

        SelectedEngineInstallation = EngineInstallations.FirstOrDefault();
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

    private void ClearProjectName_Click(object sender, RoutedEventArgs e)
    {
        ProjectNameTextBox.Clear();
        Keyboard.Focus(ProjectNameTextBox);
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

    private void ShowError(string message)
    {
        StatusMessage = message;
        MessageBox.Show(this, message, "Quarantine Engine", MessageBoxButton.OK, MessageBoxImage.Error);
    }

    private void OnPropertyChanged([CallerMemberName] string? propertyName = null)
    {
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
    }
}
