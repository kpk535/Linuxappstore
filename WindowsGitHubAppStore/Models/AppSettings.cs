namespace WindowsGitHubAppStore.Models;

public class AppSettings
{
    public string GitHubToken { get; set; } = string.Empty;
    public bool IsDarkMode { get; set; }
    public bool AutoCheckUpdates { get; set; } = true;
    public string DownloadFolder { get; set; } = string.Empty;
    public bool ShowNotifications { get; set; } = true;
    public bool MinimizeOnClose { get; set; }
    public int SearchResultsPerPage { get; set; } = 30;
}
