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
