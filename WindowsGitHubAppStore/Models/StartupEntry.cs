using System.ComponentModel;
using System.Runtime.CompilerServices;

namespace WindowsGitHubAppStore.Models;

public sealed class StartupEntry : INotifyPropertyChanged
{
    public string Name { get; set; } = string.Empty;
    public string Command { get; set; } = string.Empty;
    public string Source { get; set; } = string.Empty;  // "Current User" | "All Users"
    public string RegistryKey { get; set; } = string.Empty;

    private bool _enabled = true;
    public bool Enabled
    {
        get => _enabled;
        set { _enabled = value; OnPropertyChanged(); OnPropertyChanged(nameof(StatusText)); }
    }

    public string StatusText => Enabled ? "Enabled" : "Disabled";

    public event PropertyChangedEventHandler? PropertyChanged;
    private void OnPropertyChanged([CallerMemberName] string? n = null) =>
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(n));
}
