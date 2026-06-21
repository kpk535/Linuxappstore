using System.ComponentModel;
using System.Runtime.CompilerServices;

namespace WindowsGitHubAppStore.Models;

public sealed class InstalledApp : INotifyPropertyChanged
{
    public string Name { get; set; } = string.Empty;
    public string FullName { get; set; } = string.Empty;
    public string SourceUrl { get; set; } = string.Empty;
    public string InstallPath { get; set; } = string.Empty;
    public string Version { get; set; } = "unknown";
    public string LatestVersion { get; set; } = string.Empty;
    public string AvatarUrl { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;
    public DateTime InstalledAt { get; set; } = DateTime.Now;
    public DateTime LastCheckedAt { get; set; } = DateTime.MinValue;

    private bool _hasUpdate;
    public bool HasUpdate
    {
        get => _hasUpdate;
        set { _hasUpdate = value; OnPropertyChanged(); }
    }

    public event PropertyChangedEventHandler? PropertyChanged;
    private void OnPropertyChanged([CallerMemberName] string? name = null) =>
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
}
