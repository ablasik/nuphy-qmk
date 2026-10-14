using static NuphyTests.Keyboard;

namespace NuphyTests;

// When the keyboard must not sleep, and when it sleeps on its own.
public class SleepGuardTests(Renode renode) : IClassFixture<Renode>
{
    [Fact]
    public void Fn_Esc_only_locks_while_charging()
    {
        var kb = new Keyboard(renode).Boot(charge: 3).RfClear();

        kb.Combo("fn", "esc").Run(1000);

        Assert.Contains("E1: 09 00 14 00 00 00 00 00", kb.RfFrames);
        Assert.False(kb.DeepSleep);
        Assert.True(kb.LedPower);
    }

    [Fact]
    public void Fn_Esc_only_locks_on_the_cable()
    {
        // on the cable the keyboard sleeps when the computer suspends USB. USB isn't emulated, so
        // the host never suspends it: USB_ACTIVE stays true, which is the guard under test
        var kb = new Keyboard(renode).Boot(wireless: false);
        Assert.Equal(4, kb.Byte("dev_info")); // link_mode == LINK_USB

        kb.Combo("fn", "esc").Run(1000);

        Assert.False(kb.DeepSleep);
        Assert.True(kb.LedPower);
    }

    [Fact]
    public void Losing_the_host_puts_the_keyboard_to_deep_sleep_after_20_s()
    {
        var kb = new Keyboard(renode).Boot();
        kb.Rf("State", 4).Run(1000); // RF_DISCONNECT
        Assert.False(kb.DeepSleep);

        kb.Set("rf_disconnect_time", 39); // sleep_handle counts 40 half-seconds
        Assert.True(kb.RunUntil(() => kb.DeepSleep, 2000));
    }
}
