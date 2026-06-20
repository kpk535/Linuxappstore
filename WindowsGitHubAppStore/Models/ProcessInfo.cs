namespace WindowsGitHubAppStore.Models;

public class ProcessInfo
{
    public int    Id      { get; set; }
    public string Name    { get; set; } = string.Empty;
    public long   MemoryMB { get; set; }
    public string MemoryDisplay => MemoryMB >= 1024
        ? $"{MemoryMB / 1024.0:F1} GB"
        : $"{MemoryMB} MB";
}
