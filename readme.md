# nuphy-qmk

Community-maintained QMK firmware for NuPhy keyboards.

NuPhy no longer develops the QMK firmware for its V2 boards, and the best community fork ([adi4086](https://github.com/adi4086/qmk_firmware)) is no longer maintained either. This project picks it up: it tracks current upstream QMK, ports NuPhy's own fixes, and adds new features.

| Keyboard   | Status       | Branch     |
|------------|--------------|------------|
| Air96 V2   | ✅ maintained | `air96-v2` |
| Air75 V2   | planned      |            |
| Air60 V2   | planned      |            |

> [!WARNING]
> This is unofficial firmware. Flashing it is at your own risk. The STM32 DFU bootloader can't be overwritten, so you can always go back to stock firmware.

## What's different from stock

On top of NuPhy's firmware (via adi4086's work):

- **Wireless reliability**
  - Far fewer stuck or dropped keys over BT/2.4G.
  - Keystrokes pressed while the keyboard is waking up are buffered, not lost.
  - Fixed random freezes.
- **Battery**
  - Real MCU deep sleep.
  - Variable matrix scan rate when idle.
  - Adjustable sleep timers.
- **Game mode** (`Fn+M+G`): max scan rate (~3500 Hz) and SOCD/snap-tap modes (`Fn+M+K`).
- **Debounce**: algorithm and time adjustable from the keyboard.
- **Caps Word**: double-tap Left Shift.
- **Win lock** and a NumLock/Insert dual key.
- **Battery level** shown on the number row (`Fn+B`).
- **All QMK RGB effects**, plus fewer EEPROM writes.

Added in this repo:

- Rebased on **current upstream QMK** (adi4086's fork was frozen at early 2025).
- **RF packet validation** ported from NuPhy firmware 2.1.x.
- **Readable lights**: holding Fn lights up every key that does something; the side strips show only what changes how your keys behave (Caps, Num, Win lock, game mode) plus battery and connection events.
- **Mac/Win per connection**: USB, BT1–3 and 2.4G each remember their own layout. Flip the switch once per device; switching connections restores it.
- **Wireless fixes**:
  - Faster BT reconnect after wake.
  - The first key after deep sleep is no longer swallowed.
  - The keyboard no longer wakes a sleeping Mac.
  - All NKRO keys work over BT/2.4G.
  - No more stuck volume keys.
  - No more lost key releases on link switch.
- **USB fixes**:
  - A keypress wakes a sleeping computer.
  - No deep sleep (and USB detach) while on the cable.
  - Unplugging the charging cable no longer reboots the board.
- **Other fixes and changes**:
  - VIA effect list matches the firmware.
  - Win Fn+PrtSc opens the Snipping Tool.
  - Fn+Shift works like Fn+M.
  - Stock side-light brightness by default.
  - Battery-saver fixes.

## Quick Guide

![Air96 V2 Quick Guide](keyboards/nuphy/air96_v2/ansi/quick_guide.png)

[Download the Quick Guide as PDF](keyboards/nuphy/air96_v2/ansi/quick_guide.pdf)

The full list of key combos is in [`keyboards/nuphy/air96_v2/ansi/customizations.md`](keyboards/nuphy/air96_v2/ansi/customizations.md).

## Requirements

Update the **RF module** and **dongle** to NuPhy's latest firmware (2.1.5) first. Those are closed-source binaries, which you can get from the NuPhy website (Support → Firmware). This repo only replaces the main (QMK) firmware.

## Flashing

1. Download the `.bin` from [Releases](../../releases), or build it yourself (see below).
2. Connect the keyboard over USB.
3. Press `Fn+M+Esc` and keep holding `Esc`. The keyboard reboots into the bootloader.
4. Flash the `.bin` with [QMK Toolbox](https://github.com/qmk/qmk_toolbox), or from the command line:
   ```sh
   dfu-util -a 0 -s 0x08000000:leave -D nuphy_air96_v2_ansi_default.bin
   ```

For remapping, load [`nuphy-air96-v2-via_custom_v4.json`](keyboards/nuphy/air96_v2/ansi/keymaps/nuphy-air96-v2-via_custom_v4.json) in [VIA](https://usevia.app) (Settings → Show Design tab → Load).

## Building

The easiest way is Docker, which needs no local toolchain:

```sh
git clone --recurse-submodules https://github.com/ablasik/nuphy-qmk.git
cd nuphy-qmk
docker run --rm -v "$PWD":/qmk_firmware -w /qmk_firmware ghcr.io/qmk/qmk_cli \
  qmk compile -kb nuphy/air96_v2/ansi -km default
```

Or with the [QMK CLI](https://docs.qmk.fm/newbs_getting_started):

```sh
qmk compile -kb nuphy/air96_v2/ansi -km default
```

## Contributing

Issues and PRs are welcome: bug reports, features, and ports to other NuPhy boards.

- Wireless and sleep bugs are hard to reproduce. Please include your mode (BT/2.4G/USB), your sleep mode (`Fn+]` side LED color), your OS, and steps to reproduce.
- The board code lives in [`keyboards/nuphy/`](keyboards/nuphy). Everything else is upstream QMK, so please don't edit core QMK files.

### Staying on current QMK

This repo keeps full QMK history, and every upstream release is merged in:

```sh
git remote add qmk https://github.com/qmk/qmk_firmware.git   # once
git fetch qmk master
git merge qmk/master
git submodule update --init --recursive
```

When upstream changes an API (as happened with `eeconfig_*_datablock` and `debounce()`), fix the board code in a separate commit right after the merge.

## Credits

- [NuPhy](https://nuphy.com) for the original open-source firmware.
- [adi4086](https://github.com/adi4086/qmk_firmware) for most of the enhancements.
- [jincao1](https://github.com/jincao1) for the reporting and freeze fixes adi4086 built on.
- [QMK](https://github.com/qmk/qmk_firmware) and all its contributors.

## License

GPL-2.0-or-later, same as QMK. See [LICENSE](LICENSE).
