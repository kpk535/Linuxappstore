using WindowsGitHubAppStore.Models;

namespace WindowsGitHubAppStore.Services;

public class AppUpdateScanner
{
    private readonly GitHubReleaseService releases = new();

    public async Task<string> ScanOneAsync(InstalledApp app)
    {
        var latest = await releases.GetLatestReleaseAsync(app.FullName);
        if (latest == null) return app.FullName + ": no release data";
        if (latest.TagName != app.Version) return app.FullName + ": update " + latest.TagName;
        return app.FullName + ": current";
    }
}
