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
    private readonly EditorLaunchService _editorLaunchService = new();
    private ProjectEntry? _selectedProject;
    private string _statusMessage = "Listo.";

    public MainWindow()
    {
        InitializeComponent();
        DataContext = this;
        ProjectsRootDisplay = $"Workspace projects: {_projectRepository.ProjectsRoot}";
        RefreshProjects();
    }

    public ObservableCollection<ProjectEntry> Projects { get; } = new();

    public string ProjectsRootDisplay { get; }

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
            var project = _projectRepository.CreateProject(ProjectNameTextBox.Text);
            RefreshProjects();
            SelectedProject = Projects.FirstOrDefault(item =>
                string.Equals(item.Name, project.Name, StringComparison.OrdinalIgnoreCase));
            ProjectNameTextBox.Clear();
            StatusMessage = $"Project '{project.Name}' was created successfully.";
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
