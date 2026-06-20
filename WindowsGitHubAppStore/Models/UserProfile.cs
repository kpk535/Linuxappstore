namespace WindowsGitHubAppStore.Models;

public class UserProfile
{
    public string Login { get; set; } = string.Empty;
    public string Name { get; set; } = string.Empty;
    public string AvatarUrl { get; set; } = string.Empty;
    public string Bio { get; set; } = string.Empty;
    public string Location { get; set; } = string.Empty;
    public int PublicRepos { get; set; }
    public int Followers { get; set; }
    public int Following { get; set; }
    public string WindowsUser { get; set; } = string.Empty;
    public string ComputerName { get; set; } = string.Empty;
    public bool HasGitHubProfile { get; set; }
    public string DisplayName => !string.IsNullOrEmpty(Name) ? Name : (!string.IsNullOrEmpty(Login) ? Login : WindowsUser);
    public string Initials => DisplayName.Length > 0 ? DisplayName[..1].ToUpper() : "?";
}
