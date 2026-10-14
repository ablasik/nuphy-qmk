# Releasing

A release is one commit that gives `## [Unreleased]` in `CHANGELOG.md` its version. When it reaches `nuphy-keyboards`, GitHub Actions (`nuphy.yml`) builds, tests and tags it and creates a draft release. Nothing is built, tagged or uploaded by hand.

## Versions

`vMAJOR.MINOR.PATCH`, one version for all boards:

- **MAJOR**: flashing it resets the keyboard settings or the VIA keymap (the settings block or the EEPROM layout changed). Say so at the top of the release notes. The VIA keymap survives updates (`via_init_kb` in `user_kb.c`); a release that changes the keymap layout (layer count, custom keycode values) has to reset it there.
- **MINOR**: new features or behaviour changes.
- **PATCH**: fixes only.

`v1.2.0-rc1` and other tags with a dash, pushed by hand (`git tag -a v1.2.0-rc1 -m v1.2.0-rc1 && git push origin v1.2.0-rc1`), become pre-releases for testers, with what's under `[Unreleased]` as the notes.

The firmware knows its version: QMK takes it from `git describe`, and it's printed with debug on (`Fn+M+F1`, QMK console).

## Steps

1. **Upstream QMK is merged.** The weekly *Sync upstream QMK* PR is merged or there isn't one waiting.
2. **CI is green** on `nuphy-keyboards`: lint, all boards built, guides rendered, emulator tests passed.
3. **The guides are current.** Run `python3 docs/nuphy/guide/build.py` and commit if anything changed.
4. **Changelog.** Read the `[Unreleased]` entries as a user would (they become the release notes), pick MAJOR, MINOR or PATCH (above), rename the heading to `## [1.2.0] - YYYY-MM-DD`, commit (`Release v1.2.0`) and push to `nuphy-keyboards`. Pushing straight to the branch needs admin rights; anyone else opens a PR with that commit.
5. **CI tags the commit `v1.2.0` and creates a draft release**: the `[1.2.0]` changelog section as the notes, then for every board the `.bin`, the VIA file and the Quick Guide PDF, flashing instructions and SHA-256 sums.
6. **Test the draft on hardware.** Flash the `.bin` from the draft itself, on at least the boards that changed:
   - typing over USB, Bluetooth and 2.4G;
   - deep sleep (`Fn+M+]` on battery) and waking with a key press;
   - side lights, Caps, battery bar (`Fn+\`);
   - settings survive unplugging (change something, wait 10 s, unplug).

   Boards nobody could test are marked as untested in the notes. CI has already run the Air96 V2 build on an emulated board (`tests/nuphy`, locally: `dotnet run --project tests/nuphy/NuphyTests`, add `NUPHY_SLOW=1` for the real-time timer tests); the hardware check is for what emulation can't see: USB, the real radio, battery.
7. **Publish** the draft.

If something is wrong, delete the draft and the tag (`git push --delete origin v1.2.0`), fix it on `nuphy-keyboards`, and push: CI sees `[1.2.0]` without a tag and releases again. Fixes worth their own line go under `[1.2.0]`, not a new `[Unreleased]`.

## Changelog entries

Every change users can notice gets a line under `## [Unreleased]` at the top of `CHANGELOG.md`, in the same PR; the first change after a release adds that heading. CI fails a PR that touches `keyboards/nuphy/` without one; for changes nobody would notice (comments, refactoring) add the `skip-changelog` label instead.
