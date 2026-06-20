using System.IO;
using System.Text.Json;
using WindowsGitHubAppStore.Models;

namespace WindowsGitHubAppStore.Services;

public class PackageDatabase
{
    private readonly string _dbPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "WindowsGitHubAppStore", "packages.json");

    public async Task SaveAsync(List<InstalledApp> apps)
    {
        Directory.CreateDirectory(Path.GetDirectoryName(_dbPath)!);
        await File.WriteAllTextAsync(_dbPath, JsonSerializer.Serialize(apps));
    }

    public async Task<List<InstalledApp>> LoadAsync()
    {
        if (!File.Exists(_dbPath)) return new();
        return JsonSerializer.Deserialize<List<InstalledApp>>(await File.ReadAllTextAsync(_dbPath)) ?? new();
    }
}
