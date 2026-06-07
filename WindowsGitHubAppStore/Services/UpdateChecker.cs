namespace WindowsGitHubAppStore.Services;

public class UpdateChecker
{
    public string CurrentVersion => "1.0.0";

    public Task<bool> CheckForUpdatesAsync()
    {
        return Task.FromResult(false);
    }
}
