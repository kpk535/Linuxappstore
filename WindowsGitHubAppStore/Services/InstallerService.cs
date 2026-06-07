using System.Diagnostics;

namespace WindowsGitHubAppStore.Services;

public class InstallerService
{
    public void RunInstaller(string path)
    {
        Process.Start(new ProcessStartInfo
        {
            FileName = path,
            UseShellExecute = true
        });
    }
}
