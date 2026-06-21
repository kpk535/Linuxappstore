using System.IO;
using System.Text.Json;
using WindowsGitHubAppStore.Models;

namespace WindowsGitHubAppStore.Services;

public class SettingsService
{
    private static readonly string FilePath = Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData),
        "WindowsGitHubAppStore", "settings.json");

    private static readonly JsonSerializerOptions Opts = new() { WriteIndented = true };

    public AppSettings Load()
    {
        try
        {
            if (File.Exists(FilePath))
                return JsonSerializer.Deserialize<AppSettings>(File.ReadAllText(FilePath)) ?? new AppSettings();
        }
        catch { }
        return new AppSettings();
    }

    public void Save(AppSettings s)
    {
        try
        {
            Directory.CreateDirectory(Path.GetDirectoryName(FilePath)!);
            File.WriteAllText(FilePath, JsonSerializer.Serialize(s, Opts));
        }
        catch { }
    }
}
