using System.Net.Http;

namespace WindowsGitHubAppStore.Services;

public class ZipDownloadService
{
    public async Task<string> DownloadAsync(string url,string fileName)
    {
        string folder = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.Downloads), "GitHubAppStore");
        Directory.CreateDirectory(folder);
        string file = Path.Combine(folder,fileName + ".zip");
        using HttpClient client = new();
        await File.WriteAllBytesAsync(file, await client.GetByteArrayAsync(url));
        return file;
    }
}
