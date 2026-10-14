using static NuphyTests.Keyboard;

namespace NuphyTests;

// Fn+M+Esc, the bootloader and firmware updates.
public class BootloaderTests(Renode renode) : IClassFixture<Renode>
{
    // 1.1.0: Fn+M+Esc restarts the keyboard; it used to land in the bootloader almost every time
    [Fact]
    public void Fn_M_Esc_restarts_the_keyboard()
    {
        var kb = new Keyboard(renode).Boot().RfClear();

        kb.Combo("fn", "m", "esc").Started();

        Assert.False(kb.InBootloader);
        Assert.True(kb.RfCount(Hand) >= 1, "started again");
    }

    [Fact]
    public void Holding_Fn_M_Esc_for_3_s_flashes_the_keys_white_and_enters_the_bootloader()
    {
        var kb = new Keyboard(renode).Boot();

        kb.Press("fn").Run(50).Press("m").Run(50).Press("esc");

        Assert.True(kb.RunUntil(() => kb.KeyLight("esc") == "255 255 255", 4000));
        Assert.True(kb.RunUntil(() => kb.InBootloader, 3000));
    }

    // 1.1.0: holding Esc for the bootloader wiped the settings too; 1.1.1: an update reset VIA
    [Fact]
    public void Settings_and_VIA_remaps_survive_a_firmware_update()
    {
        var kb = Settings.Save(Settings.Change(new Keyboard(renode).Boot()).Remap(2, "a", 0x05)); // A -> B on the Windows layer (BT 1 is Windows now)
        kb.Press("fn").Run(50).Press("m").Run(50).Press("esc");
        Assert.True(kb.RunUntil(() => kb.InBootloader, 5000));
        kb.Release("esc").Release("m").Release("fn");

        kb.Update();

        Settings.Check(kb);
        kb.RfClear().Tap("a");
        Assert.Contains("E1: 00 00 05 00 00 00 00 00", kb.RfFrames);
    }
}
