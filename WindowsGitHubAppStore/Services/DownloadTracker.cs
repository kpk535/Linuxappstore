namespace WindowsGitHubAppStore.Services;

public class DownloadTracker
{
    public List<string> Downloads { get; } = new();

    public void Track(string repo)
    {
        Downloads.Add(repo);
    }
}
