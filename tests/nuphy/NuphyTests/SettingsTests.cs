using static NuphyTests.Keyboard;

namespace NuphyTests;

// Settings and the VIA keymap live in the emulated flash: checked by what the keyboard does
// after power cycles and restarts.
public class SettingsTests(Renode renode) : IClassFixture<Renode>
{
    // 1.0.0: settings are saved 10 s after the last change
    [Fact]
    public void Settings_saved_after_10_s_survive_taking_the_battery_out()
    {
        var kb = Settings.Save(Settings.Change(new Keyboard(renode).Boot()));

        kb.PowerCycle();

        Settings.Check(kb);
    }

    // 1.1.0: a setting changed less than 10 s before a restart (Fn+M+Esc) is no longer lost
    [Fact]
    public void A_setting_changed_just_before_Fn_M_Esc_is_kept()
    {
        var kb = Settings.Change(new Keyboard(renode).Boot());

        kb.Combo("fn", "m", "esc").Started();

        Settings.Check(kb);
    }

    [Fact]
    public void A_VIA_remap_takes_effect_and_survives_a_power_cycle()
    {
        var kb = new Keyboard(renode).Boot().Remap(0, "a", 0x05);

        kb.RfClear().Tap("a");
        Assert.Contains("E1: 00 00 05 00 00 00 00 00", kb.RfFrames);

        kb.PowerCycle().RfClear().Tap("a");
        Assert.Contains("E1: 00 00 05 00 00 00 00 00", kb.RfFrames);
    }
}
