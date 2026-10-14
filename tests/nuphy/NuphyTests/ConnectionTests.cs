using static NuphyTests.Keyboard;

namespace NuphyTests;

// Switching wireless slots and what each slot remembers.
public class ConnectionTests(Renode renode) : IClassFixture<Renode>
{
    // 1.0.0: no lost key releases when switching connections
    [Fact]
    public void Switching_the_connection_releases_held_keys_first()
    {
        var kb = new Keyboard(renode).Boot();
        kb.Press("a").Run(50).RfClear();

        kb.Press("fn").Run(50).Press("2").Run(50);

        Assert.Equal("00 00 00 00 00 00 00 00", kb.RfLast(Keys));
    }

    [Fact]
    public void Fn_2_switches_to_Bluetooth_2_and_Fn_Tab_back_to_1()
    {
        var kb = new Keyboard(renode).Boot().RfClear();

        kb.Combo("fn", "2").Run(500);
        Assert.Equal("02", kb.RfLast(SetLink));
        Assert.Equal(2, kb.Byte("dev_info")); // link_mode

        kb.Combo("fn", "tab").Run(500);
        Assert.Equal("01", kb.RfLast(SetLink));
        Assert.Equal(1, kb.Byte("dev_info"));
    }

    // 1.0.0: Mac/Win is remembered per connection, Fn+M+W flips it
    [Fact]
    public void Fn_M_W_flips_Mac_and_Windows_for_this_connection_only()
    {
        var kb = new Keyboard(renode).Boot();

        kb.Combo("fn", "m", "w").Run(300).RfClear().Tap("f1");
        Assert.Equal("00 00 3A 00 00 00 00 00", kb.RfFrames.Where(f => f.StartsWith("E1")).First()[4..]); // Windows: F1
        Assert.Equal(0, kb.RfCount(Consumer));

        kb.Combo("fn", "2").Run(500).RfClear().Tap("f1");
        Assert.Equal("70 00", kb.RfFrames.First(f => f.StartsWith("E3"))[4..]); // Mac: brightness down

        kb.Combo("fn", "1").Run(500).RfClear().Tap("f1");
        Assert.Equal(0, kb.RfCount(Consumer)); // back on BT 1: Windows again
    }

    // 1.0.0: the connection indicator lights the key that really holds BT 1-3 / 2.4G
    [Fact]
    public void Connecting_to_Bluetooth_2_lights_the_2_key_blue()
    {
        var kb = new Keyboard(renode).Boot();

        kb.Combo("fn", "2").Run(500);

        var rgb = Rgb(kb.KeyLight("2"));
        Assert.True(rgb[0] == 0 && rgb[2] > rgb[1], $"blue, got {kb.KeyLight("2")}");
    }
}
