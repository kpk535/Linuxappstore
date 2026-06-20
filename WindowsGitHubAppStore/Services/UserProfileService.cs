using System.Net.Http;
using System.Net.Http.Headers;
using System.Text.Json;
using WindowsGitHubAppStore.Models;

namespace WindowsGitHubAppStore.Services;

public class UserProfileService
{
    private static readonly HttpClient Http = new();

    public async Task<UserProfile> LoadAsync(string? token)
    {
        var profile = new UserProfile
        {
            WindowsUser  = Environment.UserName,
            ComputerName = Environment.MachineName
        };

        if (string.IsNullOrWhiteSpace(token)) return profile;

        try
        {
            using var req = new HttpRequestMessage(HttpMethod.Get, "https://api.github.com/user");
            req.Headers.Authorization = new AuthenticationHeaderValue("Bearer", token);
            req.Headers.UserAgent.ParseAdd("WindowsGitHubAppStore/1.0");
            using var resp = await Http.SendAsync(req);
            if (!resp.IsSuccessStatusCode) return profile;

            using var doc = JsonDocument.Parse(await resp.Content.ReadAsStringAsync());
            var r = doc.RootElement;
            profile.Login       = GetStr(r, "login");
            profile.Name        = GetStr(r, "name");
            profile.AvatarUrl   = GetStr(r, "avatar_url");
            profile.Bio         = GetStr(r, "bio");
            profile.Location    = GetStr(r, "location");
            profile.PublicRepos = GetInt(r, "public_repos");
            profile.Followers   = GetInt(r, "followers");
            profile.Following   = GetInt(r, "following");
            profile.HasGitHubProfile = true;
        }
        catch { }
        return profile;
    }

    private static string GetStr(JsonElement e, string prop) =>
        e.TryGetProperty(prop, out var v) ? v.GetString() ?? "" : "";

    private static int GetInt(JsonElement e, string prop) =>
        e.TryGetProperty(prop, out var v) ? v.GetInt32() : 0;
}
