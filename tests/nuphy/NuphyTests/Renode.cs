using System.Diagnostics;
using System.Net;
using System.Net.Sockets;
using System.Xml.Linq;

namespace NuphyTests;

// A Renode in Docker (tests/nuphy/renode/Dockerfile) for each test class (IClassFixture), so the
// classes run in parallel; driven through Renode's remote keyword server: ExecuteCommand runs a
// monitor command and returns what it printed.
public sealed class Renode : IAsyncLifetime
{
    public static readonly string RepoRoot = FindRepoRoot();

    public async ValueTask InitializeAsync()
    {
        await Build.Value;
        port = FreePort();
        container = Docker($"run -d --rm -p 127.0.0.1:{port}:{port} -v {RepoRoot}:/work {Image} renode --disable-gui --robot-server-port {port}").Trim();

        var deadline = DateTime.UtcNow.AddMinutes(1);
        while(true)
        {
            try
            {
                await Call("get_keyword_names");
                return;
            }
            catch(Exception) when(DateTime.UtcNow < deadline)
            {
                await Task.Delay(500);
            }
        }
    }

    public ValueTask DisposeAsync()
    {
        if(container != null)
        {
            Docker($"rm -f {container}");
        }
        http.Dispose();
        return ValueTask.CompletedTask;
    }

    // a monitor command; throws with Renode's message if it fails
    public string Execute(string command) => Keyword("ExecuteCommand", command).GetAwaiter().GetResult();

    public void ResetEmulation() => Keyword("ResetEmulation").GetAwaiter().GetResult();

    private async Task<string> Keyword(string name, params string[] args)
    {
        var result = await Call("run_keyword", Value(name), new XElement("value", new XElement("array", new XElement("data", args.Select(Value)))));
        var members = result.Descendants("member").ToDictionary(m => (string)m.Element("name")!, m => m.Element("value")!.Value);
        if(members.GetValueOrDefault("status") != "PASS")
        {
            throw new InvalidOperationException($"{name} {string.Join(" ", args)}: {members.GetValueOrDefault("error")}");
        }
        return members.GetValueOrDefault("return", "").Trim();
    }

    private async Task<XDocument> Call(string method, params XElement[] parameters)
    {
        var body = new XDocument(new XElement("methodCall", new XElement("methodName", method),
            new XElement("params", parameters.Select(p => new XElement("param", p)))));
        using var response = await http.PostAsync($"http://127.0.0.1:{port}/", new StringContent(body.ToString(), System.Text.Encoding.UTF8, "text/xml"));
        response.EnsureSuccessStatusCode();
        var doc = XDocument.Parse(await response.Content.ReadAsStringAsync());
        if(doc.Descendants("fault").Any())
        {
            throw new InvalidOperationException($"{method}: {doc}");
        }
        return doc;
    }

    private static XElement Value(string s) => new("value", new XElement("string", s));

    private static string Docker(string args)
    {
        using var p = Process.Start(new ProcessStartInfo("docker", args) { RedirectStandardOutput = true, RedirectStandardError = true })!;
        var output = p.StandardOutput.ReadToEnd();
        var error = p.StandardError.ReadToEnd();
        p.WaitForExit();
        if(p.ExitCode != 0)
        {
            throw new InvalidOperationException($"docker {args}: {error}");
        }
        return output;
    }

    private static int FreePort()
    {
        var listener = new TcpListener(IPAddress.Loopback, 0);
        listener.Start();
        var port = ((IPEndPoint)listener.LocalEndpoint).Port;
        listener.Stop();
        return port;
    }

    private static string FindRepoRoot()
    {
        for(var dir = new DirectoryInfo(AppContext.BaseDirectory); dir != null; dir = dir.Parent)
        {
            if(Directory.Exists(Path.Combine(dir.FullName, "tests/nuphy/renode")))
            {
                return dir.FullName;
            }
        }
        throw new DirectoryNotFoundException("tests/nuphy/renode not found above " + AppContext.BaseDirectory);
    }

    private static readonly Lazy<Task> Build = new(() => Task.Run(() => Docker($"build -q -t {Image} {Path.Combine(RepoRoot, "tests/nuphy/renode")}")));
    private const string Image = "nuphy-renode";
    private readonly HttpClient http = new() { Timeout = TimeSpan.FromMinutes(10) };
    private string? container;
    private int port;
}
