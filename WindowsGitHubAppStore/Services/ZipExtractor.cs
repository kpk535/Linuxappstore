namespace WindowsGitHubAppStore.Services;

public class ZipExtractor
{
    public string GetExtractFolder(string zipPath)
    {
        string folder = Path.Combine(Path.GetDirectoryName(zipPath) ?? string.Empty, Path.GetFileNameWithoutExtension(zipPath));
        Directory.CreateDirectory(folder);
        return folder;
    }
}
