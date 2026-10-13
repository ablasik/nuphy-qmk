# Changelog

All notable changes to the nuphy-qmk firmware. The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and versions follow the rules in [`docs/nuphy/RELEASING.md`](docs/nuphy/RELEASING.md) (MAJOR means flashing it resets your settings).

Write entries for people who type on the keyboard: what changes for them, not how the code changed. Note which boards an entry applies to when it isn't all of them.

## [1.1.1] - 2026-10-13

VIA remaps are kept from now on, starting with this update.

### Fixed

- VIA remaps are kept when you flash an update. QMK reset them to the default keymap with every new build.
- `Fn+M+F1` shows whether debug is on: the key flashes green when on, red when off.

## [1.1.0] - 2026-10-13

Your settings are kept from now on, including when you update. VIA remaps go back to the default keymap once more: QMK resets them for every new build, which a later version will fix.

### Added

- `Fn+M+A` keeps the computer awake: no idle sleep or screen lock, remembered per connection. The key flashes green when on, red when off. While it's on, the keyboard doesn't sleep on that connection either, so it uses more battery.

### Fixed

- Setting changes (sleep mode, debounce, snap tap) showed their side-light confirmation only about half the time.
- `Fn+M+Esc` restarts the keyboard; hold it 3 s (the keys flash white) for the bootloader. It used to land in the bootloader almost every time.
- Flashing an update no longer resets your settings: holding Esc to enter the bootloader used to wipe them too.
- A setting changed less than 10 s before a restart (`Fn+M+Esc`) is no longer lost.

## [1.0.0] - 2026-10-12

First release of the community firmware, compared with [adi4086 v1.1.7.0](https://github.com/adi4086/qmk_firmware/releases/tag/v1.1.7.0). Flashing it resets the keyboard settings and the VIA keymap once.

### Added

- Air75 V2 and Air60 V2 run the same firmware and features as the Air96 V2 (not yet tested on these boards).
- Mac/Win is remembered per connection (USB, BT 1–3, 2.4G). `Fn+M+W` flips it without the switch.
- On the cable the layout follows the detected OS.
- `Fn+M+F` swaps F1–F12 and the media keys, remembered per connection (Air96, Air75).
- `Fn+Tab` switches back to the previous wireless device; `Fn+Esc` locks the computer (Air96, Air75; on the Air60 via VIA).
- Holding Fn lights every key that does something on the Fn layer, Fn+M the M-layer keys, at full brightness even with the backlight off.
- Clearer side lights: Caps, Num Lock, Win lock and game mode on their own LEDs; battery gauge, connection and settings changes shown briefly.
- On battery the backlight runs at half brightness and fades after 30 s without typing.
- The two top side-light levels go up to full brightness (stock stopped at 160/255; Air96, Air60).
- A Quick Guide for every board, as an image and a PDF.

### Changed

- Rebuilt on current upstream QMK.
- Settings are saved 10 s after the last change (was 30 s).
- Fn+Shift works like Fn+M (Air96, Air75).
- Win `Fn+PrtSc` opens the Snipping Tool.
- The side lights default to NuPhy's stock brightness.
- The Air60 V2 has its own USB product ID, so VIA no longer mixes it up with the Air96 V2.

### Fixed

- Fewer stuck or dropped keys over Bluetooth and 2.4G; RF packets are checked like NuPhy's firmware 2.1.x does.
- Faster reconnect after waking; the first key after deep sleep is no longer lost.
- The keyboard no longer wakes a sleeping Mac.
- All NKRO keys work wirelessly; no more stuck volume keys or lost key releases when switching connections.
- A key press wakes a sleeping computer over USB; no deep sleep (and USB disconnect) on the cable.
- The VIA effect list matches the firmware.
- The connection indicator lights the key that really holds BT 1–3 / 2.4G (Fn+Q…R on the Air60).

### Removed

- Old, unmaintained code: copies of the LED drivers, ST's legacy peripheral library and the legacy EEPROM emulation are replaced by their current QMK and ChibiOS equivalents.
