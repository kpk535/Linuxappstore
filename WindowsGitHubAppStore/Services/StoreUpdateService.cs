using WindowsGitHubAppStore.Models;

namespace WindowsGitHubAppStore.Services;

public class StoreUpdateService
{
    private readonly GitHubReleaseService releases = new();
    private readonly ZipDownloadService downloader = new();

    public string StoreRepository { get; set; } = "kpk535/Linuxappstore";
    public string CurrentVersion  { get; set; } = "1.0.0";

    public string? AccessToken
    {
        get => releases.AccessToken;
        set => releases.AccessToken = value;
    }

    public async Task<string> CheckStoreUpdateAsync()
    {
        GitHubRelease? latest = await releases.GetLatestReleaseAsync(StoreRepository);
        if (latest == null)
            return "Unable to check for updates. Verify your internet connection.";

        var latestTag  = latest.TagName?.TrimStart('v') ?? "";
        var currentTag = CurrentVersion.TrimStart('v');

        if (string.IsNullOrEmpty(latestTag))
            return "No release version found on GitHub.";

        if (latestTag == currentTag)
            return $"Up to date — v{currentTag}";

        if (Version.TryParse(latestTag,  out var latestVer) &&
            Version.TryParse(currentTag, out var currentVer) &&
            latestVer <= currentVer)
            return $"Up to date — v{currentTag}";

        return $"Update available: v{latestTag}";
    }

    public async Task<string> DownloadStoreUpdateAsync()
    {
        GitHubRelease? latest = await releases.GetLatestReleaseAsync(StoreRepository);
        if (latest == null)
            return "No release available to download.";

        string url = releases.PickWindowsAsset(latest) ?? latest.HtmlUrl;
        if (url == latest.HtmlUrl)
            return "Open release page manually: " + latest.HtmlUrl;

        string file = await downloader.DownloadAsync(url, "WindowsGitHubAppStore-" + latest.TagName);
        return file;
    }
}
