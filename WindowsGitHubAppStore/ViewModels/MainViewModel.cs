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
    private readonly GitHubReleaseService _releaseService = new();
    private readonly ZipDownloadService _zipDownloader = new();
    private readonly InstallerService _installer = new();
    private readonly PackageDatabase _database = new();
    private readonly AppUpdateScanner _scanner = new();
    private readonly PackageDetector _detector = new();
    private string _searchText = "windows app";
    private string _statusText = "Ready. Search GitHub apps and tools.";
    private string _tokenText = string.Empty;
    private bool _isLoading;
    private int _currentPage = 1;

    public string SearchText { get => _searchText; set { _searchText = value; OnPropertyChanged(); } }
    public string StatusText { get => _statusText; set { _statusText = value; OnPropertyChanged(); } }
    public string TokenText { get => _tokenText; set { _tokenText = value; OnPropertyChanged(); } }
    public bool IsLoading { get => _isLoading; set { _isLoading = value; OnPropertyChanged(); } }
    public int CurrentPage { get => _currentPage; set { _currentPage = value; OnPropertyChanged(); } }

    public ObservableCollection<GitHubRepository> Repositories { get; } = new();
    public ObservableCollection<GitHubRepository> FeaturedApps { get; } = new();
    public ObservableCollection<InstalledApp> InstalledApps { get; } = new();

    public ICommand SearchCommand { get; }
    public ICommand NextPageCommand { get; }
    public ICommand PreviousPageCommand { get; }
    public ICommand OpenCommand { get; }
    public ICommand DownloadCommand { get; }
    public ICommand InstallCommand { get; }
    public ICommand CopyCloneCommand { get; }
    public ICommand CategoryCommand { get; }
    public ICommand SaveTokenCommand { get; }
    public ICommand LoadInstalledCommand { get; }
    public ICommand CheckUpdatesCommand { get; }

    public MainViewModel()
    {
        SearchCommand = new RelayCommand(async _ => { CurrentPage = 1; await SearchAsync(); });
        NextPageCommand = new RelayCommand(async _ => { CurrentPage++; await SearchAsync(); });
        PreviousPageCommand = new RelayCommand(async _ => { if (CurrentPage > 1) CurrentPage--; await SearchAsync(); });
        OpenCommand = new RelayCommand(r => { if (r is GitHubRepository repo) BrowserHelper.Open(repo.HtmlUrl); });
        DownloadCommand = new RelayCommand(async r => await DownloadAsync(r));
        InstallCommand = new RelayCommand(async r => await InstallAsync(r));
        CopyCloneCommand = new RelayCommand(r => { if (r is GitHubRepository repo) { Clipboard.SetText(repo.CloneUrl); StatusText = "Copied clone URL: " + repo.CloneUrl; } });
        CategoryCommand = new RelayCommand(async q => { SearchText = q?.ToString() ?? "windows app"; CurrentPage = 1; await SearchAsync(); });
        SaveTokenCommand = new RelayCommand(_ => { _service.AccessToken = TokenText; _releaseService.AccessToken = TokenText; StatusText = "GitHub token saved for this session."; });
        LoadInstalledCommand = new RelayCommand(async _ => await LoadInstalledAsync());
        CheckUpdatesCommand = new RelayCommand(async _ => await CheckUpdatesAsync());
        _ = LoadFeaturedAsync();
        _ = LoadInstalledAsync();
    }

    private async Task SearchAsync()
    {
        try
        {
            IsLoading = true;
            StatusText = "Searching GitHub page " + CurrentPage + "...";
            Repositories.Clear();
            var repos = await _service.SearchAsync(SearchText, CurrentPage);
            foreach (var repo in repos) Repositories.Add(repo);
            StatusText = $"Page {CurrentPage}: found {Repositories.Count} repositories for '{SearchText}'.";
        }
        catch (Exception ex) { StatusText = "Search failed: " + ex.Message; }
        finally { IsLoading = false; }
    }

    private async Task LoadFeaturedAsync()
    {
        var repos = await _service.SearchAsync("stars:>5000 windows app", 1);
        FeaturedApps.Clear();
        foreach (var repo in repos.Take(8)) FeaturedApps.Add(repo);
    }

    private async Task DownloadAsync(object? arg)
    {
        if (arg is not GitHubRepository repo) return;
        StatusText = "Checking latest release for " + repo.FullName + "...";
        var release = await _releaseService.GetLatestReleaseAsync(repo.FullName);
        var url = release == null ? repo.ZipUrl : _releaseService.PickWindowsAsset(release) ?? repo.ZipUrl;
        var file = await _zipDownloader.DownloadAsync(url, repo.Name);
        StatusText = "Downloaded " + _detector.GetPackageType(file) + " package to " + file;
    }

    private async Task InstallAsync(object? arg)
    {
        if (arg is not GitHubRepository repo) return;
        var release = await _releaseService.GetLatestReleaseAsync(repo.FullName);
        var url = release == null ? repo.ZipUrl : _releaseService.PickWindowsAsset(release) ?? repo.ZipUrl;
        var version = release?.TagName ?? "zip";
        var file = await _zipDownloader.DownloadAsync(url, repo.Name);
        if (_detector.IsInstaller(file)) _installer.RunInstaller(file);
        var app = new InstalledApp { Name = repo.Name, FullName = repo.FullName, SourceUrl = repo.HtmlUrl, InstallPath = file, Version = version, InstalledAt = DateTime.Now };
        InstalledApps.Add(app);
        await _database.SaveAsync(InstalledApps.ToList());
        StatusText = "Installed library entry: " + repo.FullName + " version " + version;
    }

    private async Task LoadInstalledAsync()
    {
        InstalledApps.Clear();
        foreach (var app in await _database.LoadAsync()) InstalledApps.Add(app);
        StatusText = "Loaded installed apps library.";
    }

    private async Task CheckUpdatesAsync()
    {
        if (InstalledApps.Count == 0) { StatusText = "No installed apps to scan."; return; }
        var messages = new List<string>();
        foreach (var app in InstalledApps) messages.Add(await _scanner.ScanOneAsync(app));
        StatusText = string.Join(" | ", messages.Take(4));
    }

    public event PropertyChangedEventHandler? PropertyChanged;
    private void OnPropertyChanged([CallerMemberName] string? name = null) => PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
}
