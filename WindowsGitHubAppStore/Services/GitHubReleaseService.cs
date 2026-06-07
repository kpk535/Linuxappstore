using System.Net.Http;
using System.Net.Http.Headers;
using System.Text.Json;
using WindowsGitHubAppStore.Models;

namespace WindowsGitHubAppStore.Services;

public class GitHubReleaseService
{
    private readonly HttpClient _client = new();
    public string? AccessToken { get; set; }

    public async Task<GitHubRelease?> GetLatestReleaseAsync(string fullName)
    {
        _client.DefaultRequestHeaders.UserAgent.Clear();
        _client.DefaultRequestHeaders.UserAgent.ParseAdd("WindowsGitHubAppStore");
        _client.DefaultRequestHeaders.Authorization = string.IsNullOrWhiteSpace(AccessToken) ? null : new AuthenticationHeaderValue("Bearer", AccessToken);
        string url = "https://api.github.com/repos/" + fullName + "/releases/latest";
        try
        {
            string json = await _client.GetStringAsync(url);
            return JsonSerializer.Deserialize<GitHubRelease>(json);
        }
        catch
        {
            return null;
        }
    }

    public string? PickWindowsAsset(GitHubRelease release)
    {
        var asset = release.Assets.FirstOrDefault(a => a.Name.EndsWith(".msi", StringComparison.OrdinalIgnoreCase))
            ?? release.Assets.FirstOrDefault(a => a.Name.EndsWith(".exe", StringComparison.OrdinalIgnoreCase))
            ?? release.Assets.FirstOrDefault(a => a.Name.EndsWith(".zip", StringComparison.OrdinalIgnoreCase));
        return asset?.BrowserDownloadUrl;
    }
}
