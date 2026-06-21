using WindowsGitHubAppStore.Models;

namespace WindowsGitHubAppStore.Services;

public class AppUpdateScanner
{
    private readonly GitHubReleaseService _releases = new();

    public string? AccessToken
    {
        get => _releases.AccessToken;
        set => _releases.AccessToken = value;
    }

    public async Task ScanOneAsync(InstalledApp app)
    {
        var latest = await _releases.GetLatestReleaseAsync(app.FullName);
        app.LastCheckedAt = DateTime.Now;
        if (latest == null) return;
        app.LatestVersion = latest.TagName;
        app.HasUpdate = !string.IsNullOrEmpty(app.Version)
                        && app.Version != "zip"
                        && latest.TagName != app.Version;
    }
}
