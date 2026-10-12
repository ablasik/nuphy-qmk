#!/usr/bin/env python3
# Copyright 2026 nuphy-qmk contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Build the Quick Guide (PNG + PDF) for each NuPhy board.

Everything board-specific comes from the board's own code: key geometry from
keyboard.json, legends from the default keymap, the side LED count from the
driver header. template.html refers to functions, not keys: {{fn LOCK_SCR}}
becomes "Fn + Esc" on a board that has LOCK_SCR on Esc, and a line whose
function a board doesn't have is left out.

    python3 docs/nuphy/guide/build.py              # all boards
    python3 docs/nuphy/guide/build.py air75_v2     # one board
    python3 docs/nuphy/guide/build.py --html-only  # skip PNG/PDF, just the HTML

PNG/PDF need Playwright: pip install playwright && playwright install chromium
"""
import glob
import html
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
OUT = os.path.normpath(os.path.join(HERE, ".."))
BOARDS = json.load(open(os.path.join(HERE, "boards.json")))

# What's printed on the keycaps. Keys that differ between the Mac and Windows
# layers (Opt/Win, Cmd/Alt) get both, the rest the Windows legend (F1, not media).
PRINTED = {
    "KC_ESC": "Esc", "KC_GRV": "`", "KC_MINS": "-", "KC_EQL": "=", "KC_BSPC": "⌫", "KC_TAB": "Tab",
    "KC_LBRC": "[", "KC_RBRC": "]", "KC_BSLS": "\\", "KC_CAPS": "Caps", "KC_SCLN": ";", "KC_QUOT": "'",
    "KC_ENT": "Enter", "KC_LSFT": "Shift", "KC_RSFT": "Shift", "KC_COMM": ",", "KC_DOT": ".", "KC_SLSH": "/",
    "KC_LCTL": "Ctrl", "KC_RCTL": "Ctrl", "KC_SPC": "", "KC_UP": "↑", "KC_DOWN": "↓", "KC_LEFT": "←",
    "KC_RGHT": "→", "KC_DEL": "Del", "KC_INS": "Ins", "KC_HOME": "Home", "KC_END": "End", "KC_PGUP": "PgUp",
    "KC_PGDN": "PgDn", "KC_PSCR": "PrtSc", "KC_NUM": "Num", "KC_PSLS": "/", "KC_PAST": "*", "KC_PMNS": "−",
    "KC_PPLS": "+", "KC_PENT": "Ent", "KC_PDOT": ".", "KC_LALT": "Alt", "KC_RALT": "Alt", "KC_LGUI": "Win",
    "KC_RGUI": "Win",
}
MAC_MOD = {"KC_LALT": "Opt", "KC_RALT": "Opt", "KC_LGUI": "Cmd", "KC_RGUI": "Cmd"}

# Legends for what a key does under Fn (mint) and Fn + M (mustard).
FUNCTION = {
    "LNK_BLE1": "BT 1", "LNK_BLE2": "BT 2", "LNK_BLE3": "BT 3", "LNK_RF": "2.4G", "LNK_USB": "USB",
    "LINK_PREV": "Last device", "LOCK_SCR": "Lock screen", "PRT_SCR": "Screenshot", "PRT_AREA": "Area shot",
    "KC_INS": "Insert", "KC_GRV": "`", "DEV_RESET": "Reset 3 s", "SLEEP_MODE": "Sleep mode", "BAT_SHOW": "Battery bar",
    "RGB_TEST": "LED test", "BAT_NUM": "Battery %", "RM_SPDD": "Speed −", "RM_SPDU": "Speed +",
    "RM_VALU": "Bright +", "RM_VALD": "Bright −", "RM_NEXT": "Effect", "RM_HUEU": "Colour",
    "KC_BRID": "Screen −", "KC_BRIU": "Screen +", "KC_MPRV": "Prev", "KC_MPLY": "Play", "KC_MNXT": "Next",
    "KC_MUTE": "Mute", "KC_VOLD": "Vol −", "KC_VOLU": "Vol +", "TASK": "Mission", "SEARCH": "Search",
    "MAC_VOICE": "Dictation", "MAC_DND": "Focus", "KC_CALC": "Calc",
    "QK_REBOOT": "Reboot", "DB_TOGG": "Debug", "RF_DFU": "RF delay", "DEBOUNCE_D": "Debounce −",
    "DEBOUNCE_I": "Debounce +", "SLEEP_D": "Sleep −", "SLEEP_I": "Sleep +", "MACWIN_TG": "Mac/Win",
    "SIDE_1": "One side", "SLEEP_NOW": "Sleep now", "CAPS_WORD": "Caps Word", "DEBOUNCE_T": "Debounce",
    "FROW_SWAP": "F-keys ⇄", "GAME_MODE": "Game", "SOCD_TOG": "Snap tap", "NUMLOCK_IND": "Num light",
    "SIDE_SPD": "Side spd −", "SIDE_SPI": "Side spd +", "SIDE_VAI": "Side +", "SIDE_VAD": "Side −",
    "SIDE_MOD": "Side mode", "SIDE_HUI": "Side colour", "WIN_LOCK": "Win lock",
}
FUNCTION.update({f"KC_F{i}": f"F{i}" for i in range(1, 13)})


def printed(kc):
    if kc in PRINTED:
        return PRINTED[kc]
    m = re.fullmatch(r"KC_P?(\w)", kc)  # letters, digits, numpad digits
    if m:
        return m.group(1)
    m = re.fullmatch(r"KC_(F\d+)", kc)
    return m.group(1) if m else kc


def parse_keymap(path):
    """{layer name: [keycode, ...]} in LAYOUT argument order."""
    src = open(path).read()
    layers = {}
    for m in re.finditer(r"\[(\w+)\]\s*=\s*LAYOUT\w*\(", src):
        depth, i, start = 1, m.end(), m.end()
        while depth:
            depth += {"(": 1, ")": -1}.get(src[i], 0)
            i += 1
        body = re.sub(r"//[^\n]*", "", src[start : i - 1])
        args, depth, cur = [], 0, ""
        for ch in body:
            if ch == "," and depth == 0:
                args.append(cur.strip())
                cur = ""
                continue
            depth += {"(": 1, ")": -1}.get(ch, 0)
            cur += ch
        args.append(cur.strip())
        layers[m.group(1)] = [a for a in args if a]
    return layers


def board_data(name):
    d = os.path.join(ROOT, "keyboards", "nuphy", name, "ansi")
    kb = json.load(open(os.path.join(d, "keyboard.json")))
    lay = next(iter(kb["layouts"].values()))["layout"]
    layers = parse_keymap(os.path.join(d, "keymaps", "default", "keymap.c"))
    order = list(layers)  # enum order == definition order in these keymaps
    for ly in layers.values():
        assert len(ly) == len(lay), f"{name}: keymap/layout size mismatch"
    side = None
    for h in glob.glob(os.path.join(d, "*.h")):
        m = re.search(r"#define\s+SIDE_LINE\s+(\d+)", open(h).read())
        side = int(m.group(1)) if m else side
    via = sorted(glob.glob(os.path.join(d, "keymaps", "*.json")))
    return kb, lay, layers, order, side, os.path.basename(via[0]) if via else ""


def build(name, cfg):
    kb, lay, layers, order, side_n, via = board_data(name)
    mac, win, fn, m = layers["MAC_BASE"], layers["WIN_BASE"], layers["MAC_FN"], layers["M_LAYER"]
    m_idx = order.index("M_LAYER")

    labels = []
    for a, b in zip(mac, win):
        if a != b and a in MAC_MOD and b in PRINTED:
            labels.append(f"{MAC_MOD[a]} {printed(b)}")
        else:
            labels.append(printed(b))

    def fn_legend(i):
        kc = fn[i]
        if kc == f"MO({m_idx})":
            return "M layer"
        lab = FUNCTION.get(kc)
        return lab if lab and lab != labels[i] else None

    def m_legend(i):
        kc = m[i]
        if kc in ("_______", "KC_TRNS", "KC_NO"):
            # the left GUI key locks itself on the M layer (ansi.c, KC_LGUI)
            if mac[i] == "KC_LGUI":
                return "⌘ lock"
            if win[i] == "KC_LGUI":
                return "Win lock"
            return None
        return FUNCTION.get(kc, kc if not kc.startswith("MO(") else None)

    accent = {kc: cls for cls, kcs in cfg.get("accents", {}).items() for kc in kcs}
    width = max(k["x"] + k.get("w", 1) for k in lay)
    height = max(k["y"] + k.get("h", 1) for k in lay)
    unit = 100 / width
    keys = []
    for i, k in enumerate(lay):
        x, y, w, h = k["x"], k["y"], k.get("w", 1), k.get("h", 1)
        name_ = "Fn" if mac[i].startswith("MO(") else labels[i]
        lf, lm = fn_legend(i), m_legend(i)
        cls = ["key"]
        if lf or lm:
            cls.append("has")
        if name_ == "Fn":
            cls.append("fnkey")
        if win[i] == "KC_M":
            cls.append("mkey")
        if win[i] in accent:
            cls.append(accent[win[i]])
        if len(name_) > 1 and not re.fullmatch(r"F\d+", name_):
            cls.append("word")
        sub = (f'<span class="lf">{html.escape(lf)}</span>' if lf else "") + (f'<span class="lm">{html.escape(lm)}</span>' if lm else "")
        keys.append(
            f'<div class="{" ".join(cls)}" style="left:{x * unit:.3f}%;top:{y / height * 100:.3f}%;'
            f'width:{w * unit:.3f}%;height:{h / height * 100:.3f}%">'
            f'<span class="cap"><b>{html.escape(name_).replace(" ", " · ")}</b>{sub}</span></div>'
        )

    flags = {
        "numpad": "KC_P1" in win,
        "frow": "KC_F1" in win,
        "fnshift_m": fn[win.index("KC_LSFT")] == f"MO({m_idx})",
    }

    def chip(layer, kc):
        ly = {"fn": fn, "m": m}[layer]
        if kc not in ly:
            raise KeyError(kc)
        return f'<span class="k">{html.escape(labels[ly.index(kc)])}</span>'

    FN = '<span class="k fn">Fn</span><span class="plus">+</span>'
    M = '<span class="k m">M</span><span class="plus">+</span>'

    def strip(spec, n):
        cells = spec.split()
        fill = [c for c in cells if c.endswith("*")]
        if fill:
            at = cells.index(fill[0])
            cells[at : at + 1] = [fill[0][:-1]] * (n - len(cells) + 1)
        return '<span class="strip">' + "".join(f'<i class="{c.replace(".", " ")}"></i>' if c != "_" else "<i></i>" for c in cells) + "</span>"

    page = open(os.path.join(HERE, "template.html")).read()
    page = re.sub(r"<!--if (!?)(\w+)-->(.*?)<!--endif-->", lambda mm: mm.group(3) if flags[mm.group(2)] != bool(mm.group(1)) else "", page, flags=re.S)
    page = page.replace("{{BOARD}}", "\n".join(keys))
    page = page.replace("{{TITLE}}", html.escape(kb["keyboard_name"]))
    page = page.replace("{{VIA}}", html.escape(via))
    page = page.replace("{{SIDE_N}}", str(side_n)).replace("{{ASPECT}}", f"{width} / {height + 0.1}").replace("{{ROWS}}", f"{height + 0.1}")
    page = page.replace("{{SIDE_LEFT}}", strip("c fx*".replace("fx", "_"), side_n)[len('<span class="strip">') : -len("</span>")])
    right = "w _* g g g" if flags["numpad"] else "_* g g g"
    page = page.replace("{{SIDE_RIGHT}}", strip(right, side_n)[len('<span class="strip">') : -len("</span>")])
    page = re.sub(r"\{\{STRIP ([^}]*)\}\}", lambda mm: strip(mm.group(1), side_n), page)

    out_lines = []
    for line in page.split("\n"):
        try:
            line = re.sub(r"\{\{fn (\w+)\}\}", lambda mm: FN + chip("fn", mm.group(1)), line)
            line = re.sub(r"\{\{fnm (\w+)\}\}", lambda mm: FN + M + chip("m", mm.group(1)), line)
            line = re.sub(r"\{\{key (fn|m):(\w+)\}\}", lambda mm: chip(mm.group(1), mm.group(2)), line)
        except KeyError:
            continue  # this board doesn't have the function: leave the line out
        out_lines.append(line)
    return "\n".join(out_lines)


def render(html_path, png, pdf):
    from playwright.sync_api import sync_playwright

    with sync_playwright() as p:
        browser = p.chromium.launch()
        page = browser.new_page(viewport={"width": 1240, "height": 900}, device_scale_factor=2)
        page.goto("file://" + html_path, wait_until="networkidle")
        page.evaluate("document.fonts.ready")
        page.screenshot(path=png, full_page=True)
        h = page.evaluate("document.documentElement.scrollHeight")
        page.pdf(path=pdf, width="1240px", height=f"{h + 2}px", print_background=True)
        browser.close()


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    for name in args or BOARDS:
        page = build(name, BOARDS[name])
        base = os.path.join(OUT, f"{name}_quick_guide")
        if "--html-only" in sys.argv:
            open(base + ".html", "w").write(page)
            print("wrote", base + ".html")
            continue
        tmp = os.path.join(HERE, f".{name}.html")  # rendered from a file so relative paths and fonts load
        open(tmp, "w").write(page)
        try:
            render(tmp, base + ".png", base + ".pdf")
        finally:
            os.remove(tmp)
        print("wrote", base + ".png", base + ".pdf")


if __name__ == "__main__":
    main()
