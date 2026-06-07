using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Windows;
using System.Windows.Input;
using WindowsGitHubAppStore.Commands;
using WindowsGitHubAppStore.Helpers;
using WindowsGitHubAppStore.Models;
using WindowsGitHubAppStore.Services;

namespace WindowsGitHubAppStore.ViewModels;

public class MainViewModel : INotifyPropertyChanged
{
    private readonly GitHubService _service = new();
    private string _searchText = "windows app";
    private string _statusText = "Ready. Search GitHub apps and tools.";
    private bool _isLoading;

    public string SearchText
    {
        get => _searchText;
        set { _searchText = value; OnPropertyChanged(); }
    }

    public string StatusText
    {
        get => _statusText;
        set { _statusText = value; OnPropertyChanged(); }
    }

    public bool IsLoading
    {
        get => _isLoading;
        set { _isLoading = value; OnPropertyChanged(); }
    }

    public ObservableCollection<GitHubRepository> Repositories { get; } = new();

    public ICommand SearchCommand { get; }
    public ICommand OpenCommand { get; }
    public ICommand DownloadCommand { get; }
    public ICommand CopyCloneCommand { get; }
    public ICommand CategoryCommand { get; }

    public MainViewModel()
    {
        SearchCommand = new RelayCommand(async _ => await SearchAsync());
        OpenCommand = new RelayCommand(r => { if (r is GitHubRepository repo) BrowserHelper.Open(repo.HtmlUrl); });
        DownloadCommand = new RelayCommand(r => { if (r is GitHubRepository repo) BrowserHelper.Open(repo.ZipUrl); });
        CopyCloneCommand = new RelayCommand(r =>
        {
            if (r is GitHubRepository repo)
            {
                Clipboard.SetText(repo.CloneUrl);
                StatusText = "Copied clone URL: " + repo.CloneUrl;
            }
        });
        CategoryCommand = new RelayCommand(async q =>
        {
            SearchText = q?.ToString() ?? "windows app";
            await SearchAsync();
        });
    }

    private async Task SearchAsync()
    {
        try
        {
            IsLoading = true;
            StatusText = "Searching GitHub...";
            Repositories.Clear();
            var repos = await _service.SearchAsync(SearchText);
            foreach (var repo in repos) Repositories.Add(repo);
            StatusText = $"Found {Repositories.Count} repositories for '{SearchText}'.";
        }
        catch (Exception ex)
        {
            StatusText = "Search failed: " + ex.Message;
        }
        finally
        {
            IsLoading = false;
        }
    }

    public event PropertyChangedEventHandler? PropertyChanged;
    private void OnPropertyChanged([CallerMemberName] string? name = null)
        => PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
}
