using System.IO;

namespace WindowsGitHubAppStore.Services;

public class SystemCleanerService
{
    private static readonly string CacheFolder = Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
        "WindowsGitHubAppStore", "Downloads");

    public long GetCacheSize()
    {
        if (!Directory.Exists(CacheFolder)) return 0;
        return Directory.GetFiles(CacheFolder, "*", SearchOption.AllDirectories)
            .Sum(f => { try { return new FileInfo(f).Length; } catch { return 0L; } });
    }

    public int GetCacheFileCount()
    {
        if (!Directory.Exists(CacheFolder)) return 0;
        try { return Directory.GetFiles(CacheFolder, "*", SearchOption.AllDirectories).Length; }
        catch { return 0; }
    }

    public void ClearCache()
    {
        if (!Directory.Exists(CacheFolder)) return;
        foreach (var f in Directory.GetFiles(CacheFolder))
            try { File.Delete(f); } catch { }
    }

    public string FormatSize(long bytes)
    {
        if (bytes < 1024) return $"{bytes} B";
        if (bytes < 1024 * 1024) return $"{bytes / 1024.0:F1} KB";
        if (bytes < 1024L * 1024 * 1024) return $"{bytes / (1024.0 * 1024):F1} MB";
        return $"{bytes / (1024.0 * 1024 * 1024):F2} GB";
    }
}
