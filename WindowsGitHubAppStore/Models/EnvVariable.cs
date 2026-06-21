namespace WindowsGitHubAppStore.Models;

public class EnvVariable
{
    public string Name  { get; set; } = string.Empty;
    public string Value { get; set; } = string.Empty;
    public string Scope { get; set; } = "User";
    public bool   IsUserScope    => Scope == "User";
    public bool   IsMachineScope => Scope == "Machine";
    public string ValuePreview   => Value.Length > 90 ? Value[..90] + "…" : Value;
}
