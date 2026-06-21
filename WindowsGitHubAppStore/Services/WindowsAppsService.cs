using System.Diagnostics;
using System.IO;
using Microsoft.Win32;
using WindowsGitHubAppStore.Models;

namespace WindowsGitHubAppStore.Services;

/// <summary>
/// Reads the real Windows Uninstall registry keys to enumerate installed programs,
/// and executes their actual UninstallString to remove them.
/// </summary>
public class WindowsAppsService
{
    private static readonly string[] KeyPaths =
    {
        @"SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall",
        @"SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall"
    };

    public List<RegisteredApp> GetInstalledApps()
    {
        var seen = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        var apps = new List<RegisteredApp>();

        foreach (var keyPath in KeyPaths)
        {
            Scan(Registry.LocalMachine, keyPath, apps, seen);
            Scan(Registry.CurrentUser,  keyPath, apps, seen);
        }

        return apps.OrderBy(a => a.DisplayName).ToList();
    }

    private static void Scan(RegistryKey root, string keyPath, List<RegisteredApp> apps, HashSet<string> seen)
    {
        try
        {
            using var key = root.OpenSubKey(keyPath);
            if (key == null) return;

            foreach (var name in key.GetSubKeyNames())
            {
                try
                {
                    using var sub = key.OpenSubKey(name);
                    if (sub == null) continue;

                    // Skip entries without a display name
                    var displayName = sub.GetValue("DisplayName") as string;
                    if (string.IsNullOrWhiteSpace(displayName)) continue;

                    // Skip Windows system components and updates
                    if ((sub.GetValue("SystemComponent") as int?) == 1) continue;
                    if ((sub.GetValue("ReleaseType") as string)?.Contains("Update", StringComparison.OrdinalIgnoreCase) == true) continue;

                    if (!seen.Add(displayName)) continue;  // deduplicate by display name

                    apps.Add(new RegisteredApp
                    {
                        DisplayName         = displayName,
                        DisplayVersion      = sub.GetValue("DisplayVersion")      as string ?? "",
                        Publisher           = sub.GetValue("Publisher")           as string ?? "",
                        UninstallString     = sub.GetValue("UninstallString")     as string ?? "",
                        QuietUninstallString= sub.GetValue("QuietUninstallString")as string ?? "",
                        InstallDate         = FormatDate(sub.GetValue("InstallDate") as string),
                        EstimatedSizeKB     = (sub.GetValue("EstimatedSize") as int?) ?? 0,
                        InstallLocation     = sub.GetValue("InstallLocation")     as string ?? ""
                    });
                }
                catch { }
            }
        }
        catch { }
    }

    /// <summary>
    /// Runs the program's own uninstaller via cmd /c so that quoted paths and
    /// arguments in the uninstall string are handled correctly by the shell.
    /// </summary>
    public void Uninstall(RegisteredApp app)
    {
        var cmd = !string.IsNullOrEmpty(app.QuietUninstallString)
                      ? app.QuietUninstallString
                      : app.UninstallString;
        if (string.IsNullOrEmpty(cmd)) return;

        Process.Start(new ProcessStartInfo
        {
            FileName        = "cmd.exe",
            Arguments       = $"/c {cmd}",
            UseShellExecute = true,
            Verb            = "runas"   // request elevation so uninstallers that need admin can proceed
        });
    }

    private static string FormatDate(string? raw)
    {
        if (string.IsNullOrEmpty(raw) || raw.Length != 8) return raw ?? "";
        // YYYYMMDD → YYYY-MM-DD
        return $"{raw[..4]}-{raw[4..6]}-{raw[6..8]}";
    }
}
