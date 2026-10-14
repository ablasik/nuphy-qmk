using static NuphyTests.Keyboard;

namespace NuphyTests;

// Typing over RF: what goes out to the module for each key.
public class TypingTests(Renode renode) : IClassFixture<Renode>
{
    [Fact]
    public void Boot_handshakes_with_the_RF_module_and_keeps_its_status_in_sync()
    {
        var kb = new Keyboard(renode).Boot();

        Assert.True(kb.RfCount(Hand) >= 1);
        Assert.True(kb.RfCount(ReadData) >= 1);
        Assert.True(kb.RfCount(StatusSync) >= 5); // every 200 ms
    }

    [Fact]
    public void A_key_press_and_its_release_go_out_over_Bluetooth()
    {
        var kb = new Keyboard(renode).Boot().RfClear();

        kb.Press("a").Run(50);
        Assert.Equal("00 00 04 00 00 00 00 00", kb.RfLast(Keys));

        kb.Release("a").Run(50);
        Assert.Equal("00 00 00 00 00 00 00 00", kb.RfLast(Keys));
    }

    // 1.0.0: no more stuck volume keys (consumer reports aren't repeated by the module)
    [Fact]
    public void Volume_keys_are_sent_three_times_and_so_is_their_release()
    {
        var kb = new Keyboard(renode).Boot().RfClear();

        kb.Tap("f12"); // Mac: volume up

        Assert.Equal(new[] { "E3: E9 00", "E3: E9 00", "E3: E9 00", "E3: 00 00", "E3: 00 00", "E3: 00 00" },
                     kb.RfFrames.Where(f => f.StartsWith("E3")));
    }

    // 1.0.0: all NKRO keys work wirelessly
    [Fact]
    public void A_seventh_key_on_Windows_goes_out_in_the_NKRO_report()
    {
        var kb = new Keyboard(renode).Boot(mac: false).RfClear();

        foreach(var key in new[] { "q", "w", "e", "r", "t", "a" }) { kb.Press(key).Run(30); }
        kb.Press("s").Run(50);

        Assert.Equal("00 00 14 1A 08 15 17 04", kb.RfLast(Keys));
        Assert.Equal(0x40, Convert.ToInt32(kb.RfLast(KeyBits).Split(' ')[3], 16)); // S = 0x16: byte 1 + 2, bit 6

        foreach(var key in new[] { "q", "w", "e", "r", "t", "a", "s" }) { kb.Release(key); }
        kb.Run(50);
        Assert.Equal("00 00 00 00 00 00 00 00", kb.RfLast(Keys));
        Assert.All(kb.RfLast(KeyBits).Split(' '), b => Assert.Equal("00", b));
    }

    [Fact]
    public void Snap_tap_lets_the_last_of_A_and_D_win_and_brings_A_back_on_release()
    {
        var kb = new Keyboard(renode).Boot();
        kb.Combo("fn", "m", "k");           // snap tap: last key wins
        kb.Combo("fn", "m", "k").Run(100);  // ... and the other comes back on release

        kb.Press("a").Run(200).Press("d").Run(50); // pressed together (one 100 ms tick) counts as neither first
        Assert.Equal("07", KeysDown(kb.RfLast(Keys))); // D only

        kb.Release("d").Run(50);
        Assert.Equal("04", KeysDown(kb.RfLast(Keys))); // A again
    }
}
