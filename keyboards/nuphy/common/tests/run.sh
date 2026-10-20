#!/usr/bin/env bash
set -eu
cd "$(git rev-parse --show-toplevel)"
out=.build/connection-actions-tests
mkdir -p "$out"
stubs=keyboards/nuphy/common/tests/stubs
backend=quantum/nvm/eeprom/nvm_dynamic_keymap.c
for cols in 17 21; do
    "${CC:-gcc}" -std=gnu11 -Wall -Wextra -Werror -Wno-sign-compare \
        -DVIA_ENABLE -DMATRIX_ROWS=6 -DMATRIX_COLS="$cols" \
        -include "$stubs/eeprom.h" -I"$stubs" -Iquantum/nvm \
        keyboards/nuphy/common/tests/test_connection_actions.c "$backend" \
        -o "$out/actions-$cols.exe"
    "$out/actions-$cols.exe"
done
"${CC:-gcc}" -std=gnu11 -Wall -Wextra -Werror -Wno-sign-compare \
    -DVIA_ENABLE -DMATRIX_ROWS=6 -DMATRIX_COLS=17 \
    -include "$stubs/eeprom.h" -I"$stubs" -Iquantum/nvm \
    keyboards/nuphy/common/tests/test_nvm_default.c "$backend" \
    -o "$out/default.exe"
"$out/default.exe"
"${CC:-gcc}" -std=gnu11 -Wall -Wextra -Werror -fsyntax-only \
    keyboards/nuphy/common/connection_actions.c
echo 'PASS: Connection Actions disabled for non-VIA builds'

python keyboards/nuphy/common/tests/board_switch_tests.py
for board in air60_v2 air75_v2 air96_v2; do
    "${CC:-gcc}" -std=gnu11 -Wall -Wextra -Werror -Wno-sign-compare \
        -DVIA_ENABLE -DMATRIX_ROWS=6 -DMATRIX_COLS=17 \
        -include "$stubs/eeprom.h" -I"$stubs" -Iquantum/nvm \
        "$out/$board-switch.c" "$backend" -o "$out/$board-switch.exe"
    "$out/$board-switch.exe"
done

"${CC:-gcc}" -std=gnu11 -Wall -Wextra -Werror -Wno-sign-compare -Wno-unused-parameter \
    -DVIA_ENABLE -DMATRIX_ROWS=6 -DMATRIX_COLS=17 \
    -include "$stubs/eeprom.h" -I"$stubs" -Iquantum/nvm \
    "$out/usb-os.c" "$backend" -o "$out/usb-os.exe"
"$out/usb-os.exe"
