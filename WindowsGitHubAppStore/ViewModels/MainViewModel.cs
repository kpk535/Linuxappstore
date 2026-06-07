using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Windows.Input;
using WindowsGitHubAppStore.Commands;
using WindowsGitHubAppStore.Helpers;
using WindowsGitHubAppStore.Models;
using WindowsGitHubAppStore.Services;

namespace WindowsGitHubAppStore.ViewModels;

public class MainViewModel : INotifyPropertyChanged
{
    private readonly GitHubService _service = new();
    private string _searchText = "video player";

    public string SearchText
    {
        get => _searchText;
        set { _searchText = value; OnPropertyChanged(); }
    }

    public ObservableCollection<GitHubRepository> Repositories { get; } = new();

    public ICommand SearchCommand { get; }
    public ICommand OpenCommand { get; }

    public MainViewModel()
    {
        SearchCommand = new RelayCommand(async _ => await SearchAsync());
        OpenCommand = new RelayCommand(r =>
        {
            if (r is GitHubRepository repo)
                BrowserHelper.Open(repo.HtmlUrl);
        });
    }

    private async Task SearchAsync()
    {
        Repositories.Clear();
        var repos = await _service.SearchAsync(SearchText);
        foreach (var repo in repos)
            Repositories.Add(repo);
    }

    public event PropertyChangedEventHandler? PropertyChanged;
    private void OnPropertyChanged([CallerMemberName] string? name = null)
        => PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
}
