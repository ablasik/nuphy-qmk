using static NuphyTests.Keyboard;

namespace NuphyTests;

// Fn+M+A: keeping the computer awake.
public class KeepAwakeTests(Renode renode) : IClassFixture<Renode>
{
    // 1.1.0: Fn+M+A keeps the computer awake by moving the pointer 1 px and back every 50 s,
    // without counting as activity for the keyboard's own sleep timers
    [Fact]
    public void Keep_awake_nudges_the_pointer_when_50_s_are_up()
    {
        var kb = new Keyboard(renode).Boot();
        kb.Combo("fn", "m", "a").Run(1500).RfClear(); // and 1 s without typing

        kb.Elapse("last", 49000).Run(500);   // keep_awake_task's last nudge
        Assert.Equal(0, kb.RfCount(Mouse));

        var idle = kb.DoubleWord("no_act_time");
        kb.Run(1000);
        Assert.Equal(new[] { "E0: 00 01 00 00 00", "E0: 00 FF 00 00 00", "E0: 00 00 00 00 00" },
                     kb.RfFrames.Where(f => f.StartsWith("E0")));
        Assert.True(kb.DoubleWord("no_act_time") > idle, "the nudge isn't the user: idle time carries on");
    }

    // a Bluetooth link that blinks for a moment (the computer reconnecting) doesn't put the
    // keyboard straight to deep sleep after hours of keep awake: idle time is long past every timer
    [Fact]
    public void Keep_awake_survives_a_link_blink()
    {
        var kb = new Keyboard(renode).Boot();
        kb.Combo("fn", "m", "a").Run(500);
        kb.Set("no_act_time", 60 * 6000); // an hour without typing

        kb.Rf("State", 2).Run(500).Rf("State", 3).Run(1500); // RF_LINKING, then RF_CONNECT again

        Assert.False(kb.DeepSleep);
        Assert.True(kb.LedPower);
    }

    // keep awake doesn't keep a keyboard that lost its computer awake
    [Fact]
    public void Keep_awake_still_sleeps_once_the_host_is_gone()
    {
        var kb = new Keyboard(renode).Boot();
        kb.Combo("fn", "m", "a").Run(500);
        kb.Rf("State", 4).Run(1000); // RF_DISCONNECT

        kb.Set("rf_disconnect_time", 39);
        Assert.True(kb.RunUntil(() => kb.DeepSleep, 2000));
    }

    [Fact]
    public void Keep_awake_nudges_every_50_s_in_real_time()
    {
        Assert.SkipUnless(Environment.GetEnvironmentVariable("NUPHY_SLOW") == "1", "real-time timer (NUPHY_SLOW=1, always on in CI)");
        var kb = new Keyboard(renode).Boot();
        kb.Combo("fn", "m", "a").RfClear();

        kb.Run(52000);

        Assert.Contains("E0: 00 01 00 00 00", kb.RfFrames);
    }
}
