namespace WindowsGitHubAppStore.Models;

public sealed class RegisteredApp
{
    public string DisplayName { get; set; } = string.Empty;
    public string DisplayVersion { get; set; } = string.Empty;
    public string Publisher { get; set; } = string.Empty;
    public string UninstallString { get; set; } = string.Empty;
    public string QuietUninstallString { get; set; } = string.Empty;
    public string InstallDate { get; set; } = string.Empty;
    public long EstimatedSizeKB { get; set; }
    public string InstallLocation { get; set; } = string.Empty;
    public bool CanUninstall => !string.IsNullOrEmpty(UninstallString);

    public string SizeDisplay => EstimatedSizeKB switch
    {
        > 1024 => $"{EstimatedSizeKB / 1024:F0} MB",
        > 0    => $"{EstimatedSizeKB} KB",
        _      => "—"
    };
}
