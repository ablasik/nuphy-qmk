using static NuphyTests.Keyboard;

namespace NuphyTests;

// Fn+Esc: lock the computer, sleep the keyboard; and waking up.
public class SleepTests(Renode renode) : IClassFixture<Renode>
{
    [Fact]
    public void Fn_Esc_locks_the_Mac_and_puts_the_keyboard_to_sleep_right_away()
    {
        var kb = new Keyboard(renode).Boot().RfClear();

        kb.Combo("fn", "esc").Run(1000);

        Assert.Contains("E1: 09 00 14 00 00 00 00 00", kb.RfFrames); // Ctrl+Cmd+Q
        Assert.True(kb.DeepSleep);
        Assert.False(kb.LedPower);
        // 1.0.0: going to sleep doesn't send an empty mouse report, which woke a sleeping Mac
        Assert.Equal(0, kb.RfCount(Mouse));
    }

    [Fact]
    public void Fn_Esc_locks_Windows_with_Win_L()
    {
        var kb = new Keyboard(renode).Boot(mac: false).RfClear();

        kb.Combo("fn", "esc").Run(1000);

        Assert.Contains("E1: 08 00 0F 00 00 00 00 00", kb.RfFrames);
        Assert.True(kb.DeepSleep);
    }

    // 1.0.0: the first key after deep sleep is no longer lost
    [Fact]
    public void A_key_wakes_the_keyboard_and_still_goes_out()
    {
        var kb = new Keyboard(renode).Boot();
        kb.Combo("fn", "esc").Run(1000);
        Assert.True(kb.DeepSleep);
        kb.RfClear();

        kb.Press("a").Run(1500); // queued until the link is back: rf_delay, 680 ms on BT

        Assert.False(kb.DeepSleep);
        Assert.True(kb.LedPower);
        Assert.True(kb.RfCount(Hand) >= 1, "the wake path (exit_light_sleep) handshakes again");
        Assert.Contains("E1: 00 00 04 00 00 00 00 00", kb.RfFrames);
        Assert.Equal(0, kb.RfCount(0x00)); // nothing dequeued yet used to go out as empty frames
    }

    // the report the last wake left in the queue went out again first on the next wake
    [Fact]
    public void The_next_wake_doesnt_replay_keys_from_the_last_one()
    {
        var kb = new Keyboard(renode).Boot();
        kb.Combo("fn", "esc").Run(1000);
        kb.Press("cmd").Run(100).Tap("tab").Run(1500); // Cmd still held as the queue drains
        kb.Release("cmd").Run(200);
        kb.Combo("fn", "esc").Run(1000);
        Assert.True(kb.DeepSleep);
        kb.RfClear();

        kb.Press("a").Run(1500);

        Assert.Contains("E1: 00 00 04 00 00 00 00 00", kb.RfFrames);
        Assert.DoesNotContain(kb.RfFrames, f => f.StartsWith("E1: 08")); // no Cmd from the last wake
    }

    // same over a link blink mid-typing: no sleep in between to clear it, only 750 ms idle
    [Fact]
    public void A_link_blink_doesnt_replay_keys_from_the_last_one()
    {
        var kb = new Keyboard(renode).Boot();
        kb.Rf("State", 2).Run(300).Press("cmd").Run(100); // RF_LINKING: queued
        kb.Rf("State", 3).Run(550).Tap("tab").Run(300); // typing on as the queue drains (680 ms), Cmd held
        kb.Release("cmd").Run(50).RfClear();

        kb.Rf("State", 2).Run(300).Press("q").Run(100).Rf("State", 3).Run(1500);

        Assert.Contains("E1: 00 00 14 00 00 00 00 00", kb.RfFrames);
        Assert.DoesNotContain(kb.RfFrames, f => f.StartsWith("E1: 08")); // no Cmd from before the blink
    }
}
