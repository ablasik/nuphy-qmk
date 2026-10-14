using static NuphyTests.Keyboard;

namespace NuphyTests;

// A recognisable set of changed settings, and the check that they're still there.
internal static class Settings
{
    // Windows on BT 1 (Fn+M+W) and snap tap with the other key back on release (Fn+M+K twice)
    public static Keyboard Change(Keyboard kb) =>
        kb.Combo("fn", "m", "w").Combo("fn", "m", "k").Combo("fn", "m", "k").Run(100);

    // the delayed save is 10 s after the last change
    public static Keyboard Save(Keyboard kb) => kb.Elapse("eeprom_update_timer", 9900).Run(300);

    public static void Check(Keyboard kb)
    {
        kb.RfClear().Tap("f1");
        Assert.Equal(0, kb.RfCount(Consumer)); // F1 is F1, not brightness: Windows

        kb.Press("left").Run(200).Press("right").Run(50);
        Assert.Equal("4F", KeysDown(kb.RfLast(Keys))); // snap tap: right wins
        kb.Release("right").Run(50);
        Assert.Equal("50", KeysDown(kb.RfLast(Keys))); // and left is back
        kb.Release("left").Run(50);
    }
}
