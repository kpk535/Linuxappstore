using System.IO;
using Microsoft.Win32;
using WindowsGitHubAppStore.Models;

namespace WindowsGitHubAppStore.Services;

/// <summary>
/// Reads and manages Windows startup entries from the real registry Run keys.
/// Enabled/disabled state is tracked via StartupApproved (same mechanism Task Manager uses).
/// </summary>
public class StartupService
{
    private const string RunKey      = @"Software\Microsoft\Windows\CurrentVersion\Run";
    private const string ApprovedKey = @"Software\Microsoft\Windows\CurrentVersion\Explorer\StartupApproved\Run";

    public List<StartupEntry> GetEntries()
    {
        var entries = new List<StartupEntry>();
        ReadRoot(Registry.CurrentUser,  "Current User", entries);
        ReadRoot(Registry.LocalMachine, "All Users",    entries);
        return entries.OrderBy(e => e.Name).ToList();
    }

    private static void ReadRoot(RegistryKey root, string source, List<StartupEntry> entries)
    {
        try
        {
            using var run = root.OpenSubKey(RunKey);
            if (run == null) return;

            foreach (var name in run.GetValueNames())
            {
                var cmd = run.GetValue(name) as string ?? "";
                bool enabled = ReadEnabled(root, name);
                entries.Add(new StartupEntry
                {
                    Name       = name,
                    Command    = cmd,
                    Source     = source,
                    RegistryKey= RunKey,
                    Enabled    = enabled
                });
            }
        }
        catch { }
    }

    /// <summary>
    /// Reads the StartupApproved binary value. First byte 0x02 = enabled, 0x03 = disabled.
    /// If the value is absent the entry is considered enabled (Windows default behaviour).
    /// </summary>
    private static bool ReadEnabled(RegistryKey root, string name)
    {
        try
        {
            using var approved = root.OpenSubKey(ApprovedKey);
            if (approved == null) return true;
            if (approved.GetValue(name) is byte[] data && data.Length >= 1)
                return data[0] == 0x02;
        }
        catch { }
        return true;
    }

    /// <summary>Disables a startup entry without deleting it (writes 0x03 to StartupApproved).</summary>
    public void Disable(StartupEntry entry)
    {
        WriteApproved(entry, enabled: false);
        entry.Enabled = false;
    }

    /// <summary>Re-enables a startup entry (writes 0x02 to StartupApproved).</summary>
    public void Enable(StartupEntry entry)
    {
        WriteApproved(entry, enabled: true);
        entry.Enabled = true;
    }

    /// <summary>Permanently removes a startup entry from the Run key.</summary>
    public void Delete(StartupEntry entry)
    {
        var root = entry.Source == "Current User" ? Registry.CurrentUser : Registry.LocalMachine;
        try
        {
            using var run = root.OpenSubKey(entry.RegistryKey, writable: true);
            run?.DeleteValue(entry.Name, throwOnMissingValue: false);
        }
        catch { }
        // Also clean up the approved entry
        try
        {
            using var approved = root.OpenSubKey(ApprovedKey, writable: true);
            approved?.DeleteValue(entry.Name, throwOnMissingValue: false);
        }
        catch { }
    }

    private static void WriteApproved(StartupEntry entry, bool enabled)
    {
        var root = entry.Source == "Current User" ? Registry.CurrentUser : Registry.LocalMachine;
        try
        {
            using var approved = root.CreateSubKey(ApprovedKey);
            // 12-byte value: first byte controls state, rest are a timestamp written by Windows
            var data = new byte[12];
            data[0] = enabled ? (byte)0x02 : (byte)0x03;
            approved.SetValue(entry.Name, data, RegistryValueKind.Binary);
        }
        catch { }
    }
}
