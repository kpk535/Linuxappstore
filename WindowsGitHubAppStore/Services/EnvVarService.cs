using System.Collections;
using WindowsGitHubAppStore.Models;

namespace WindowsGitHubAppStore.Services;

public class EnvVarService
{
    public List<EnvVariable> GetAll()
    {
        var list = new List<EnvVariable>();
        try
        {
            foreach (DictionaryEntry kv in Environment.GetEnvironmentVariables(EnvironmentVariableTarget.User))
                list.Add(new EnvVariable
                {
                    Name  = kv.Key?.ToString()   ?? string.Empty,
                    Value = kv.Value?.ToString()  ?? string.Empty,
                    Scope = "User"
                });
        }
        catch { }
        try
        {
            foreach (DictionaryEntry kv in Environment.GetEnvironmentVariables(EnvironmentVariableTarget.Machine))
                list.Add(new EnvVariable
                {
                    Name  = kv.Key?.ToString()   ?? string.Empty,
                    Value = kv.Value?.ToString()  ?? string.Empty,
                    Scope = "Machine"
                });
        }
        catch { }
        return list.OrderBy(v => v.Scope).ThenBy(v => v.Name).ToList();
    }

    public void SetUser(string name, string value) =>
        Environment.SetEnvironmentVariable(name, value, EnvironmentVariableTarget.User);

    public void DeleteUser(string name) =>
        Environment.SetEnvironmentVariable(name, null, EnvironmentVariableTarget.User);
}
