using System.Globalization;

namespace NuphyTests;

// The emulated Air96 V2 (tests/nuphy/renode), as a test sees it.
public sealed class Keyboard(Renode renode)
{
    public static readonly string Elf = Environment.GetEnvironmentVariable("NUPHY_ELF") ?? ".build/nuphy_air96_v2_ansi_default.elf";

    // RF frame commands (user_kb.h)
    public const byte Hand = 0xF2, ReadData = 0x81, StatusSync = 0xC9, SetLink = 0xC0,
                      Mouse = 0xE0, Keys = 0xE1, KeyBits = 0xE2, Consumer = 0xE3;

    // power on a factory-fresh board, with the mode switches and RF module as given, and wait
    // until it's up (post-init handshake, mode switch debounce)
    public Keyboard Boot(bool wireless = true, bool mac = true, byte charge = 0)
    {
        Assert.True(File.Exists(Path.Combine(Renode.RepoRoot, Elf)), $"build the firmware first: {Elf}");
        renode.ResetEmulation();
        renode.Execute($"$elf = @/work/{Elf}");
        renode.Execute("include @/work/tests/nuphy/renode/air_v2.resc");
        renode.Execute($"board Wireless {wireless}");
        renode.Execute($"board Mac {mac}");
        renode.Execute($"rf Charge {charge}");
        return Started();
    }

    // battery out and back in: RAM is gone, flash (settings, VIA keymap) stays
    public Keyboard PowerCycle()
    {
        renode.Execute("machine Reset");
        return Started();
    }

    // up once the mode switches are read (after the post-init RF handshake and their debounce)
    public Keyboard Started()
    {
        Run(1500);
        Assert.True(RunUntil(() => Byte("f_dial_sw_init_ok") == 1, 2000, 100), "the keyboard didn't start");
        return Run(100);
    }

    // flash a new build of the same firmware over the old one, like dfu-util does: only the
    // firmware's own pages are written, then it starts
    public Keyboard Update()
    {
        renode.Execute("sysbus LoadELF $elf");
        renode.Execute("board NewBuildDate");
        return PowerCycle();
    }

    public Keyboard Run(int ms)
    {
        renode.Execute($"emulation RunFor \"{TimeSpan.FromMilliseconds(ms):hh\\:mm\\:ss\\.fff}\"");
        return this;
    }

    // run until `condition` holds, checking every `step` ms; false if it never did within `ms`
    public bool RunUntil(Func<bool> condition, int ms, int step = 50)
    {
        for(var t = 0; t < ms; t += step)
        {
            if(condition()) { return true; }
            Run(step);
        }
        return condition();
    }

    // ---- keys, by keycap (Mac labels, keyboard.json)
    public Keyboard Press(string key) { renode.Execute($"board Press {Position(key)}"); return this; }

    public Keyboard Release(string key) { renode.Execute($"board Release {Position(key)}"); return this; }

    public Keyboard Tap(string key) => Press(key).Run(50).Release(key).Run(50);

    // hold the keys down in order (Fn first), let go in reverse: "fn", "m", "k" is Fn+M+K
    public Keyboard Combo(params string[] keys)
    {
        foreach(var key in keys) { Press(key).Run(50); }
        foreach(var key in keys.Reverse()) { Release(key).Run(50); }
        return this;
    }

    // ---- the RF module
    public int RfCount(byte cmd) => (int)Number(renode.Execute($"rf Count 0x{cmd:X2}"));

    // payload of the last frame of `cmd`, as hex bytes
    public string RfLast(byte cmd) => renode.Execute($"rf Last 0x{cmd:X2}");

    public IReadOnlyList<string> RfFrames => renode.Execute("rf Log").Split('\n', StringSplitOptions.RemoveEmptyEntries);

    public Keyboard RfClear() { renode.Execute("rf Clear"); return this; }

    public Keyboard Rf(string property, int value) { renode.Execute($"rf {property} {value}"); return this; }

    // ---- LEDs, as "r g b"
    public bool LedPower => renode.Execute("board LedPower") == "True";

    public string KeyLight(string key) => renode.Execute($"board KeyLight {Position(key)}");

    public string Light(int index) => renode.Execute($"board Light {index}");

    // side strips (rgb.c side_led_index_tab): pos 0 is the top
    public string LeftSide(int pos) => Light(SideIndex + 4 - pos);

    public string RightSide(int pos) => Light(SideIndex + 5 + pos);

    // the keycodes down in a 6KRO report ("mods 00 k1..k6"), in order: slots aren't positions
    public static string KeysDown(string report) =>
        string.Join(" ", report.Split(' ').Skip(2).Where(b => b != "00").Order());

    public static int[] Rgb(string light) => light.Split(' ').Select(int.Parse).ToArray();

    public static string Colour(string light) => Rgb(light) switch
    {
        [0, > 0, 0] => "green",
        [> 0, 0, 0] => "red",
        _ => light,
    };

    // a setting toast (rgb.c side_toast) on the bottom three right-side LEDs: n of 3 lit in
    // warm white (IND_WHITE), the rest of the scale at 1/8
    public static string[] Toast(int n) => Enumerable.Range(0, 3).Select(i => i < n ? "128 112 88" : "16 14 11").ToArray();

    // ---- MCU
    // STOP mode (deep sleep) is WFI with SCB->SCR.SLEEPDEEP set
    public bool DeepSleep => (Number(renode.Execute("sysbus ReadDoubleWord 0xE000ED10")) & 4) != 0;

    // ST's ROM bootloader (air_v2.resc): waiting for DFU over USB
    public bool InBootloader => Number(renode.Execute("cpu PC")) == 0x1FFFC808;

    public int Byte(string symbol) => (int)Number(renode.Execute($"board Byte \"{symbol}\""));

    public long DoubleWord(string symbol) => Number(renode.Execute($"board DoubleWord \"{symbol}\""));

    public Keyboard Set(string symbol, long value)
    {
        renode.Execute($"board SetDoubleWord \"{symbol}\" 0x{value:X}");
        return this;
    }

    // ---- fast-forward: make the firmware's own timers and counters say time has passed, so a
    // test of what happens after 30 s needn't emulate 30 s

    // timer_read32() as the firmware sees it (ms)
    public long Now()
    {
        renode.Execute("board Call \"timer_read32\"");
        Assert.True(RunUntil(() => renode.Execute("board CallDone") == "True", 500, 5), "timer_read32 didn't run");
        return Number(renode.Execute("board CallResult"));
    }

    // a timer_read32() timestamp `symbol` (e.g. eeprom_update_timer) set `ms` in the past
    public Keyboard Elapse(string symbol, int ms) => Set(symbol, (uint)(Now() - ms));

    // what VIA's "set keycode" does, from the main loop (USB isn't emulated)
    public Keyboard Remap(int layer, string key, int keycode)
    {
        var (row, col) = Matrix[key];
        renode.Execute($"board Call \"nvm_dynamic_keymap_update_keycode\" {layer} {row} {col} 0x{keycode:X}");
        Assert.True(RunUntil(() => renode.Execute("board CallDone") == "True", 500, 5), "the remap didn't run");
        return this;
    }

    private static string Position(string key) => $"{Matrix[key].Item1} {Matrix[key].Item2}";

    private static long Number(string s) =>
        s.StartsWith("0x") ? long.Parse(s[2..], NumberStyles.HexNumber) : long.Parse(s, CultureInfo.InvariantCulture);

    private const int SideIndex = 100; // RGB_MATRIX_LED_COUNT - 2 * SIDE_LINE

    // keycaps the tests press -> matrix position (keyboard.json)
    private static readonly Dictionary<string, (int, int)> Matrix = new()
    {
        ["esc"] = (0, 0), ["f1"] = (0, 2), ["f12"] = (0, 13),
        ["1"] = (1, 1), ["2"] = (1, 2), ["-"] = (1, 11), ["="] = (1, 12),
        ["tab"] = (2, 0), ["q"] = (2, 1), ["w"] = (2, 2), ["e"] = (2, 3), ["r"] = (2, 4), ["t"] = (2, 5), ["]"] = (2, 12),
        ["caps"] = (3, 0), ["a"] = (3, 1), ["s"] = (3, 2), ["d"] = (3, 3), ["f"] = (3, 4), ["g"] = (3, 5), ["k"] = (3, 8),
        ["shift"] = (4, 0), ["m"] = (4, 8),
        ["cmd"] = (5, 2), ["fn"] = (5, 10), ["left"] = (5, 14), ["right"] = (5, 16),
    };
}
