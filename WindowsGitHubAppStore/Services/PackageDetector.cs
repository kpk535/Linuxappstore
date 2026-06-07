namespace WindowsGitHubAppStore.Services;

public class PackageDetector
{
    public bool IsInstaller(string path)
    {
        string ext = Path.GetExtension(path).ToLowerInvariant();
        return ext == ".exe" || ext == ".msi";
    }

    public bool IsArchive(string path)
    {
        return Path.GetExtension(path).Equals(".zip", StringComparison.OrdinalIgnoreCase);
    }

    public string GetPackageType(string path)
    {
        if (IsInstaller(path)) return "installer";
        if (IsArchive(path)) return "zip";
        return "file";
    }
}
