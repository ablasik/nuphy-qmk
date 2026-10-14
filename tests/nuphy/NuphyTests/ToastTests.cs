using static NuphyTests.Keyboard;

namespace NuphyTests;

// Setting changes confirmed on the right side strip.
public class ToastTests(Renode renode) : IClassFixture<Renode>
{
    // 1.1.0: setting changes showed their side-light confirmation only about half the time
    [Theory]
    [InlineData("k", new[] { 1, 2, 3, 0 })]  // Fn+M+K snap tap: off, cancel, exclude, nullify
    [InlineData("d", new[] { 3, 1, 2, 3 })]  // Fn+M+D debounce type
    public void Every_setting_change_shows_its_toast(string key, int[] toasts)
    {
        var kb = new Keyboard(renode).Boot();
        kb.Press("fn").Run(50).Press("m").Run(50);

        foreach(var n in toasts)
        {
            kb.Tap(key).Run(50);
            Assert.Equal(Toast(n), new[] { kb.RightSide(4), kb.RightSide(3), kb.RightSide(2) });
        }
    }

    [Fact]
    public void Every_sleep_mode_change_shows_its_toast()
    {
        var kb = new Keyboard(renode).Boot();
        kb.Press("fn").Run(50);

        foreach(var n in new[] { 2, 1, 3, 2 }) // Fn+]: lights only, never, full saving, ...
        {
            kb.Tap("]").Run(100);
            Assert.Equal(Toast(n), new[] { kb.RightSide(4), kb.RightSide(3), kb.RightSide(2) });
        }
    }

    // 1.0.0: Fn+Shift works like Fn+M
    [Fact]
    public void Fn_Shift_reaches_the_M_layer()
    {
        var kb = new Keyboard(renode).Boot();

        kb.Press("fn").Run(50).Press("shift").Run(50).Press("k").Run(100); // snap tap on: toast 1 of 3

        Assert.Equal(Toast(1), new[] { kb.RightSide(4), kb.RightSide(3), kb.RightSide(2) });
    }
}
