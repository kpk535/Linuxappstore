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
    private readonly GitHubService _gitHubService = new();
    private readonly GitHubReleaseService _releaseService = new();
    private readonly ZipDownloadService _zipDownloader = new();
    private readonly InstallerService _installer = new();
    private readonly PackageDatabase _database = new();
    private readonly AppUpdateScanner _scanner = new();
    private readonly PackageDetector _detector = new();
    private readonly AppRemovalService _removal = new();
    private readonly SystemCleanerService _cleaner = new();

    private string _searchText = "windows app";
    private int _searchPage = 1;
    private string _statusText = "Ready. Search GitHub apps and tools.";
    private bool _isLoading;
    private bool _isDownloading;
    private double _downloadProgress;
    private string _downloadLabel = "Downloading...";
    private string _currentPage = "Home";
    private bool _showDetail;
    private GitHubRepository? _selectedApp;
    private string _selectedAppLatestVersion = string.Empty;
    private string _tokenText = string.Empty;
    private string _cacheSize = "—";
    private int _cacheFileCount;
    private string _cleanStatus = string.Empty;
    private int _updateCount;
    private bool _isDarkMode;

    public string SearchText { get => _searchText; set { _searchText = value; OnPropertyChanged(); } }
    public int SearchPage { get => _searchPage; set { _searchPage = value; OnPropertyChanged(); } }
    public string StatusText { get => _statusText; set { _statusText = value; OnPropertyChanged(); } }
    public bool IsLoading { get => _isLoading; set { _isLoading = value; OnPropertyChanged(); } }
    public bool IsDownloading { get => _isDownloading; set { _isDownloading = value; OnPropertyChanged(); } }
    public double DownloadProgress { get => _downloadProgress; set { _downloadProgress = value; OnPropertyChanged(); } }
    public string DownloadLabel { get => _downloadLabel; set { _downloadLabel = value; OnPropertyChanged(); } }
    public bool ShowDetail { get => _showDetail; set { _showDetail = value; OnPropertyChanged(); } }
    public GitHubRepository? SelectedApp { get => _selectedApp; set { _selectedApp = value; OnPropertyChanged(); } }
    public string SelectedAppLatestVersion { get => _selectedAppLatestVersion; set { _selectedAppLatestVersion = value; OnPropertyChanged(); } }
    public string TokenText { get => _tokenText; set { _tokenText = value; OnPropertyChanged(); } }
    public string CacheSize { get => _cacheSize; set { _cacheSize = value; OnPropertyChanged(); } }
    public int CacheFileCount { get => _cacheFileCount; set { _cacheFileCount = value; OnPropertyChanged(); } }
    public string CleanStatus { get => _cleanStatus; set { _cleanStatus = value; OnPropertyChanged(); OnPropertyChanged(nameof(HasCleanStatus)); } }
    public bool IsDarkMode { get => _isDarkMode; set { _isDarkMode = value; OnPropertyChanged(); } }

    public int UpdateCount
    {
        get => _updateCount;
        set { _updateCount = value; OnPropertyChanged(); OnPropertyChanged(nameof(HasPendingUpdates)); OnPropertyChanged(nameof(UpdateCountText)); }
    }
    public bool HasPendingUpdates => _updateCount > 0;
    public string UpdateCountText => _updateCount > 0 ? _updateCount.ToString() : string.Empty;

    public string CurrentPage
    {
        get => _currentPage;
        set
        {
            _currentPage = value;
            OnPropertyChanged();
            OnPropertyChanged(nameof(IsHomePage));
            OnPropertyChanged(nameof(IsLibraryPage));
            OnPropertyChanged(nameof(IsUpdatesPage));
            OnPropertyChanged(nameof(IsCleanerPage));
        }
    }
    public bool IsHomePage => _currentPage == "Home";
    public bool IsLibraryPage => _currentPage == "Library";
    public bool IsUpdatesPage => _currentPage == "Updates";
    public bool IsCleanerPage => _currentPage == "Cleaner";

    public ObservableCollection<GitHubRepository> Repositories { get; } = new();
    public ObservableCollection<GitHubRepository> FeaturedApps { get; } = new();
    public ObservableCollection<InstalledApp> InstalledApps { get; } = new();
    public ObservableCollection<InstalledApp> AppsWithUpdates { get; } = new();

    public bool HasNoInstalledApps => InstalledApps.Count == 0;
    public bool HasCleanStatus => !string.IsNullOrEmpty(_cleanStatus);

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
    public ICommand NavigateCommand { get; }
    public ICommand RemoveCommand { get; }
    public ICommand UpgradeCommand { get; }
    public ICommand ShowDetailCommand { get; }
    public ICommand CloseDetailCommand { get; }
    public ICommand CleanCacheCommand { get; }
    public ICommand UpdateAllCommand { get; }
    public ICommand ToggleThemeCommand { get; }

    public MainViewModel()
    {
        SearchCommand = new RelayCommand(async _ => { SearchPage = 1; await SearchAsync(); });
        NextPageCommand = new RelayCommand(async _ => { SearchPage++; await SearchAsync(); });
        PreviousPageCommand = new RelayCommand(async _ => { if (SearchPage > 1) { SearchPage--; await SearchAsync(); } });
        OpenCommand = new RelayCommand(r =>
        {
            var url = r switch
            {
                GitHubRepository repo => repo.HtmlUrl,
                InstalledApp app => app.SourceUrl,
                string s => s,
                _ => null
            };
            if (!string.IsNullOrEmpty(url)) BrowserHelper.Open(url);
        });
        DownloadCommand = new RelayCommand(async r => await DownloadAsync(r));
        InstallCommand = new RelayCommand(async r => await InstallAsync(r));
        CopyCloneCommand = new RelayCommand(r =>
        {
            if (r is GitHubRepository repo) { Clipboard.SetText(repo.CloneUrl); StatusText = "Copied clone URL: " + repo.CloneUrl; }
        });
        CategoryCommand = new RelayCommand(async q =>
        {
            SearchText = q?.ToString() ?? "windows app";
            SearchPage = 1;
            CurrentPage = "Home";
            await SearchAsync();
        });
        SaveTokenCommand = new RelayCommand(_ =>
        {
            _gitHubService.AccessToken = TokenText;
            _releaseService.AccessToken = TokenText;
            _scanner.AccessToken = TokenText;
            StatusText = "GitHub token saved.";
        });
        LoadInstalledCommand = new RelayCommand(async _ => await LoadInstalledAsync());
        CheckUpdatesCommand = new RelayCommand(async _ => await CheckUpdatesAsync());
        NavigateCommand = new RelayCommand(p =>
        {
            CurrentPage = p?.ToString() ?? "Home";
            if (CurrentPage == "Cleaner") LoadCleanerInfo();
            if (CurrentPage == "Library") _ = LoadInstalledAsync();
            if (CurrentPage == "Updates") _ = CheckUpdatesAsync();
        });
        RemoveCommand = new RelayCommand(async r => await RemoveAsync(r));
        UpgradeCommand = new RelayCommand(async r => await UpgradeAsync(r));
        ShowDetailCommand = new RelayCommand(async r => await ShowDetailAsync(r));
        CloseDetailCommand = new RelayCommand(_ => ShowDetail = false);
        CleanCacheCommand = new RelayCommand(_ => CleanCache());
        UpdateAllCommand = new RelayCommand(async _ => await UpdateAllAsync());
        ToggleThemeCommand = new RelayCommand(_ => ToggleTheme());

        InstalledApps.CollectionChanged += (_, __) => OnPropertyChanged(nameof(HasNoInstalledApps));

        _ = LoadFeaturedAsync();
        _ = LoadInstalledAsync();
        _ = SearchAsync();
    }

    private async Task SearchAsync()
    {
        try
        {
            IsLoading = true;
            StatusText = $"Searching GitHub page {SearchPage}...";
            Repositories.Clear();
            var repos = await _gitHubService.SearchAsync(SearchText, SearchPage);
            foreach (var repo in repos) Repositories.Add(repo);
            StatusText = $"Page {SearchPage} — {Repositories.Count} results for \"{SearchText}\"";
        }
        catch (Exception ex) { StatusText = "Search failed: " + ex.Message; }
        finally { IsLoading = false; }
    }

    private async Task LoadFeaturedAsync()
    {
        try
        {
            var repos = await _gitHubService.SearchAsync("stars:>5000 windows app", 1);
            FeaturedApps.Clear();
            foreach (var repo in repos.Take(8)) FeaturedApps.Add(repo);
        }
        catch { }
    }

    private async Task DownloadAsync(object? arg)
    {
        if (arg is not GitHubRepository repo) return;
        StatusText = "Fetching release for " + repo.FullName + "...";
        var release = await _releaseService.GetLatestReleaseAsync(repo.FullName);
        var url = release == null ? repo.ZipUrl : _releaseService.PickWindowsAsset(release) ?? repo.ZipUrl;
        IsDownloading = true;
        DownloadProgress = 0;
        DownloadLabel = $"Downloading {repo.Name}...";
        try
        {
            var file = await _zipDownloader.DownloadAsync(url, repo.Name, new Progress<double>(p =>
            {
                DownloadProgress = p;
                DownloadLabel = $"Downloading {repo.Name} — {p:F0}%";
            }));
            StatusText = $"Downloaded: {file}";
        }
        catch (Exception ex) { StatusText = "Download failed: " + ex.Message; }
        finally { IsDownloading = false; }
    }

    private async Task InstallAsync(object? arg)
    {
        if (arg is not GitHubRepository repo) return;
        StatusText = "Fetching release for " + repo.FullName + "...";
        var release = await _releaseService.GetLatestReleaseAsync(repo.FullName);
        var url = release == null ? repo.ZipUrl : _releaseService.PickWindowsAsset(release) ?? repo.ZipUrl;
        var version = release?.TagName ?? "zip";

        IsDownloading = true;
        DownloadProgress = 0;
        DownloadLabel = $"Installing {repo.Name}...";
        string file;
        try
        {
            file = await _zipDownloader.DownloadAsync(url, repo.Name, new Progress<double>(p =>
            {
                DownloadProgress = p;
                DownloadLabel = $"Installing {repo.Name} — {p:F0}%";
            }));
        }
        catch (Exception ex) { StatusText = "Install failed: " + ex.Message; IsDownloading = false; return; }
        finally { IsDownloading = false; }

        if (_detector.IsInstaller(file)) _installer.RunInstaller(file);

        var existing = InstalledApps.FirstOrDefault(a => a.FullName == repo.FullName);
        if (existing != null)
        {
            existing.Version = version;
            existing.InstallPath = file;
            existing.InstalledAt = DateTime.Now;
            existing.HasUpdate = false;
        }
        else
        {
            InstalledApps.Add(new InstalledApp
            {
                Name = repo.Name,
                FullName = repo.FullName,
                SourceUrl = repo.HtmlUrl,
                InstallPath = file,
                Version = version,
                InstalledAt = DateTime.Now,
                AvatarUrl = repo.Owner.AvatarUrl,
                Description = repo.Description ?? string.Empty
            });
        }
        await _database.SaveAsync(InstalledApps.ToList());
        StatusText = $"Installed {repo.FullName} {version}";
    }

    private async Task LoadInstalledAsync()
    {
        InstalledApps.Clear();
        foreach (var app in await _database.LoadAsync()) InstalledApps.Add(app);
        StatusText = $"Library: {InstalledApps.Count} installed app(s).";
    }

    private async Task CheckUpdatesAsync()
    {
        if (InstalledApps.Count == 0) { StatusText = "No installed apps to scan."; return; }
        StatusText = "Scanning for updates...";
        AppsWithUpdates.Clear();
        foreach (var app in InstalledApps.ToList())
        {
            await _scanner.ScanOneAsync(app);
            if (app.HasUpdate && !AppsWithUpdates.Contains(app)) AppsWithUpdates.Add(app);
        }
        UpdateCount = AppsWithUpdates.Count;
        StatusText = UpdateCount == 0 ? "All apps are up to date." : $"{UpdateCount} update(s) available.";
    }

    private async Task RemoveAsync(object? arg)
    {
        if (arg is not InstalledApp app) return;
        _removal.Remove(app);
        InstalledApps.Remove(app);
        var upd = AppsWithUpdates.FirstOrDefault(a => a.FullName == app.FullName);
        if (upd != null) { AppsWithUpdates.Remove(upd); UpdateCount = AppsWithUpdates.Count; }
        await _database.SaveAsync(InstalledApps.ToList());
        StatusText = $"Removed {app.FullName} from library.";
    }

    private async Task UpgradeAsync(object? arg)
    {
        if (arg is not InstalledApp app) return;
        StatusText = $"Upgrading {app.FullName}...";
        var release = await _releaseService.GetLatestReleaseAsync(app.FullName);
        if (release == null) { StatusText = "No release found for " + app.FullName; return; }
        var url = _releaseService.PickWindowsAsset(release)
                  ?? $"https://github.com/{app.FullName}/archive/refs/heads/main.zip";

        IsDownloading = true;
        DownloadProgress = 0;
        DownloadLabel = $"Upgrading {app.Name} to {release.TagName}...";
        string file;
        try
        {
            file = await _zipDownloader.DownloadAsync(url, app.Name, new Progress<double>(p =>
            {
                DownloadProgress = p;
                DownloadLabel = $"Upgrading {app.Name} — {p:F0}%";
            }));
        }
        catch (Exception ex) { StatusText = "Upgrade failed: " + ex.Message; IsDownloading = false; return; }
        finally { IsDownloading = false; }

        if (_detector.IsInstaller(file)) _installer.RunInstaller(file);
        app.Version = release.TagName;
        app.LatestVersion = release.TagName;
        app.InstallPath = file;
        app.HasUpdate = false;
        var upd = AppsWithUpdates.FirstOrDefault(a => a.FullName == app.FullName);
        if (upd != null) { AppsWithUpdates.Remove(upd); UpdateCount = AppsWithUpdates.Count; }
        await _database.SaveAsync(InstalledApps.ToList());
        StatusText = $"Upgraded {app.FullName} to {release.TagName}";
    }

    private async Task ShowDetailAsync(object? arg)
    {
        if (arg is not GitHubRepository repo) return;
        SelectedApp = repo;
        ShowDetail = true;
        SelectedAppLatestVersion = "Checking...";
        var release = await _releaseService.GetLatestReleaseAsync(repo.FullName);
        SelectedAppLatestVersion = release?.TagName ?? "No release";
    }

    private void LoadCleanerInfo()
    {
        CacheSize = _cleaner.FormatSize(_cleaner.GetCacheSize());
        CacheFileCount = _cleaner.GetCacheFileCount();
        CleanStatus = string.Empty;
    }

    private void CleanCache()
    {
        _cleaner.ClearCache();
        CacheSize = "0 B";
        CacheFileCount = 0;
        CleanStatus = "Cache cleared.";
        StatusText = "Download cache cleared.";
    }

    private async Task UpdateAllAsync()
    {
        var toUpdate = AppsWithUpdates.ToList();
        foreach (var app in toUpdate) await UpgradeAsync(app);
        StatusText = $"Updated {toUpdate.Count} app(s).";
    }

    private void ToggleTheme()
    {
        IsDarkMode = !IsDarkMode;
        var uri = new Uri(IsDarkMode ? "Themes/DarkTheme.xaml" : "Themes/StoreTheme.xaml", UriKind.Relative);
        var dict = new ResourceDictionary { Source = uri };
        Application.Current.Resources.MergedDictionaries.Clear();
        Application.Current.Resources.MergedDictionaries.Add(dict);
    }

    public event PropertyChangedEventHandler? PropertyChanged;
    private void OnPropertyChanged([CallerMemberName] string? name = null) =>
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
}
