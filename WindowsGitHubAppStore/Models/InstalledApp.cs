namespace WindowsGitHubAppStore.Models;

public sealed class InstalledApp
{
    public string Name { get; set; } = string.Empty;
    public string FullName { get; set; } = string.Empty;
    public string SourceUrl { get; set; } = string.Empty;
    public string InstallPath { get; set; } = string.Empty;
    public string Version { get; set; } = "unknown";
    public DateTime InstalledAt { get; set; } = DateTime.Now;
    public DateTime LastCheckedAt { get; set; } = DateTime.MinValue;
}
