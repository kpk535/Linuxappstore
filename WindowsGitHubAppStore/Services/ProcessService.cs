using System.Diagnostics;
using WindowsGitHubAppStore.Models;

namespace WindowsGitHubAppStore.Services;

public class ProcessService
{
    public List<ProcessInfo> GetProcesses(string filter = "")
    {
        var result = new List<ProcessInfo>();
        foreach (var p in Process.GetProcesses())
        {
            try
            {
                if (!string.IsNullOrEmpty(filter) &&
                    !p.ProcessName.Contains(filter, StringComparison.OrdinalIgnoreCase))
                    continue;

                result.Add(new ProcessInfo
                {
                    Id       = p.Id,
                    Name     = p.ProcessName,
                    MemoryMB = p.WorkingSet64 / (1024 * 1024),
                });
            }
            catch { }
        }
        return result.OrderByDescending(p => p.MemoryMB).ToList();
    }

    public (bool Success, string Message) Kill(int processId)
    {
        try
        {
            var p = Process.GetProcessById(processId);
            var name = p.ProcessName;
            p.Kill(entireProcessTree: true);
            return (true, $"Killed {name} (PID {processId}).");
        }
        catch (Exception ex) { return (false, $"Failed to kill PID {processId}: {ex.Message}"); }
    }
}
