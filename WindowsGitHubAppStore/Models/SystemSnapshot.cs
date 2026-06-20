namespace WindowsGitHubAppStore.Models;

public class SystemSnapshot
{
    public string MachineName { get; set; } = string.Empty;
    public string UserName { get; set; } = string.Empty;
    public string OsDescription { get; set; } = string.Empty;
    public string Architecture { get; set; } = string.Empty;
    public string DotNetVersion { get; set; } = string.Empty;
    public string CpuName { get; set; } = string.Empty;
    public int ProcessorCount { get; set; }
    public string Uptime { get; set; } = string.Empty;
    public long TotalRamMB { get; set; }
    public long AvailRamMB { get; set; }
    public int RamUsedPercent { get; set; }
    public long UsedRamMB => TotalRamMB - AvailRamMB;
    public string RamDisplay => $"{UsedRamMB:N0} MB used of {TotalRamMB:N0} MB total";
    public List<DiskInfo> Disks { get; set; } = new();
    public List<NetworkAdapterInfo> NetworkAdapters { get; set; } = new();
}

public class DiskInfo
{
    public string Name { get; set; } = string.Empty;
    public string Label { get; set; } = string.Empty;
    public string DriveFormat { get; set; } = string.Empty;
    public long TotalBytes { get; set; }
    public long FreeBytes { get; set; }
    public long UsedBytes => TotalBytes - FreeBytes;
    public double UsedPercent => TotalBytes > 0 ? (double)UsedBytes / TotalBytes * 100 : 0;
    public string FreeDisplay => FormatSize(FreeBytes);
    public string UsedDisplay => FormatSize(UsedBytes);
    public string TotalDisplay => FormatSize(TotalBytes);
    public string SummaryDisplay => $"{UsedDisplay} used — {FreeDisplay} free of {TotalDisplay}";
    public string LabelDisplay => string.IsNullOrEmpty(Label) ? Name : $"{Label} ({Name})";
    private static string FormatSize(long b) => b >= 1L << 30 ? $"{b / (double)(1L << 30):F1} GB" : $"{b / (double)(1 << 20):F0} MB";
}

public class NetworkAdapterInfo
{
    public string Name { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;
    public string Status { get; set; } = string.Empty;
    public string IpAddress { get; set; } = string.Empty;
    public string MacAddress { get; set; } = string.Empty;
    public string Speed { get; set; } = string.Empty;
    public bool IsUp { get; set; }
}
