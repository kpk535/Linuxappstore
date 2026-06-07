using System.Collections.Generic;
using System.Text.Json.Serialization;

namespace WindowsGitHubAppStore.Models;

public sealed class GitHubSearchResponse
{
    [JsonPropertyName("total_count")]
    public int TotalCount { get; set; }

    [JsonPropertyName("incomplete_results")]
    public bool IncompleteResults { get; set; }

    [JsonPropertyName("items")]
    public List<GitHubRepository> Items { get; set; } = new();
}
