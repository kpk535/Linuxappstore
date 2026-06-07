namespace WindowsGitHubAppStore.Services;

public class GitHubDeviceFlowAuth
{
    public string ClientId { get; set; } = string.Empty;

    public Task<string> StartLoginAsync()
    {
        return Task.FromResult("Configure GitHub OAuth Client ID and Device Flow endpoints.");
    }
}
