# Quick Guides

`build.py` makes the Quick Guide for every board: `docs/nuphy/<board>_quick_guide.png` and `.pdf`.

## Building

```sh
pip install playwright
playwright install chromium

python3 docs/nuphy/guide/build.py              # all boards
python3 docs/nuphy/guide/build.py air75_v2     # one board
python3 docs/nuphy/guide/build.py --html-only  # just the HTML, to check in a browser
```

Commit the regenerated PNG and PDF together with your change.

## Where things come from

Most of the guide is read from the board's own code, so the guide stays in step with the firmware:

| What | Source |
|---|---|
| Key positions and sizes | `keyboards/nuphy/<board>/ansi/keyboard.json` |
| Printed legends, Fn (mint) and Fn+M (mustard) legends | the default keymap, `keymaps/default/keymap.c` |
| Number of side LEDs | `SIDE_LINE` in the board's driver header |
| Numpad, F row, Fn+Shift = M layer | detected from the keymap |
| VIA file name | `keymaps/*.json` |
| Accent keycaps (mint/coral/mustard) | `boards.json` |

When you change a keymap, rebuild the guides and nothing else needs touching.

## Editing the text

`template.html` is shared by all boards. Refer to functions, not keys:

| Write | Becomes |
|---|---|
| `{{fn LOCK_SCR}}` | Fn + the key that has `LOCK_SCR` on the Fn layer |
| `{{fnm GAME_MODE}}` | Fn + M + the key that has `GAME_MODE` on the M layer |
| `{{key fn:RM_VALD}}` | only that key, e.g. after `{{fn RM_VALU}}` |
| `{{STRIP c fx*}}` | a side strip, top LED first: `c` cyan, `fx` the side effect, `_` off, `x*` fills the rest |
| `<!--if numpad-->…<!--endif-->` | only on boards with a numpad (also `frow`, `fnshift_m`, and `!` to negate) |

If a board doesn't have a function, the whole line that mentions it is left out, so keep each shortcut on its own line.

New keycodes need a short legend in `FUNCTION` in `build.py`; new printed keys go in `PRINTED`.

## Adding a board

Add it to `boards.json` with its accent keycaps and run `build.py`.
