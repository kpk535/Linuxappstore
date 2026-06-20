using System.IO;
using WindowsGitHubAppStore.Models;

namespace WindowsGitHubAppStore.Services;

public class AppRemovalService
{
    public void Remove(InstalledApp app)
    {
        if (!string.IsNullOrEmpty(app.InstallPath) && File.Exists(app.InstallPath))
            try { File.Delete(app.InstallPath); } catch { }
    }
}
