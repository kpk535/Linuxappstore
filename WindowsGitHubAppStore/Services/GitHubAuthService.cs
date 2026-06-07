namespace WindowsGitHubAppStore.Services;

public class GitHubAuthService
{
    public string? AccessToken { get; private set; }

    public bool IsAuthenticated => !string.IsNullOrWhiteSpace(AccessToken);

    public void SetToken(string token)
    {
        AccessToken = token;
    }
}
