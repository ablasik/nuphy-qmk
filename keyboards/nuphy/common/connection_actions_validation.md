# Connection Actions validation

Updated 2026-10-20; PR base is `da1f866e6be335af995725bf87539ad415328754`.
The original builds used `fc7159feadc8acd5888139d5e2e6df5b86f8a16a`; the later
upstream merge only changed documentation and its generator dependency.

User-reported checks on Air75 V2: manual VIA macro playback, a USB After/Ready
rule, and a working Windows/Mac monitor-switching workflow using Before rules
with Host OS set to Any. These are reports from the keyboard owner, not an
instrumented test of every transport. The owner subsequently confirmed the latest text-label UI, Windows (USB detected)
status, and a Before-switch rule filtered to Windows over USB. Air60/Air96 physical testing,
2.4 GHz, profile/reconnect/wake stress, and actual flash-journal power cuts remain
unverified.

## Firmware builds

QMK CLI 1.2.0; xPack GNU Arm Embedded GCC 14.2.1 20241119 (package 14.2.1-1.1).
Pinned ChibiOS, ChibiOS-Contrib, LUFA and printf submodules were initialized.
All three `default` keymaps compiled, linked and produced BIN/HEX successfully.

| Target | Result | Flash bytes (ELF text + data) | Remaining linker heap |
| --- | --- | ---: | ---: |
| Air75 V2, original base | PASS | 67404 | 1904 |
| Air75 V2, Connection Actions | PASS | 71536 | 1776 |
| Air60 V2, Connection Actions | PASS | 72386 | 1680 |
| Air96 V2, Connection Actions | PASS | 73422 | 1384 |

BIN files also contain the DFU suffix and alignment padding; their file sizes
are 71552 (Air75), 72404 (Air60), and 73440 (Air96) bytes in the latest local builds.

Air75 adds 4132 bytes of flash and 128 bytes of static RAM with this compiler.
Heap figures are `__heap_end__ - __heap_base__`; they are not measured runtime
stack high-water marks. The ELF bss total includes the linker-reserved free heap,
so its unchanged total does not mean that the feature uses no RAM. Existing
1024-byte main and 2048-byte process stacks remain unchanged. All binaries fit
below the final 8 KiB flash area used by wear leveling.

Representative build command (repeat with air60_v2 and air96_v2):

```sh
qmk compile -kb nuphy/air75_v2/ansi -km default -j4 \
  -e SKIP_GIT=yes -e TOOLCHAIN=/path/to/xpack/bin/arm-none-eabi-
```

`SKIP_GIT=yes` avoided automatic downloads of unrelated unused submodules. The
final GCC 14 builds use the repository's normal linker scripts and no firmware
source workarounds. Initial GCC 16.2/binutils 2.47 builds encountered a Windows
nested-linker-script error even on the original base; that toolchain was not
used for the delivered binaries. A temporary flattened script diagnosed it but
is not part of the change or required by the final builds.

## Automated checks

- `bash keyboards/nuphy/common/tests/run.sh`: PASS for 17-column and 21-column
  matrices, default core storage behavior, and the non-VIA module guard.
- Tests compile the production engine and production EEPROM backend, using mocks
  for hardware and other QMK facilities. They cover conservative migration and
  byte preservation, full/interrupted legacy macro buffers, every byte boundary
  of initial reservation and rule saving, invalid values and corrupt records,
  USB OS variants, RF profiles, deduplication, cancellation, UART callback
  deferral, backpressure, modifier cleanup, multiple rules and timer wraparound.
- All three VIA JSONs pass `keyboardDefinitionV3ToVIADefinitionV3()` from the
  official `@the-via/reader` 1.14.4. The package emits its own schema strict-tuple
  warnings but accepts the definitions. This does not establish live WebHID
  communication or visual rendering on a connected board.
- Whitespace/diff checks: PASS.

The byte-array power-cut tests do not emulate the physical flash journal. RF
Ready is inferred from matching-profile full module status reports, not from
end-to-end HID acknowledgments. See the hardware acceptance checklist in
[connection_actions.md](connection_actions.md) before declaring hardware acceptance.

## Compatibility decision

The conservative no-space fallback retains the full legacy macro buffer and
leaves Connection Actions unavailable until macros are shortened and the board
reboots. Otherwise reserve only unused zero padding;
never force a keymap/settings reset. Existing settings addresses, 8 keymap layers,
16 macro IDs and the Mac/Win profile logic remain unchanged. Downgrade behavior
and limitations are documented in the user guide.


## Before-switch extension (2026-10-20)

Native production-engine tests cover source/destination routing, all five source
profiles, same-profile no-op, Fn release, dial reversal, source USB/RF loss,
15-second timeout, final RF queue backpressure and persistence of trigger 1.
A generated fixture compiles the actual request latch, switch_dev_link and
physical dial scan functions from each of the three boards against the engine.
It checks that no mode/UART command or channel/driver change occurs before the
source macro drains, for dial switching, Fn profile selection and pairing.
The official VIA reader validates all three definitions.

Hardware acceptance is still required: Wired->BT, BT->Wired, BT1->BT2, 2.4G,
both directions with an actual host shortcut, fast reversal, Fn release, source
loss during delay, and a macro timeout. Check no stuck modifiers and no action
on the destination from a source-only rule. Confirm old After rules still run
on the destination once after Ready. No USB cable removal event can run Before.


## Text labels and USB OS detection fix (2026-10-20)

The firmware formerly erased the fingerprint on every queued INIT notification.
ChibiOS collects descriptor requests in interrupt context but drains Reset in the
main task. A delayed Reset/INIT could therefore erase an already collected new
Windows fingerprint. With the new per-board opt-in, erase_wlength_data() runs at
the actual USB_EVENT_RESET ISR and the delayed CA notification only invalidates
the action session, not the fingerprint.

A generated native fixture compiles the production ChibiOS event queue/callback,
production quantum/os_detection.c and the CA notification hook together. It feeds
known Windows/macOS/Linux/iOS request sequences between ISR Reset and queue drain,
then verifies that both the detected/reported OS survive and a Windows-filtered
Before rule plays. A new Reset clears the previous host immediately. An unknown
sequence remains Unknown. This reproduces the event order; it does not claim a
USB capture from the user's physical machine.

Text status GET packets are tested with actual OS results, storage states,
wireless Unknown, preserved headers, null termination and bounded short packets.
Native before/after, production adapter, EEPROM and non-VIA tests remain required.
All three JSON definitions must pass the official VIA reader. On hardware verify
Storage/Host OS appear as plain text, delay shows units, macros/rules survive,
and reconnect USB before checking OS after flashing. Retain Any until the USB OS
is visibly detected and the Windows-filtered rule has been verified.


## PR preparation checks

- `qmk lint -kb nuphy/<board>/ansi -km default`: PASS for all three boards.
- Latest upstream `nuphy-keyboards` merged without conflicts; no firmware changes
  arrived upstream between the original base and this PR base.
- A dedicated CI job runs the native Connection Actions tests; no shortcut
  bindings or guide template data changed, so Quick Guide regeneration is not
  needed for this feature.
- Existing Renode suite was not run locally: this host has no .NET SDK or Docker.
  The repository's existing emulator CI job remains the required check.
- The owner confirmed the latest USB OS fix and text labels on Air75 V2: Windows
  is detected over USB and a Windows-filtered Before rule runs successfully.

Hardware follow-up (2026-10-20): screenshots show Storage=Ready and Host OS=Windows
(USB detected), with native text labels and explicit delay units. The owner reports
successful execution of Before / Windows / USB / M0 with no delay. This confirms
that specific configuration; other USB hosts, wireless OS detection and the full
transport/reconnect matrix are not established by this check.
