using System.Diagnostics;
using System.Net;
using System.Net.Http;
using System.Text.Json;
using System.Threading.Tasks;

namespace WindowsGitHubAppStore.Services;

public class GitHubAuthService
{
    private const string ClientId = "Ov23liKRsOoI0KLR8DdO";
    private const string RedirectUri = "http://localhost:8080/callback";
    private const string AuthorizeUrl = "https://github.com/login/oauth/authorize";
    private const string TokenUrl = "https://github.com/login/oauth/access_token";
    private const string[] Scopes = { "repo", "user:email" };

    private static readonly HttpClient Http = new();

    public string GetAuthorizationUrl()
    {
        var scope = string.Join("+", Scopes);
        return $"{AuthorizeUrl}?client_id={ClientId}&redirect_uri={RedirectUri}&scope={scope}&allow_signup=true";
    }

    public void OpenAuthorizationPage()
    {
        var url = GetAuthorizationUrl();
        var psi = new ProcessStartInfo
        {
            FileName = url,
            UseShellExecute = true
        };
        Process.Start(psi);
    }

    public async Task<(bool Success, string Token, string? Error)> ExchangeCodeForTokenAsync(string code)
    {
        try
        {
            var content = new FormUrlEncodedContent(new[]
            {
                new KeyValuePair<string, string>("client_id", ClientId),
                new KeyValuePair<string, string>("code", code),
                new KeyValuePair<string, string>("redirect_uri", RedirectUri),
            });

            using var req = new HttpRequestMessage(HttpMethod.Post, TokenUrl)
            {
                Content = content
            };
            req.Headers.Accept.ParseAdd("application/json");
            req.Headers.UserAgent.ParseAdd("WindowsGitHubAppStore/1.0");

            using var resp = await Http.SendAsync(req);
            var body = await resp.Content.ReadAsStringAsync();

            if (!resp.IsSuccessStatusCode)
                return (false, "", $"OAuth exchange failed: {resp.StatusCode}");

            using var doc = JsonDocument.Parse(body);
            var root = doc.RootElement;

            if (root.TryGetProperty("access_token", out var tokenElem))
            {
                var token = tokenElem.GetString();
                if (!string.IsNullOrEmpty(token))
                    return (true, token, null);
            }

            if (root.TryGetProperty("error", out var errorElem))
            {
                var error = errorElem.GetString() ?? "Unknown error";
                return (false, "", error);
            }

            return (false, "", "No access_token in response");
        }
        catch (Exception ex)
        {
            return (false, "", $"Exception: {ex.Message}");
        }
    }

    public async Task<string?> StartAuthorizationServerAsync()
    {
        try
        {
            var listener = new HttpListener();
            listener.Prefixes.Add("http://localhost:8080/");
            listener.Start();

            OpenAuthorizationPage();

            var timeout = Task.Delay(TimeSpan.FromMinutes(5));
            var getContext = Task.Run(() => listener.GetContext());

            var completed = await Task.WhenAny(getContext, timeout);

            if (completed == timeout)
            {
                listener.Stop();
                return null;
            }

            var context = await getContext;
            var code = context.Request.QueryString["code"];

            var responseString = "<html><body><h1>Authorization Successful</h1><p>You can close this window and return to the app.</p></body></html>";
            var buffer = System.Text.Encoding.UTF8.GetBytes(responseString);
            context.Response.ContentLength64 = buffer.Length;
            context.Response.OutputStream.Write(buffer, 0, buffer.Length);
            context.Response.OutputStream.Close();

            listener.Stop();

            return code;
        }
        catch { return null; }
    }
}
