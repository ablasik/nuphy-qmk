using static NuphyTests.Keyboard;

namespace NuphyTests;

// Key backlight: power, battery saving, the Fn layer lookup.
public class BacklightTests(Renode renode) : IClassFixture<Renode>
{
    [Fact]
    public void The_backlight_is_on_after_boot()
    {
        var kb = new Keyboard(renode).Boot();

        Assert.True(kb.LedPower);
        Assert.NotEqual("0 0 0", kb.KeyLight("esc"));
    }

    // 1.0.0: on battery the backlight runs at half brightness
    [Fact]
    public void On_battery_the_backlight_is_half_as_bright_as_when_charging()
    {
        var charging = Rgb(new Keyboard(renode).Boot(charge: 3).KeyLight("esc"));
        var battery = Rgb(new Keyboard(renode).Boot(charge: 0).KeyLight("esc"));

        Assert.True(charging.Max() > 0);
        for(var i = 0; i < 3; i++) { Assert.InRange(battery[i], charging[i] / 2 - 1, charging[i] / 2 + 1); }
    }

    // 1.0.0: ... and fades after 30 s without typing
    [Fact]
    public void On_battery_the_backlight_fades_after_30_s_and_a_key_brings_it_back()
    {
        var kb = new Keyboard(renode).Boot();

        kb.Set("no_act_time", 2900).Run(500); // 10 ms ticks: 29.5 s idle
        Assert.NotEqual("0 0 0", kb.KeyLight("esc"));

        kb.Run(2600); // past 30 s and the 2 s fade
        Assert.Equal("0 0 0", kb.KeyLight("esc"));

        kb.Tap("a").Run(100);
        Assert.NotEqual("0 0 0", kb.KeyLight("esc"));
    }

    // 1.0.0: holding Fn lights every key that does something on the Fn layer, at full brightness
    [Fact]
    public void Holding_Fn_lights_the_Fn_layer_keys_at_full_brightness()
    {
        var kb = new Keyboard(renode).Boot();

        kb.Press("fn").Run(100);

        Assert.Equal("48 255 192", kb.KeyLight("f1"));  // Fn+F1 = F1
        Assert.Equal("0 0 0", kb.KeyLight("a"));        // nothing on Fn+A
    }
}
