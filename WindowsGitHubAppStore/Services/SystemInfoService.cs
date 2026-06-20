using System.IO;
using System.Net.NetworkInformation;
using System.Runtime.InteropServices;
using Microsoft.Win32;
using WindowsGitHubAppStore.Models;

namespace WindowsGitHubAppStore.Services;

public class SystemInfoService
{
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Auto)]
    private sealed class MEMORYSTATUSEX
    {
        public uint   dwLength             = (uint)Marshal.SizeOf<MEMORYSTATUSEX>();
        public uint   dwMemoryLoad;
        public ulong  ullTotalPhys;
        public ulong  ullAvailPhys;
        public ulong  ullTotalPageFile;
        public ulong  ullAvailPageFile;
        public ulong  ullTotalVirtual;
        public ulong  ullAvailVirtual;
        public ulong  ullAvailExtendedVirtual;
    }

    [DllImport("kernel32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool GlobalMemoryStatusEx([In, Out] MEMORYSTATUSEX lpBuffer);

    public SystemSnapshot GetSnapshot()
    {
        var snap = new SystemSnapshot
        {
            MachineName    = Environment.MachineName,
            UserName       = Environment.UserName,
            OsDescription  = RuntimeInformation.OSDescription,
            Architecture   = RuntimeInformation.OSArchitecture.ToString(),
            DotNetVersion  = RuntimeInformation.FrameworkDescription,
            ProcessorCount = Environment.ProcessorCount,
            CpuName        = ReadCpuName(),
            Uptime         = FormatUptime(TimeSpan.FromMilliseconds(Environment.TickCount64)),
        };

        var mem = new MEMORYSTATUSEX();
        if (GlobalMemoryStatusEx(mem))
        {
            snap.TotalRamMB     = (long)(mem.ullTotalPhys / (1024 * 1024));
            snap.AvailRamMB     = (long)(mem.ullAvailPhys / (1024 * 1024));
            snap.RamUsedPercent = (int)mem.dwMemoryLoad;
        }

        snap.Disks = DriveInfo.GetDrives()
            .Where(d => d.IsReady && d.DriveType == DriveType.Fixed)
            .Select(d => new DiskInfo
            {
                Name        = d.Name,
                Label       = d.VolumeLabel,
                DriveFormat = d.DriveFormat,
                TotalBytes  = d.TotalSize,
                FreeBytes   = d.AvailableFreeSpace
            }).ToList();

        snap.NetworkAdapters = NetworkInterface.GetAllNetworkInterfaces()
            .Where(n => n.NetworkInterfaceType != NetworkInterfaceType.Loopback)
            .Select(n =>
            {
                var ipv4 = n.GetIPProperties().UnicastAddresses
                    .FirstOrDefault(a => a.Address.AddressFamily == System.Net.Sockets.AddressFamily.InterNetwork)
                    ?.Address.ToString() ?? "—";
                var rawMac = BitConverter.ToString(n.GetPhysicalAddress().GetAddressBytes());
                return new NetworkAdapterInfo
                {
                    Name        = n.Name,
                    Description = n.Description,
                    Status      = n.OperationalStatus.ToString(),
                    IsUp        = n.OperationalStatus == OperationalStatus.Up,
                    IpAddress   = ipv4,
                    MacAddress  = string.IsNullOrEmpty(rawMac) ? "—" : rawMac.Replace("-", ":"),
                    Speed       = n.Speed > 0 ? FormatSpeed(n.Speed) : "—"
                };
            }).ToList();

        return snap;
    }

    private static string ReadCpuName()
    {
        try
        {
            using var key = Registry.LocalMachine.OpenSubKey(@"HARDWARE\DESCRIPTION\System\CentralProcessor\0");
            return ((key?.GetValue("ProcessorNameString") as string) ?? "Unknown CPU").Trim();
        }
        catch { return "Unknown CPU"; }
    }

    private static string FormatUptime(TimeSpan t)
    {
        if (t.TotalDays >= 1) return $"{(int)t.TotalDays}d {t.Hours}h {t.Minutes}m";
        if (t.TotalHours >= 1) return $"{t.Hours}h {t.Minutes}m";
        return $"{t.Minutes}m {t.Seconds}s";
    }

    private static string FormatSpeed(long bps)
    {
        if (bps >= 1_000_000_000) return $"{bps / 1_000_000_000.0:F0} Gbps";
        if (bps >= 1_000_000)     return $"{bps / 1_000_000.0:F0} Mbps";
        return $"{bps / 1_000.0:F0} Kbps";
    }
}
