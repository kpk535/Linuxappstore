using System.IO;
using System.Net.Http;

namespace WindowsGitHubAppStore.Services;

public class ZipDownloadService
{
    public static readonly string DownloadFolder = Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
        "WindowsGitHubAppStore", "Downloads");

    public async Task<string> DownloadAsync(string url, string fileName, IProgress<double>? progress = null)
    {
        Directory.CreateDirectory(DownloadFolder);
        var ext = Path.GetExtension(new Uri(url).AbsolutePath);
        if (string.IsNullOrEmpty(ext)) ext = ".zip";
        var file = Path.Combine(DownloadFolder, fileName + ext);

        using var client = new HttpClient();
        client.DefaultRequestHeaders.UserAgent.ParseAdd("WindowsGitHubAppStore");
        using var response = await client.GetAsync(url, HttpCompletionOption.ResponseHeadersRead);
        var total = response.Content.Headers.ContentLength ?? -1L;
        await using var src = await response.Content.ReadAsStreamAsync();
        await using var dst = File.Create(file);
        var buf = new byte[81920];
        long downloaded = 0;
        int read;
        while ((read = await src.ReadAsync(buf)) > 0)
        {
            await dst.WriteAsync(buf.AsMemory(0, read));
            downloaded += read;
            if (total > 0) progress?.Report(downloaded * 100.0 / total);
        }
        progress?.Report(100);
        return file;
    }
}
