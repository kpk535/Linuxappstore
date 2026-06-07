namespace WindowsGitHubAppStore.Services;

public class InstallationManager
{
    public string InstallFolder => Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "WindowsGitHubAppStore", "InstalledApps");

    public void EnsureFolders()
    {
        Directory.CreateDirectory(InstallFolder);
    }
}
