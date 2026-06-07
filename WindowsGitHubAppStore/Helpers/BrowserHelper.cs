using System.Diagnostics;

namespace WindowsGitHubAppStore.Helpers;

public static class BrowserHelper
{
    public static void Open(string url)
    {
        Process.Start(new ProcessStartInfo
        {
            FileName = url,
            UseShellExecute = true
        });
    }
}
