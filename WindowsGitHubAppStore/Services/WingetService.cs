using System.Diagnostics;
using System.IO;
using System.Text;
using WindowsGitHubAppStore.Models;

namespace WindowsGitHubAppStore.Services;

/// <summary>
/// Wraps the real winget CLI (Windows Package Manager).
/// All methods call the actual winget binary and parse its output.
/// </summary>
public class WingetService
{
    public bool IsAvailable { get; private set; }

    public WingetService() => IsAvailable = Probe();

    private bool Probe()
    {
        try
        {
            using var p = Start("--version");
            p.WaitForExit(3000);
            return p.ExitCode == 0;
        }
        catch { return false; }
    }

    // ── public API ──────────────────────────────────────────────────────────

    public async Task<List<WingetPackage>> SearchAsync(string query)
    {
        if (!IsAvailable) return new();
        var raw = await ReadOutputAsync(
            $"search --query \"{Sanitize(query)}\" --source winget --accept-source-agreements");
        return ParseTable(raw, hasAvailable: false);
    }

    public async Task<List<WingetPackage>> ListInstalledAsync()
    {
        if (!IsAvailable) return new();
        var raw = await ReadOutputAsync("list --accept-source-agreements");
        return ParseTable(raw, hasAvailable: true);
    }

    public async Task<List<WingetPackage>> GetUpgradesAsync()
    {
        if (!IsAvailable) return new();
        var raw = await ReadOutputAsync("upgrade --include-unknown --accept-source-agreements");
        return ParseTable(raw, hasAvailable: true).Where(p => p.HasUpdate).ToList();
    }

    public async Task<string> InstallAsync(string packageId)
    {
        if (!IsAvailable) return "winget not found on this system.";
        var (_, code) = await RunAsync(
            $"install --id \"{Sanitize(packageId)}\" --silent --accept-package-agreements --accept-source-agreements");
        return code == 0 ? $"Installed {packageId}." : $"Install failed (code {code}).";
    }

    public async Task<string> UpgradeAsync(string packageId)
    {
        if (!IsAvailable) return "winget not found on this system.";
        var (_, code) = await RunAsync(
            $"upgrade --id \"{Sanitize(packageId)}\" --silent --accept-package-agreements --accept-source-agreements");
        return code == 0 ? $"Upgraded {packageId}." : $"Upgrade failed (code {code}).";
    }

    public async Task<string> UninstallAsync(string packageId)
    {
        if (!IsAvailable) return "winget not found on this system.";
        var (_, code) = await RunAsync(
            $"uninstall --id \"{Sanitize(packageId)}\" --silent --accept-source-agreements");
        return code == 0 ? $"Uninstalled {packageId}." : $"Uninstall failed (code {code}).";
    }

    // ── helpers ─────────────────────────────────────────────────────────────

    private static Process Start(string args)
    {
        var psi = new ProcessStartInfo
        {
            FileName = "winget",
            Arguments = args,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            UseShellExecute = false,
            CreateNoWindow = true,
            StandardOutputEncoding = Encoding.UTF8
        };
        return Process.Start(psi)!;
    }

    private static async Task<string> ReadOutputAsync(string args)
    {
        using var p = Start(args);
        var output = await p.StandardOutput.ReadToEndAsync();
        await p.WaitForExitAsync();
        return output;
    }

    private static async Task<(string Output, int ExitCode)> RunAsync(string args)
    {
        using var p = Start(args);
        var output = await p.StandardOutput.ReadToEndAsync();
        await p.WaitForExitAsync();
        return (output, p.ExitCode);
    }

    // Prevent command injection through query strings
    private static string Sanitize(string s) => s.Replace("\"", "").Replace(";", "").Replace("&", "").Replace("|", "");

    /// <summary>
    /// Parses winget's fixed-width columnar output by locating the dash-separator line,
    /// reading column positions from the header above it, then slicing each data row.
    /// </summary>
    private static List<WingetPackage> ParseTable(string raw, bool hasAvailable)
    {
        var list = new List<WingetPackage>();
        var lines = raw.Split('\n');

        int sepIdx = -1;
        for (int i = 0; i < lines.Length; i++)
        {
            if (lines[i].TrimStart().StartsWith("---")) { sepIdx = i; break; }
        }
        if (sepIdx < 1) return list;

        var header = lines[sepIdx - 1];
        int nameCol    = 0;
        int idCol      = ColOf(header, "Id");
        int verCol     = ColOf(header, "Version");
        int availCol   = hasAvailable ? ColOf(header, "Available") : -1;
        int sourceCol  = ColOf(header, "Source");

        for (int i = sepIdx + 1; i < lines.Length; i++)
        {
            var line = lines[i].TrimEnd('\r');
            if (string.IsNullOrWhiteSpace(line)) continue;
            try
            {
                var name  = Slice(line, nameCol, idCol);
                var id    = Slice(line, idCol, verCol);
                var ver   = availCol > 0 ? Slice(line, verCol, availCol)
                                         : Slice(line, verCol, sourceCol > 0 ? sourceCol : line.Length);
                var avail = availCol > 0 ? Slice(line, availCol, sourceCol > 0 ? sourceCol : line.Length) : "";
                var src   = sourceCol > 0 ? Slice(line, sourceCol, line.Length) : "";

                if (!string.IsNullOrEmpty(name))
                    list.Add(new WingetPackage { Name = name, Id = id, Version = ver, AvailableVersion = avail, Source = src });
            }
            catch { }
        }
        return list;
    }

    private static int ColOf(string header, string col)
    {
        int i = header.IndexOf(col, StringComparison.OrdinalIgnoreCase);
        return i;
    }

    private static string Slice(string s, int start, int end)
    {
        if (start < 0 || start >= s.Length) return "";
        int e = (end < 0 || end > s.Length) ? s.Length : end;
        if (e <= start) return "";
        return s[start..e].Trim();
    }
}
