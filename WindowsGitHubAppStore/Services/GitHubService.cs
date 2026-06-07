using System.Net.Http;
using System.Net.Http.Headers;
using System.Text.Json;
using WindowsGitHubAppStore.Models;

namespace WindowsGitHubAppStore.Services;

public class GitHubService
{
    private readonly HttpClient _client = new();

    public string? AccessToken { get; set; }

    public async Task<List<GitHubRepository>> SearchAsync(string query, int page = 1)
    {
        _client.DefaultRequestHeaders.UserAgent.Clear();
        _client.DefaultRequestHeaders.UserAgent.ParseAdd("WindowsGitHubAppStore");
        _client.DefaultRequestHeaders.Authorization = string.IsNullOrWhiteSpace(AccessToken) ? null : new AuthenticationHeaderValue("Bearer", AccessToken);
        var url = $"https://api.github.com/search/repositories?q={Uri.EscapeDataString(query)}&sort=stars&per_page=20&page={page}";
        var json = await _client.GetStringAsync(url);
        var result = JsonSerializer.Deserialize<GitHubSearchResponse>(json);
        return result?.Items ?? new List<GitHubRepository>();
    }
}
