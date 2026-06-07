using WindowsGitHubAppStore.Models;

namespace WindowsGitHubAppStore.Services;

public class StoreUpdateService
{
    private readonly GitHubReleaseService releases = new();
    private readonly ZipDownloadService downloader = new();

    public string StoreRepository { get; set; } = "kpk535/Linuxappstore";
    public string CurrentVersion { get; set; } = "1.0.0";

    public string? AccessToken
    {
        get => releases.AccessToken;
        set => releases.AccessToken = value;
    }

    public async Task<string> CheckStoreUpdateAsync()
    {
        GitHubRelease? latest = await releases.GetLatestReleaseAsync(StoreRepository);
        if (latest == null) return "No store release found. Create a GitHub Release with installer assets.";
        if (latest.TagName == CurrentVersion) return "Store is up to date: " + CurrentVersion;
        return "Store update available: " + latest.TagName;
    }

    public async Task<string> DownloadStoreUpdateAsync()
    {
        GitHubRelease? latest = await releases.GetLatestReleaseAsync(StoreRepository);
        if (latest == null) return "No release available.";
        string url = releases.PickWindowsAsset(latest) ?? latest.HtmlUrl;
        if (url == latest.HtmlUrl) return "Open release page manually: " + latest.HtmlUrl;
        string file = await downloader.DownloadAsync(url, "WindowsGitHubAppStore-" + latest.TagName);
        return file;
    }
}
