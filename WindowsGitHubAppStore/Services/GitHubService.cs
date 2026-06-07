using System.Net.Http;
using System.Text.Json;
using WindowsGitHubAppStore.Models;

namespace WindowsGitHubAppStore.Services;

public class GitHubService
{
    private readonly HttpClient _client = new();

    public async Task<List<GitHubRepository>> SearchAsync(string query)
    {
        _client.DefaultRequestHeaders.UserAgent.ParseAdd("WindowsGitHubAppStore");
        var url = $"https://api.github.com/search/repositories?q={Uri.EscapeDataString(query)}&sort=stars";
        var json = await _client.GetStringAsync(url);
        var result = JsonSerializer.Deserialize<GitHubSearchResponse>(json);
        return result?.Items ?? new List<GitHubRepository>();
    }
}
