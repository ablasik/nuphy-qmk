using static NuphyTests.Keyboard;

namespace NuphyTests;

// Side-light and key indicators for states.
public class IndicatorTests(Renode renode) : IClassFixture<Renode>
{
    // 1.0.0: Caps Lock, game mode on their own side LEDs
    [Fact]
    public void Caps_Lock_on_the_computer_lights_the_top_left_side_LED()
    {
        var kb = new Keyboard(renode).Boot().Run(2000); // after "connected" has had the left strip

        kb.Rf("Leds", 2).Run(500); // the host's Caps Lock LED, relayed by the RF module

        Assert.Equal("0 128 112", kb.LeftSide(0));
    }

    [Fact]
    public void Game_mode_lights_the_third_left_side_LED()
    {
        var kb = new Keyboard(renode).Boot().Run(2000); // after "connected" has had the left strip

        kb.Combo("fn", "m", "g");

        // IND_GAME at the side brightness game mode switches to (game_side_light 2: half)
        Assert.True(kb.RunUntil(() => kb.LeftSide(2) == "64 0 48", 1500), kb.LeftSide(2));
    }

    // 1.1.1: Fn+M+F1 shows whether debug is on; 1.1.0: Fn+M+A whether keep-awake is
    [Theory]
    [InlineData("f1")]
    [InlineData("a")]
    public void A_toggle_flashes_its_key_green_when_on_and_red_when_off(string key)
    {
        var kb = new Keyboard(renode).Boot();

        kb.Combo("fn", "m", key).Run(100);
        Assert.Equal("green", Colour(kb.KeyLight(key)));

        kb.Combo("fn", "m", key).Run(100);
        Assert.Equal("red", Colour(kb.KeyLight(key)));
    }
}
