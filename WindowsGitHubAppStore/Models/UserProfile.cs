namespace WindowsGitHubAppStore.Models;

public class UserProfile
{
    public string Login        { get; set; } = string.Empty;
    public string Name         { get; set; } = string.Empty;
    public string AvatarUrl    { get; set; } = string.Empty;
    public string Bio          { get; set; } = string.Empty;
    public string Location     { get; set; } = string.Empty;
    public string Company      { get; set; } = string.Empty;
    public string Email        { get; set; } = string.Empty;
    public string ProfileUrl   { get; set; } = string.Empty;
    public string CreatedAt    { get; set; } = string.Empty;
    public int    PublicRepos  { get; set; }
    public int    Followers    { get; set; }
    public int    Following    { get; set; }
    public string WindowsUser  { get; set; } = string.Empty;
    public string ComputerName { get; set; } = string.Empty;
    public bool   HasGitHubProfile { get; set; }
    public bool   HasAvatar    => HasGitHubProfile && !string.IsNullOrEmpty(AvatarUrl);
    public string DisplayName  => !string.IsNullOrEmpty(Name) ? Name : (!string.IsNullOrEmpty(Login) ? Login : WindowsUser);
    public string Initials     => DisplayName.Length > 0 ? DisplayName[..1].ToUpper() : "?";
    public string MemberSince  => string.IsNullOrEmpty(CreatedAt) ? "" :
        DateTime.TryParse(CreatedAt, out var d) ? $"Member since {d:MMMM yyyy}" : "";
    public string CompanyClean => Company.TrimStart('@');
}
