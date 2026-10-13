# Releasing

Releases are built by GitHub Actions (`.github/workflows/nuphy.yml`) from a tag. Nothing is built or uploaded by hand.

## Versions

`vMAJOR.MINOR.PATCH`, one version for all boards:

- **MAJOR**: flashing it resets the keyboard settings or the VIA keymap (the settings block or the EEPROM layout changed). Say so at the top of the release notes. The VIA keymap survives updates (`via_init_kb` in `user_kb.c`); a release that changes the keymap layout (layer count, custom keycode values) has to reset it there.
- **MINOR**: new features or behaviour changes.
- **PATCH**: fixes only.

`v1.2.0-rc1` and other tags with a dash are published as pre-releases, for testers.

The firmware knows its version: QMK takes it from `git describe`, and it's printed with debug on (`Fn+M+F1`, QMK console).

## Steps

1. **Upstream QMK is merged.** The weekly *Sync upstream QMK* PR is merged or there isn't one waiting.
2. **CI is green** on `nuphy-keyboards`: lint, all boards built, guides rendered.
3. **The guides are current.** Run `python3 docs/nuphy/guide/build.py` and commit if anything changed.
4. **Changelog.** In `CHANGELOG.md`, rename `## [Unreleased]` to `## [1.2.0] - YYYY-MM-DD`, read the entries as a user would, and commit (`Release v1.2.0`).
5. **Tag and push:**
   ```sh
   git tag -a v1.2.0 -m "v1.2.0"
   git push origin nuphy-keyboards v1.2.0
   ```
6. **CI creates a draft release**: the `[1.2.0]` changelog section as the notes (the job fails if it's missing), then for every board the `.bin`, the VIA file and the Quick Guide PDF, flashing instructions and SHA-256 sums.
7. **Test the draft on hardware.** Flash the `.bin` from the draft itself, on at least the boards that changed:
   - typing over USB, Bluetooth and 2.4G;
   - deep sleep (`Fn+M+]` on battery) and waking with a key press;
   - side lights, Caps, battery bar (`Fn+\`);
   - settings survive unplugging (change something, wait 10 s, unplug).

   Boards nobody could test are marked as untested in the notes.
8. **Publish** the draft.

If something is wrong, delete the draft and the tag (`git push --delete origin v1.2.0`), fix, and tag again.

## Changelog entries

Every change users can notice gets a line under `## [Unreleased]` at the top of `CHANGELOG.md`, in the same PR; the first change after a release adds that heading. CI fails a PR that touches `keyboards/nuphy/` without one; for changes nobody would notice (comments, refactoring) add the `skip-changelog` label instead.
