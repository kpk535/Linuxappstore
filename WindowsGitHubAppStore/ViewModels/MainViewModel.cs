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
    private readonly WingetService _winget = new();
    private readonly WindowsAppsService _windowsAppsService = new();
    private readonly StartupService _startupService = new();
    private readonly SettingsService _settingsService = new();
    private readonly UserProfileService _profileService = new();
    private readonly SystemInfoService _sysInfoService = new();

    private AppSettings _settings;

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
    private string _cleanStatus = string.Empty;
    private int _updateCount;
    private bool _isDarkMode;

    private string _wingetQuery = string.Empty;
    private string _wingetStatus = string.Empty;
    private bool _isWingetLoading;

    private List<RegisteredApp> _allWindowsApps = new();
    private string _windowsAppsFilter = string.Empty;
    private string _windowsAppsStatus = string.Empty;

    private string _startupStatus = string.Empty;

    private UserProfile _profile = new();
    private SystemSnapshot? _sysInfo;
    private bool _isProfileLoading;
    private bool _isSysInfoLoading;
    private string _settingsStatus = string.Empty;

    // ── Basic properties ────────────────────────────────────────────────────

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
    public string CleanStatus { get => _cleanStatus; set { _cleanStatus = value; OnPropertyChanged(); OnPropertyChanged(nameof(HasCleanStatus)); } }
    public bool IsDarkMode { get => _isDarkMode; set { _isDarkMode = value; OnPropertyChanged(); } }

    public string WingetQuery { get => _wingetQuery; set { _wingetQuery = value; OnPropertyChanged(); } }
    public string WingetStatus { get => _wingetStatus; set { _wingetStatus = value; OnPropertyChanged(); OnPropertyChanged(nameof(HasWingetStatus)); } }
    public bool IsWingetLoading { get => _isWingetLoading; set { _isWingetLoading = value; OnPropertyChanged(); } }
    public bool WingetAvailable => _winget.IsAvailable;
    public bool WingetNotAvailable => !_winget.IsAvailable;
    public bool HasWingetStatus => !string.IsNullOrEmpty(_wingetStatus);

    public string WindowsAppsFilter
    {
        get => _windowsAppsFilter;
        set { _windowsAppsFilter = value; OnPropertyChanged(); ApplyWindowsAppsFilter(); }
    }
    public string WindowsAppsStatus { get => _windowsAppsStatus; set { _windowsAppsStatus = value; OnPropertyChanged(); } }
    public string StartupStatus { get => _startupStatus; set { _startupStatus = value; OnPropertyChanged(); } }

    // ── Profile / SysInfo ───────────────────────────────────────────────────

    public UserProfile Profile
    {
        get => _profile;
        set { _profile = value; OnPropertyChanged(); }
    }

    public SystemSnapshot? SysInfo
    {
        get => _sysInfo;
        set
        {
            _sysInfo = value;
            OnPropertyChanged();
            OnPropertyChanged(nameof(IsSysInfoLoaded));
            OnPropertyChanged(nameof(IsSysInfoNotLoaded));
        }
    }

    public bool IsProfileLoading
    {
        get => _isProfileLoading;
        set { _isProfileLoading = value; OnPropertyChanged(); }
    }

    public bool IsSysInfoLoading
    {
        get => _isSysInfoLoading;
        set { _isSysInfoLoading = value; OnPropertyChanged(); }
    }

    public bool IsSysInfoLoaded    => _sysInfo != null;
    public bool IsSysInfoNotLoaded => _sysInfo == null;

    public string SettingsStatus
    {
        get => _settingsStatus;
        set { _settingsStatus = value; OnPropertyChanged(); OnPropertyChanged(nameof(HasSettingsStatus)); }
    }
    public bool HasSettingsStatus => !string.IsNullOrEmpty(_settingsStatus);

    // ── Settings bound properties ────────────────────────────────────────────

    public string SettingsToken
    {
        get => _settings.GitHubToken;
        set { _settings.GitHubToken = value; OnPropertyChanged(); }
    }

    public bool SettingsDarkMode
    {
        get => _settings.IsDarkMode;
        set { _settings.IsDarkMode = value; OnPropertyChanged(); ApplyTheme(value); }
    }

    public bool SettingsAutoUpdate
    {
        get => _settings.AutoCheckUpdates;
        set { _settings.AutoCheckUpdates = value; OnPropertyChanged(); }
    }

    public string SettingsDownloadFolder
    {
        get => _settings.DownloadFolder;
        set { _settings.DownloadFolder = value; OnPropertyChanged(); }
    }

    public bool SettingsShowNotifications
    {
        get => _settings.ShowNotifications;
        set { _settings.ShowNotifications = value; OnPropertyChanged(); }
    }

    public bool SettingsMinimizeOnClose
    {
        get => _settings.MinimizeOnClose;
        set { _settings.MinimizeOnClose = value; OnPropertyChanged(); }
    }

    // ── Update count ────────────────────────────────────────────────────────

    public int UpdateCount
    {
        get => _updateCount;
        set { _updateCount = value; OnPropertyChanged(); OnPropertyChanged(nameof(HasPendingUpdates)); OnPropertyChanged(nameof(UpdateCountText)); }
    }
    public bool HasPendingUpdates => _updateCount > 0;
    public string UpdateCountText => _updateCount > 0 ? _updateCount.ToString() : string.Empty;

    // ── Current page ────────────────────────────────────────────────────────

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
            OnPropertyChanged(nameof(IsWindowsAppsPage));
            OnPropertyChanged(nameof(IsStartupPage));
            OnPropertyChanged(nameof(IsWingetPage));
            OnPropertyChanged(nameof(IsProfilePage));
            OnPropertyChanged(nameof(IsSettingsPage));
            OnPropertyChanged(nameof(IsSysInfoPage));
        }
    }

    public bool IsHomePage       => _currentPage == "Home";
    public bool IsLibraryPage    => _currentPage == "Library";
    public bool IsUpdatesPage    => _currentPage == "Updates";
    public bool IsCleanerPage    => _currentPage == "Cleaner";
    public bool IsWindowsAppsPage => _currentPage == "WindowsApps";
    public bool IsStartupPage    => _currentPage == "Startup";
    public bool IsWingetPage     => _currentPage == "Winget";
    public bool IsProfilePage    => _currentPage == "Profile";
    public bool IsSettingsPage   => _currentPage == "Settings";
    public bool IsSysInfoPage    => _currentPage == "SysInfo";

    // ── Collections ─────────────────────────────────────────────────────────

    public ObservableCollection<GitHubRepository> Repositories  { get; } = new();
    public ObservableCollection<GitHubRepository> FeaturedApps  { get; } = new();
    public ObservableCollection<InstalledApp>     InstalledApps { get; } = new();
    public ObservableCollection<InstalledApp>     AppsWithUpdates { get; } = new();
    public ObservableCollection<WingetPackage>    WingetResults { get; } = new();
    public ObservableCollection<RegisteredApp>    WindowsApps   { get; } = new();
    public ObservableCollection<StartupEntry>     StartupEntries { get; } = new();
    public ObservableCollection<CleanerTarget>    CleanerTargets { get; } = new();

    public bool HasNoInstalledApps  => InstalledApps.Count == 0;
    public bool HasCleanStatus      => !string.IsNullOrEmpty(_cleanStatus);
    public bool HasNoWindowsApps    => WindowsApps.Count == 0;
    public bool HasNoStartupEntries => StartupEntries.Count == 0;
    public bool HasNoWingetResults  => WingetResults.Count == 0;
    public bool HasNoCleanerTargets => CleanerTargets.Count == 0;

    // ── Commands ─────────────────────────────────────────────────────────────

    public ICommand SearchCommand               { get; }
    public ICommand NextPageCommand             { get; }
    public ICommand PreviousPageCommand         { get; }
    public ICommand OpenCommand                 { get; }
    public ICommand DownloadCommand             { get; }
    public ICommand InstallCommand              { get; }
    public ICommand CopyCloneCommand            { get; }
    public ICommand CategoryCommand             { get; }
    public ICommand LoadInstalledCommand        { get; }
    public ICommand CheckUpdatesCommand         { get; }
    public ICommand NavigateCommand             { get; }
    public ICommand RemoveCommand               { get; }
    public ICommand UpgradeCommand              { get; }
    public ICommand ShowDetailCommand           { get; }
    public ICommand CloseDetailCommand          { get; }
    public ICommand CleanCacheCommand           { get; }
    public ICommand CleanTargetCommand          { get; }
    public ICommand ScanCleanerCommand          { get; }
    public ICommand UpdateAllCommand            { get; }
    public ICommand ToggleThemeCommand          { get; }
    public ICommand WingetSearchCommand         { get; }
    public ICommand WingetInstallCommand        { get; }
    public ICommand WingetUpgradeCommand        { get; }
    public ICommand WingetUninstallCommand      { get; }
    public ICommand LoadWindowsAppsCommand      { get; }
    public ICommand UninstallWindowsAppCommand  { get; }
    public ICommand LoadStartupCommand          { get; }
    public ICommand DisableStartupCommand       { get; }
    public ICommand EnableStartupCommand        { get; }
    public ICommand DeleteStartupCommand        { get; }
    public ICommand LoadProfileCommand          { get; }
    public ICommand SaveSettingsCommand         { get; }
    public ICommand BrowseDownloadFolderCommand { get; }
    public ICommand LoadSysInfoCommand          { get; }

    // ── Constructor ──────────────────────────────────────────────────────────

    public MainViewModel()
    {
        _settings = _settingsService.Load();

        // Apply persisted dark mode
        if (_settings.IsDarkMode) ApplyTheme(true);

        // Seed profile with local user info; GitHub data loaded on navigate
        _profile = new UserProfile
        {
            WindowsUser  = Environment.UserName,
            ComputerName = Environment.MachineName
        };

        SearchCommand       = new RelayCommand(async _ => { SearchPage = 1; await SearchAsync(); });
        NextPageCommand     = new RelayCommand(async _ => { SearchPage++; await SearchAsync(); });
        PreviousPageCommand = new RelayCommand(async _ => { if (SearchPage > 1) { SearchPage--; await SearchAsync(); } });

        OpenCommand = new RelayCommand(r =>
        {
            var url = r switch
            {
                GitHubRepository repo => repo.HtmlUrl,
                InstalledApp app      => app.SourceUrl,
                string s              => s,
                _                    => null
            };
            if (!string.IsNullOrEmpty(url)) BrowserHelper.Open(url);
        });

        DownloadCommand = new RelayCommand(async r => await DownloadAsync(r));
        InstallCommand  = new RelayCommand(async r => await InstallAsync(r));

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
            SearchText  = q?.ToString() ?? "windows app";
            SearchPage  = 1;
            CurrentPage = "Home";
            await SearchAsync();
        });

        LoadInstalledCommand = new RelayCommand(async _ => await LoadInstalledAsync());
        CheckUpdatesCommand  = new RelayCommand(async _ => await CheckUpdatesAsync());

        NavigateCommand = new RelayCommand(p =>
        {
            CurrentPage = p?.ToString() ?? "Home";
            switch (CurrentPage)
            {
                case "Cleaner":     _ = LoadCleanerAsync();           break;
                case "Library":     _ = LoadInstalledAsync();         break;
                case "Updates":     _ = CheckUpdatesAsync();          break;
                case "WindowsApps": _ = LoadWindowsAppsAsync();       break;
                case "Startup":     _ = LoadStartupAsync();           break;
                case "Winget":      _ = LoadWingetInstalledAsync();   break;
                case "Profile":     _ = LoadProfileAsync();           break;
                case "SysInfo":     _ = LoadSysInfoAsync();           break;
            }
        });

        RemoveCommand      = new RelayCommand(async r => await RemoveAsync(r));
        UpgradeCommand     = new RelayCommand(async r => await UpgradeAsync(r));
        ShowDetailCommand  = new RelayCommand(async r => await ShowDetailAsync(r));
        CloseDetailCommand = new RelayCommand(_ => ShowDetail = false);

        ScanCleanerCommand = new RelayCommand(async _ => await LoadCleanerAsync());
        CleanCacheCommand  = ScanCleanerCommand;

        CleanTargetCommand = new RelayCommand(async r =>
        {
            if (r is not CleanerTarget target) return;
            target.Scanning = true;
            var deleted = await Task.Run(() => _cleaner.Clean(target));
            var (bytes, count) = await Task.Run(() => _cleaner.Scan(target));
            target.SizeBytes  = bytes;
            target.FileCount  = count;
            target.Scanning   = false;
            CleanStatus = $"Cleaned {target.Name}: {deleted} file(s) deleted.";
            StatusText  = CleanStatus;
        });

        UpdateAllCommand   = new RelayCommand(async _ => await UpdateAllAsync());
        ToggleThemeCommand = new RelayCommand(_ => ApplyTheme(!IsDarkMode));

        // ── Winget ──────────────────────────────────────────────────────────
        WingetSearchCommand = new RelayCommand(async _ =>
        {
            if (!_winget.IsAvailable) { WingetStatus = "winget is not available on this system."; return; }
            IsWingetLoading = true;
            WingetStatus = $"Searching for \"{WingetQuery}\"…";
            WingetResults.Clear();
            try
            {
                var results = await _winget.SearchAsync(WingetQuery);
                foreach (var p in results) WingetResults.Add(p);
                WingetStatus = $"{results.Count} result(s) for \"{WingetQuery}\"";
            }
            catch (Exception ex) { WingetStatus = "Search failed: " + ex.Message; }
            finally { IsWingetLoading = false; }
        });

        WingetInstallCommand = new RelayCommand(async r =>
        {
            if (r is not WingetPackage pkg) return;
            WingetStatus = $"Installing {pkg.Id}…";
            var result = await _winget.InstallAsync(pkg.Id);
            WingetStatus = result;
            StatusText   = result;
        });

        WingetUpgradeCommand = new RelayCommand(async r =>
        {
            if (r is not WingetPackage pkg) return;
            WingetStatus = $"Upgrading {pkg.Id}…";
            var result = await _winget.UpgradeAsync(pkg.Id);
            WingetStatus = result;
            StatusText   = result;
        });

        WingetUninstallCommand = new RelayCommand(async r =>
        {
            if (r is not WingetPackage pkg) return;
            WingetStatus = $"Uninstalling {pkg.Id}…";
            var result = await _winget.UninstallAsync(pkg.Id);
            WingetStatus = result;
            StatusText   = result;
        });

        // ── Windows Apps ─────────────────────────────────────────────────────
        LoadWindowsAppsCommand = new RelayCommand(async _ => await LoadWindowsAppsAsync());

        UninstallWindowsAppCommand = new RelayCommand(r =>
        {
            if (r is not RegisteredApp app) return;
            _windowsAppsService.Uninstall(app);
            StatusText = $"Launched uninstaller for {app.DisplayName}.";
        });

        // ── Startup ──────────────────────────────────────────────────────────
        LoadStartupCommand = new RelayCommand(async _ => await LoadStartupAsync());

        DisableStartupCommand = new RelayCommand(r =>
        {
            if (r is not StartupEntry entry) return;
            _startupService.Disable(entry);
            StartupStatus = $"Disabled: {entry.Name}";
            StatusText    = StartupStatus;
        });

        EnableStartupCommand = new RelayCommand(r =>
        {
            if (r is not StartupEntry entry) return;
            _startupService.Enable(entry);
            StartupStatus = $"Enabled: {entry.Name}";
            StatusText    = StartupStatus;
        });

        DeleteStartupCommand = new RelayCommand(r =>
        {
            if (r is not StartupEntry entry) return;
            _startupService.Delete(entry);
            StartupEntries.Remove(entry);
            StartupStatus = $"Deleted startup entry: {entry.Name}";
            StatusText    = StartupStatus;
        });

        // ── Profile & Settings ───────────────────────────────────────────────
        LoadProfileCommand = new RelayCommand(async _ => await LoadProfileAsync());

        SaveSettingsCommand = new RelayCommand(_ =>
        {
            _settings.IsDarkMode = IsDarkMode;
            _settingsService.Save(_settings);
            _gitHubService.AccessToken  = _settings.GitHubToken;
            _releaseService.AccessToken = _settings.GitHubToken;
            _scanner.AccessToken        = _settings.GitHubToken;
            SettingsStatus = "Settings saved successfully.";
            StatusText     = "Settings saved.";
        });

        BrowseDownloadFolderCommand = new RelayCommand(_ =>
        {
            var dialog = new Microsoft.Win32.OpenFolderDialog
            {
                Title            = "Select Download Folder",
                InitialDirectory = string.IsNullOrEmpty(_settings.DownloadFolder)
                    ? Environment.GetFolderPath(Environment.SpecialFolder.UserProfile)
                    : _settings.DownloadFolder
            };
            if (dialog.ShowDialog() == true)
                SettingsDownloadFolder = dialog.FolderName;
        });

        LoadSysInfoCommand = new RelayCommand(async _ => await LoadSysInfoAsync());

        // ── Collection change notifications ──────────────────────────────────
        InstalledApps.CollectionChanged  += (_, _) => OnPropertyChanged(nameof(HasNoInstalledApps));
        WindowsApps.CollectionChanged    += (_, _) => OnPropertyChanged(nameof(HasNoWindowsApps));
        StartupEntries.CollectionChanged += (_, _) => OnPropertyChanged(nameof(HasNoStartupEntries));
        WingetResults.CollectionChanged  += (_, _) => OnPropertyChanged(nameof(HasNoWingetResults));
        CleanerTargets.CollectionChanged += (_, _) => OnPropertyChanged(nameof(HasNoCleanerTargets));

        // Apply persisted token to services
        if (!string.IsNullOrEmpty(_settings.GitHubToken))
        {
            _gitHubService.AccessToken  = _settings.GitHubToken;
            _releaseService.AccessToken = _settings.GitHubToken;
            _scanner.AccessToken        = _settings.GitHubToken;
        }

        _ = LoadFeaturedAsync();
        _ = LoadInstalledAsync();
        _ = SearchAsync();
    }

    // ── GitHub search ─────────────────────────────────────────────────────────

    private async Task SearchAsync()
    {
        try
        {
            IsLoading  = true;
            StatusText = $"Searching GitHub page {SearchPage}…";
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

    // ── Download / Install ────────────────────────────────────────────────────

    private async Task DownloadAsync(object? arg)
    {
        if (arg is not GitHubRepository repo) return;
        StatusText = "Fetching release for " + repo.FullName + "…";
        var release = await _releaseService.GetLatestReleaseAsync(repo.FullName);
        var url = release == null ? repo.ZipUrl : _releaseService.PickWindowsAsset(release) ?? repo.ZipUrl;
        IsDownloading    = true;
        DownloadProgress = 0;
        DownloadLabel    = $"Downloading {repo.Name}…";
        try
        {
            var file = await _zipDownloader.DownloadAsync(url, repo.Name, new Progress<double>(p =>
            {
                DownloadProgress = p;
                DownloadLabel    = $"Downloading {repo.Name} — {p:F0}%";
            }));
            StatusText = $"Downloaded: {file}";
        }
        catch (Exception ex) { StatusText = "Download failed: " + ex.Message; }
        finally { IsDownloading = false; }
    }

    private async Task InstallAsync(object? arg)
    {
        if (arg is not GitHubRepository repo) return;
        StatusText = "Fetching release for " + repo.FullName + "…";
        var release = await _releaseService.GetLatestReleaseAsync(repo.FullName);
        var url     = release == null ? repo.ZipUrl : _releaseService.PickWindowsAsset(release) ?? repo.ZipUrl;
        var version = release?.TagName ?? "zip";

        IsDownloading    = true;
        DownloadProgress = 0;
        DownloadLabel    = $"Installing {repo.Name}…";
        string file;
        try
        {
            file = await _zipDownloader.DownloadAsync(url, repo.Name, new Progress<double>(p =>
            {
                DownloadProgress = p;
                DownloadLabel    = $"Installing {repo.Name} — {p:F0}%";
            }));
        }
        catch (Exception ex) { StatusText = "Install failed: " + ex.Message; IsDownloading = false; return; }
        finally { IsDownloading = false; }

        if (_detector.IsInstaller(file)) _installer.RunInstaller(file);

        var existing = InstalledApps.FirstOrDefault(a => a.FullName == repo.FullName);
        if (existing != null)
        {
            existing.Version     = version;
            existing.InstallPath = file;
            existing.InstalledAt = DateTime.Now;
            existing.HasUpdate   = false;
        }
        else
        {
            InstalledApps.Add(new InstalledApp
            {
                Name        = repo.Name,
                FullName    = repo.FullName,
                SourceUrl   = repo.HtmlUrl,
                InstallPath = file,
                Version     = version,
                InstalledAt = DateTime.Now,
                AvatarUrl   = repo.Owner.AvatarUrl,
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
        StatusText = "Scanning for updates…";
        AppsWithUpdates.Clear();
        foreach (var app in InstalledApps.ToList())
        {
            await _scanner.ScanOneAsync(app);
            if (app.HasUpdate && !AppsWithUpdates.Contains(app)) AppsWithUpdates.Add(app);
        }
        UpdateCount = AppsWithUpdates.Count;
        StatusText  = UpdateCount == 0 ? "All apps are up to date." : $"{UpdateCount} update(s) available.";
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
        StatusText = $"Upgrading {app.FullName}…";
        var release = await _releaseService.GetLatestReleaseAsync(app.FullName);
        if (release == null) { StatusText = "No release found for " + app.FullName; return; }
        var url = _releaseService.PickWindowsAsset(release)
                  ?? $"https://github.com/{app.FullName}/archive/refs/heads/main.zip";

        IsDownloading    = true;
        DownloadProgress = 0;
        DownloadLabel    = $"Upgrading {app.Name} to {release.TagName}…";
        string file;
        try
        {
            file = await _zipDownloader.DownloadAsync(url, app.Name, new Progress<double>(p =>
            {
                DownloadProgress = p;
                DownloadLabel    = $"Upgrading {app.Name} — {p:F0}%";
            }));
        }
        catch (Exception ex) { StatusText = "Upgrade failed: " + ex.Message; IsDownloading = false; return; }
        finally { IsDownloading = false; }

        if (_detector.IsInstaller(file)) _installer.RunInstaller(file);
        app.Version       = release.TagName;
        app.LatestVersion = release.TagName;
        app.InstallPath   = file;
        app.HasUpdate     = false;
        var upd = AppsWithUpdates.FirstOrDefault(a => a.FullName == app.FullName);
        if (upd != null) { AppsWithUpdates.Remove(upd); UpdateCount = AppsWithUpdates.Count; }
        await _database.SaveAsync(InstalledApps.ToList());
        StatusText = $"Upgraded {app.FullName} to {release.TagName}";
    }

    private async Task ShowDetailAsync(object? arg)
    {
        if (arg is not GitHubRepository repo) return;
        SelectedApp              = repo;
        ShowDetail               = true;
        SelectedAppLatestVersion = "Checking…";
        var release              = await _releaseService.GetLatestReleaseAsync(repo.FullName);
        SelectedAppLatestVersion = release?.TagName ?? "No release";
    }

    private async Task UpdateAllAsync()
    {
        var toUpdate = AppsWithUpdates.ToList();
        foreach (var app in toUpdate) await UpgradeAsync(app);
        StatusText = $"Updated {toUpdate.Count} app(s).";
    }

    // ── System Cleaner ────────────────────────────────────────────────────────

    private async Task LoadCleanerAsync()
    {
        CleanerTargets.Clear();
        CleanStatus = string.Empty;
        var targets = _cleaner.BuildTargets();
        foreach (var t in targets)
        {
            t.Scanning = true;
            CleanerTargets.Add(t);
        }
        foreach (var t in CleanerTargets.ToList())
        {
            var (bytes, count) = await Task.Run(() => _cleaner.Scan(t));
            t.SizeBytes = bytes;
            t.FileCount = count;
            t.Scanning  = false;
        }
        StatusText = "Cleaner: scan complete.";
    }

    // ── Windows Apps (registry) ───────────────────────────────────────────────

    private async Task LoadWindowsAppsAsync()
    {
        WindowsAppsStatus = "Loading installed programs…";
        _allWindowsApps.Clear();
        WindowsApps.Clear();
        try
        {
            _allWindowsApps = await Task.Run(() => _windowsAppsService.GetInstalledApps());
            ApplyWindowsAppsFilter();
            WindowsAppsStatus = $"{_allWindowsApps.Count} program(s) found.";
        }
        catch (Exception ex) { WindowsAppsStatus = "Failed to load: " + ex.Message; }
    }

    private void ApplyWindowsAppsFilter()
    {
        WindowsApps.Clear();
        var filter = _windowsAppsFilter.Trim();
        foreach (var a in _allWindowsApps)
        {
            if (string.IsNullOrEmpty(filter) ||
                a.DisplayName.Contains(filter, StringComparison.OrdinalIgnoreCase) ||
                a.Publisher.Contains(filter, StringComparison.OrdinalIgnoreCase))
            {
                WindowsApps.Add(a);
            }
        }
    }

    // ── Startup manager ───────────────────────────────────────────────────────

    private async Task LoadStartupAsync()
    {
        StartupStatus = "Loading startup entries…";
        StartupEntries.Clear();
        try
        {
            var entries = await Task.Run(() => _startupService.GetEntries());
            foreach (var e in entries) StartupEntries.Add(e);
            StartupStatus = $"{entries.Count} startup entry(ies) found.";
        }
        catch (Exception ex) { StartupStatus = "Failed to load: " + ex.Message; }
    }

    // ── Winget ────────────────────────────────────────────────────────────────

    private async Task LoadWingetInstalledAsync()
    {
        if (!_winget.IsAvailable) { WingetStatus = "winget is not available on this system."; return; }
        IsWingetLoading = true;
        WingetStatus    = "Loading installed packages…";
        WingetResults.Clear();
        try
        {
            var packages = await _winget.ListInstalledAsync();
            foreach (var p in packages) WingetResults.Add(p);
            WingetStatus = $"{packages.Count} package(s) installed via winget.";
        }
        catch (Exception ex) { WingetStatus = "Failed: " + ex.Message; }
        finally { IsWingetLoading = false; }
    }

    // ── Profile ───────────────────────────────────────────────────────────────

    private async Task LoadProfileAsync()
    {
        IsProfileLoading = true;
        StatusText = "Loading profile…";
        try
        {
            var loaded = await _profileService.LoadAsync(_settings.GitHubToken);
            Profile = loaded;
            StatusText = loaded.HasGitHubProfile
                ? $"Signed in as @{loaded.Login}"
                : $"Logged in as {loaded.WindowsUser} (no GitHub token)";
        }
        catch (Exception ex) { StatusText = "Profile load failed: " + ex.Message; }
        finally { IsProfileLoading = false; }
    }

    // ── System Info ───────────────────────────────────────────────────────────

    private async Task LoadSysInfoAsync()
    {
        IsSysInfoLoading = true;
        StatusText = "Loading system information…";
        try
        {
            SysInfo    = await Task.Run(() => _sysInfoService.GetSnapshot());
            StatusText = $"System info loaded — {SysInfo.OsDescription}";
        }
        catch (Exception ex) { StatusText = "System info failed: " + ex.Message; }
        finally { IsSysInfoLoading = false; }
    }

    // ── Theme ─────────────────────────────────────────────────────────────────

    private void ApplyTheme(bool dark)
    {
        IsDarkMode = dark;
        var uri  = new Uri(dark ? "Themes/DarkTheme.xaml" : "Themes/StoreTheme.xaml", UriKind.Relative);
        var dict = new ResourceDictionary { Source = uri };
        Application.Current.Resources.MergedDictionaries.Clear();
        Application.Current.Resources.MergedDictionaries.Add(dict);
    }

    public event PropertyChangedEventHandler? PropertyChanged;
    private void OnPropertyChanged([CallerMemberName] string? name = null) =>
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
}
