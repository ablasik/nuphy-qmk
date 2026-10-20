# Connection Actions (Air60 V2, Air75 V2, Air96 V2)

Connection Actions runs an existing VIA macro after the selected connection
becomes ready, or on the current host before an intentional connection switch. Configure four independent rules in ordinary VIA; all start disabled.
There is no host agent, monitor integration, or change to the Mac/Win layout controls.

## Configure in VIA

1. Build/flash the board's `default` keymap, which already enables VIA:
   `qmk compile -kb nuphy/air75_v2/ansi -km default` (substitute air60_v2/air96_v2).
2. In VIA enable **Show Design tab** in Settings. In Design load the existing
   board definition under `ansi/keymaps/nuphy-air*-v2-via_custom_v4.json` from this
   change. Use the V3 definition path, despite the historical `v4` filename.
3. Open the normal **Macros** editor and create M0, for example a shortcut that
   an already installed host utility handles. Connection Actions does not contain
   shell commands or configure that utility.
4. Open **Connection Actions**. Check **Storage and OS**. In **Rule 1**, set
   **When: After connection / Ready**, **Host OS: Windows (USB detected)**,
   **Connection: USB**, **Macro: M0**, then **Enabled**.
   Delay displays seconds explicitly (0 to 10 s in 0.1 s steps). Changes save automatically, but never launch a
   newly edited rule in an already evaluated session. Reconnect to test it.

The menu uses standard toggle, dropdown, range and showIf controls on custom
channel 0. It does not require a VIA fork. Storage and Host OS are read-only text labels. The status label reports the
actual detected USB OS, or Unknown (USB/wireless). Numeric GET IDs 0/29 remain
for compatibility; a hidden storage-state binding preserves rule visibility. Physical VIA loading with a keyboard still requires
the hardware checks below. Definitions are not published automatically upstream.

## Before or after switching

Each rule's **When** dropdown has two values:

- **After connection / Ready** (wire value 0, existing default): execute on the NEW
  host after readiness and the 500 ms settle window. OS/profile conditions match
  the new host, and Delay is added after settling.
- **Before switching away (current host)** (wire value 1): execute on the OLD host
  before changing the host driver, RF channel or Mac/Win layer. OS/profile
  conditions match the CURRENT/source host. Delay starts with the stable switch
  request, without an additional 500 ms. It does not describe the destination.

Example for monitor software already configured on the computers:

| Rule | When | Host OS | Connection | Macro does |
| --- | --- | --- | --- | --- |
| 1 | Before switching away | Windows | USB | Source Windows switches monitors to Mac |
| 2 | Before switching away | Any | Bluetooth 1 (the Mac profile) | Source Mac switches monitors to Windows |

Use your own macro IDs. Disable obsolete After rules for these same shortcuts.
Use a short delay (0 initially); the Delay field delays macro START, not the switch
following it. The keyboard does not wait for the monitor utility to finish.

The main task latches dial and profile-key requests, holds the source transport,
executes matching rules in slot order, releases all generated keys, waits for the
RF queue/dequeue interval to empty, then allows 100 ms of host processing time
before the switch. Fn/profile key releases let the request become idle; a new
physical press cancels remaining playback but still permits switching. Reversing
the dial cancels the pending request. A dead/unready source skips the before rule;
a source lost during playback cancels it, never redirects it to the destination.
Each request has a 15 second total deadline (including delay, idle wait, macro and
queue drain), after which the switch proceeds. Keep shortcuts short. This deadline
is a best-effort escape, not a delivery acknowledgment.

No Before event runs at boot, on unplugging the USB cable, on an unsolicited
connection loss, on an RF reset, or merely when reselecting the same profile.
Supported intentional requests are the Wired/Wireless dial, Fn profile selection,
previous-profile selection and pairing to another profile. Startup dial scanning
does not replay a departure action. External RF configuration synchronization is
not an intentional local departure event.

The hardware must keep the source HID link alive during a dial move. If a switch
physically disconnects the old link, firmware cannot send a last shortcut over
that missing link. Actual USB/BT/2.4G delivery and monitor automation must be
verified on the keyboard; native tests do not establish hardware behavior.

Existing rules remain After rules with the same wire IDs, EEPROM schema, tail
size, macro buffer addresses and macro contents. An older firmware rejects rules
containing the new trigger value and loads disabled defaults; export rules before
downgrading. There is no factory reset or forced EEPROM erase.

## OS and readiness

USB uses QMK OS Detection, with a 500 ms settling window after configured HID and
the USB host driver are active. Conditions are snapshotted once after that window.
Later guesses do not replay rules. OS_UNSURE is Unknown, never Windows. QMK's
fingerprinting is heuristic: docks/KVMs and unusual hosts can affect its result.
The USB fingerprint is cleared at the actual ChibiOS USB reset ISR, before
new descriptor callbacks. Queued INIT/protocol notifications do not clear it:
they may run after the new fingerprint has already arrived. The three NuPhy
boards opt in with OS_DETECTION_RESET_ON_USB_BUS_RESET; other boards are unchanged. The existing Mac/Win detection callback and user hooks remain.

Bluetooth and 2.4 GHz always use **Unknown**. **Any** and **Unknown** can match them;
Windows/macOS/Linux/iOS conditions cannot. A saved Win/Mac layout is not an OS
identity and is not consulted. Storage and OS displays the active context's source.

Wireless Ready requires a matching-profile CMD_RF_STS_SYSC response with
RF_CONNECT in a complete status payload (not a short ACK), the RF host driver, completed dial initialization and awake firmware.
The status must be less than 650 ms old (normal polling is 200 ms). This is the
RF module's connection indication, not an end-to-end HID acknowledgment.
Selecting a profile alone cannot trigger a rule. Hardware confirmation is pending.

A session is a continuous observable ready interval for one active profile.
A different active profile, RF module reset, USB configuration loss/suspend,
or a reported RF loss ends that interval and cancels pending actions.
The next confirmed interval can fire again. Wake alone, reselecting the current
profile, local LINKING assignments, LED sleep and RF sleep/sniff/error states
do not create an event.
A stale status pauses output and cancels pending work while retaining deduplication;
only a subsequent explicit loss/reset/profile change permits replay. There are no
separate Wake/Reconnected/Disconnected triggers and no claim to distinguish a
short RF drop from a new host session. Repeated connected responses and layer or
Mac/Win changes do not create sessions. If a transport hides a disconnect from
the firmware, no new event can be inferred reliably.

Delay is 0–100 in 100 ms units (0–10 seconds), added after the 500 ms settling
window. Simultaneously due rules run in slot order, one macro at a time. An
earlier rule with a longer delay does not block a later rule that is already due.

## Macro execution

This revision has 16 VIA macros, M0–M15. A build-time guard requires updating the
JSON if that count changes. The player reads the normal dynamic macro buffer
through QMK's APIs. It supports its ASCII text and send_string tap/down/up/delay
encoding; malformed streams, invalid upload sentinels, and delays above 65535 ms
are rejected before emitting any report. This is VIA macros, not QMK's separate
on-keyboard recording feature called Dynamic Macros.

QMK's existing dynamic_keymap_macro_send() is synchronous. It blocks observation
during delays and can overrun NuPhy's RF queue. Automatic actions therefore use
a bounded, cooperative decoder of the same format. Each HID change is at least
20 ms apart; RF output waits for the queue and dequeue delay to clear. Long
macros run more slowly than manually invoked macros. Manual macro playback is
unchanged. ASCII uses QMK's current LUTs, as does send_string; it is not Unicode
text input and assumes the corresponding host keyboard layout.

Playback starts only while physical keys, modifiers, and the keyboard report are
idle. Physical input takes priority: it cancels the running macro and remaining
pending rules before normal key processing. Editing rules or uploading/resetting
macros also cancels playback. Completion/cancellation clears generated keyboard,
consumer/system, and mouse reports through the board's existing break_all_key(). RF receive callbacks defer these
releases until the receive frame has been fully processed, avoiding UART reentry.
An unmatched user-authored down is thus released at completion. An absent host
cannot acknowledge a release; physical tests must confirm reconnect behavior.

## Storage and migration

The existing 4096-byte logical EEPROM, 8192-byte wear-leveling backing, 32-byte
user_config, layer count, keymap/macro starting addresses, and link_valid version
are unchanged. A marker uses only user_config.reserved[4]. A versioned 64-byte
tail stores the rules, CRC16 and commit signature. No reset is introduced.

On first boot, reservation happens only if the old VIA upload sentinel is valid,
all 16 macro terminators fit before the new sentinel, and the rest of the old
buffer is zero padding. Existing mappings, macro content, RGB, profile settings,
and all other config fields stay at their original addresses. The marker is
written before the tail; a partial rule save results in disabled defaults.
Unknown schemas, bad CRCs or out-of-range stored fields disable every rule.
Editing a rule explicitly replaces the invalid record with disabled defaults
plus the edited field; enable a rule last.

**Unavailable** means the conservative reservation check failed. The complete
old macro capacity remains available; no bytes are overwritten. Export your
VIA configuration, shorten/re-save macros in ordinary VIA, and reboot. Merely
deleting visible text may leave nonzero padding in some clients. This fallback
was explicitly agreed for the MVP; Connection Actions is unavailable until the
check succeeds. It also refuses an interrupted macro upload.

Once reserved, VIA reports 64 fewer macro bytes. All macro get/set/reset paths
share that runtime boundary, so a stale VIA client cannot overwrite the rules.
The final physical EEPROM byte remains zero, preserving the old firmware's
macro-valid sentinel. Downgrading to the original revision keeps M0–M15 readable;
editing macros in that older firmware may overwrite Connection Actions, which
then loads disabled on upgrade. This is not a promise for unrelated firmware
with another EEPROM layout. Existing explicit factory reset still resets data.

## Implementation and checks

The common module contains the observer, host context, rule scheduler, cooperative
macro player and VIA/persistence handler. Small adapters observe the three
boards' actual RF receive, switch, reset, sleep, dial and housekeeping paths.
Two default-preserving QMK hooks permit the runtime EEPROM limit and cancellation
before dynamic macro mutations. Non-VIA builds compile no Connection Actions.

Native regression tests compile the production common module and production
EEPROM backend, replacing only hardware/QMK dependencies. Run from repository root:

```sh
bash keyboards/nuphy/common/tests/run.sh
```

The runner tests both matrix widths (17 and 21 columns), the default core hook,
and compilation without VIA. These tests cover migration byte preservation, full/interrupted old macro buffers,
read/write/reset boundaries, interrupted writes, corrupt config, field validation,
USB OS variants/Unknown, RF profile matching, repeated Ready, reconnect, pending
cancellation, input/upload cancellation, receive-path deferral (no recursive UART sends),
backpressure, multiple rules, malformed
macros, ASCII, unbalanced downs, and timer wraparound. They do not emulate the
physical wear-leveling flash journal or establish radio reliability.

### Required hardware acceptance checklist

- Build Air75/60/96 `default`; record flash/RAM sizes and compiler version.
- USB Windows/macOS/Linux/iOS/unsure: actual OS match, cold plug, bus reset,
  powered KVM host switch, suspend/resume, and no duplicate from later guesses.
- BT1/2/3 and 2.4G: Unknown/Any only; exact profile and BT Any; late replies from
  the old profile; RF status loss, brief drop, wake and reconnect. Record what
  RF_CONNECT actually guarantees on each RF firmware version.
- Switch USB→BT→USB and BT1→BT2→BT1 while an action is delayed or a modifier is
  held by a macro. Verify cancellation, final releases, and no old queue replay.
- Standard VIA: load every JSON; set/get all fields, save/reboot; invalid values;
  disabled rules; M0/M15; no-space recovery; invalid schema recovery; disconnect
  during macro upload. Verify controls render and showIf works on the live board.
- Run long text/chords/delays with physical key interruption and concurrent rules;
  verify no queue overflow, omitted presses, repeated text, or stuck modifiers.
- Before/after flash compare exported keymaps/macros and all existing settings;
  test maximum macro usage, factory reset and downgrade/upgrade on a spare board.
- Power-cut migration/settings saves on a real board. Never infer physical
  flash-transaction safety solely from the byte-array tests.

No Bluetooth/radio or physical VIA result may be claimed without that equipment.

References: [VIA Custom UI](https://caniusevia.com/docs/custom_ui/),
[VIA definitions](https://github.com/the-via/reader), and the QMK APIs in this
repository's quantum/dynamic_keymap and quantum/nvm directories.

UI update: Action is hidden because VIA Macro is the only action. Its stored field
remains zero for compatibility. Delay uses a dropdown with explicit seconds and
all 101 original wire values; this definition-only update needs no firmware flash.

## Text status protocol

GET channel 0 / value 30 returns a null-terminated UTF-8 storage summary; value
31 returns the active host's detected OS and source. The three command bytes are
preserved and the response tail is zero padded within the supplied packet length.
Labels never send SET/SAVE. Numeric IDs 0/29 keep their previous meanings so old
menus remain usable. Rules, macros, EEPROM size and offsets are unchanged.
A matching new BIN and JSON are required for the new text fields. OS detection
still uses QMK's heuristic USB descriptor sequence, never the Win/Mac dial or a
fallback to Windows; wireless stays Unknown. Hubs/KVMs and unrecognised descriptor
sequences can still produce Unknown after this fix.
