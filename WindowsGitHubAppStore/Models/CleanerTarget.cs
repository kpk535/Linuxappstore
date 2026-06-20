using System.ComponentModel;
using System.Runtime.CompilerServices;

namespace WindowsGitHubAppStore.Models;

public sealed class CleanerTarget : INotifyPropertyChanged
{
    public string Name { get; set; } = string.Empty;
    public string FolderPath { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;
    public string Icon { get; set; } = "🗑";

    private long _sizeBytes;
    public long SizeBytes
    {
        get => _sizeBytes;
        set { _sizeBytes = value; OnPropertyChanged(); OnPropertyChanged(nameof(SizeDisplay)); }
    }

    private int _fileCount;
    public int FileCount
    {
        get => _fileCount;
        set { _fileCount = value; OnPropertyChanged(); }
    }

    private bool _scanning;
    public bool Scanning
    {
        get => _scanning;
        set { _scanning = value; OnPropertyChanged(); }
    }

    public string SizeDisplay
    {
        get
        {
            if (_scanning) return "Scanning…";
            if (_sizeBytes < 1024) return $"{_sizeBytes} B";
            if (_sizeBytes < 1024 * 1024) return $"{_sizeBytes / 1024.0:F1} KB";
            if (_sizeBytes < 1024L * 1024 * 1024) return $"{_sizeBytes / (1024.0 * 1024):F1} MB";
            return $"{_sizeBytes / (1024.0 * 1024 * 1024):F2} GB";
        }
    }

    public event PropertyChangedEventHandler? PropertyChanged;
    private void OnPropertyChanged([CallerMemberName] string? n = null) =>
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(n));
}
