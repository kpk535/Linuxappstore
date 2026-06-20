using System.IO;
using WindowsGitHubAppStore.Models;

namespace WindowsGitHubAppStore.Services;

/// <summary>
/// Provides the list of real Windows cleanup targets and does the actual file deletion.
/// Each target maps to a real folder on the local machine.
/// </summary>
public class SystemCleanerService
{
    public List<CleanerTarget> BuildTargets() => new()
    {
        new CleanerTarget
        {
            Name        = "User Temp Files",
            Icon        = "🗑",
            FolderPath  = Path.GetTempPath(),
            Description = "%TEMP% — temporary files created by apps and the OS"
        },
        new CleanerTarget
        {
            Name        = "Windows Temp",
            Icon        = "⚙",
            FolderPath  = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.Windows), "Temp"),
            Description = "%WINDIR%\\Temp — system-level temporary files"
        },
        new CleanerTarget
        {
            Name        = "Thumbnail Cache",
            Icon        = "🖼",
            FolderPath  = Path.Combine(
                Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
                @"Microsoft\Windows\Explorer"),
            Description = "Explorer thumbnail database files (thumbcache_*.db)"
        },
        new CleanerTarget
        {
            Name        = "GitHub Store Cache",
            Icon        = "📥",
            FolderPath  = ZipDownloadService.DownloadFolder,
            Description = "Installer and archive files downloaded by GitHub Store"
        }
    };

    /// <summary>
    /// Calculates the size and file count for a target folder.
    /// Only counts files in the top level (non-recursive) to avoid
    /// touching OS-protected sub-folders.
    /// </summary>
    public (long Bytes, int Count) Scan(CleanerTarget target)
    {
        if (!Directory.Exists(target.FolderPath)) return (0, 0);
        long bytes = 0;
        int count = 0;
        try
        {
            foreach (var f in Directory.EnumerateFiles(target.FolderPath))
            {
                try { bytes += new FileInfo(f).Length; count++; } catch { }
            }
        }
        catch { }
        return (bytes, count);
    }

    /// <summary>
    /// Deletes files in the target folder's top level.
    /// Returns the number of files successfully deleted.
    /// </summary>
    public int Clean(CleanerTarget target)
    {
        if (!Directory.Exists(target.FolderPath)) return 0;
        int deleted = 0;
        try
        {
            foreach (var f in Directory.EnumerateFiles(target.FolderPath))
            {
                try { File.Delete(f); deleted++; } catch { }
            }
            // Special case: delete thumbcache_*.db by pattern
            if (target.Name == "Thumbnail Cache")
            {
                foreach (var f in Directory.EnumerateFiles(target.FolderPath, "thumbcache_*.db"))
                    try { File.Delete(f); deleted++; } catch { }
            }
        }
        catch { }
        return deleted;
    }

    public static string FormatSize(long bytes)
    {
        if (bytes < 1024) return $"{bytes} B";
        if (bytes < 1024 * 1024) return $"{bytes / 1024.0:F1} KB";
        if (bytes < 1024L * 1024 * 1024) return $"{bytes / (1024.0 * 1024):F1} MB";
        return $"{bytes / (1024.0 * 1024 * 1024):F2} GB";
    }
}
