#!/usr/bin/env python3
"""Generates the Aurora Trigger UI kit as SVG (vector). Render to PNG with render_png.js."""
import os, sys
OUT = sys.argv[1] if len(sys.argv) > 1 else "svg"
os.makedirs(OUT, exist_ok=True)
_uid = [0]
def uid(p="g"):
    _uid[0] += 1
    return f"{p}{_uid[0]}"

def svg(w, h, body, defs=""):
    return (f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" viewBox="0 0 {w} {h}">'
            f'<defs>{defs}</defs>{body}</svg>')

def lin(id_, stops, x2=0, y2=1):
    s = "".join(f'<stop offset="{o}" stop-color="{c}"' + (f' stop-opacity="{a}"' if a is not None else "") + "/>"
                for o, c, a in stops)
    return f'<linearGradient id="{id_}" x1="0" y1="0" x2="{x2}" y2="{y2}">{s}</linearGradient>'

def shade(hexc, f):
    hexc = hexc.lstrip("#"); r, g, b = (int(hexc[i:i+2], 16) for i in (0, 2, 4))
    if f >= 0: r, g, b = (int(c + (255 - c) * f) for c in (r, g, b))
    else:      r, g, b = (int(c * (1 + f)) for c in (r, g, b))
    return f"#{r:02x}{g:02x}{b:02x}"

M = 4  # margin for shadows

def button(name, w, h, base, glow=None, pressed=False):
    g = uid(); r = min(5, h * 0.28)
    top, bot = shade(base, 0.28), shade(base, -0.24)
    if pressed: top, bot = shade(base, -0.05), shade(base, -0.38)
    defs = lin(g, [(0, top, None), (1, bot, None)])
    b = (f'<rect x="{M}" y="{M+1.2}" width="{w}" height="{h}" rx="{r}" fill="#000" opacity=".35"/>'
         f'<rect x="{M}" y="{M}" width="{w}" height="{h}" rx="{r}" fill="url(#{g})"/>'
         f'<rect x="{M+1}" y="{M+1}" width="{w-2}" height="{h-2}" rx="{r-1}" fill="none" stroke="#fff" stroke-opacity="{.1 if pressed else .45}"/>'
         f'<rect x="{M}" y="{M}" width="{w}" height="{h}" rx="{r}" fill="none" stroke="#23262c" stroke-opacity=".95" stroke-width="1.1"/>')
    if glow:
        b += f'<rect x="{M}" y="{M}" width="{w}" height="{h}" rx="{r}" fill="none" stroke="{glow}" stroke-opacity=".38" stroke-width="2.4"/>'
    write(name, svg(w + 2*M, h + 2*M + 2, b, defs))

def knob(name, color, size=64, pointer=True):
    r = size / 2 - 2; c = size / 2 + M
    rr = r - 1.3; br = r * .70
    ring, body, gloss = uid("r"), uid("b"), uid("l")
    defs = (lin(ring, [(0, "#f3f5f8", None), (1, "#5f656f", None)]) +
            f'<radialGradient id="{body}" cx=".38" cy=".30" r=".95"><stop offset="0" stop-color="{shade(color,.65)}"/>'
            f'<stop offset=".55" stop-color="{color}"/><stop offset="1" stop-color="{shade(color,-.6)}"/></radialGradient>' +
            lin(gloss, [(0, "#fff", .55), (1, "#fff", 0)]))
    b = (f'<circle cx="{c+.8}" cy="{c+2.4}" r="{r}" fill="#000" opacity=".45"/>'
         f'<circle cx="{c}" cy="{c}" r="{r}" fill="#15171b"/>'
         f'<circle cx="{c}" cy="{c}" r="{rr}" fill="url(#{ring})"/>'
         f'<circle cx="{c}" cy="{c}" r="{br}" fill="url(#{body})" stroke="#000" stroke-opacity=".55"/>'
         f'<ellipse cx="{c}" cy="{c - br*.45}" rx="{br*.72}" ry="{br*.48}" fill="url(#{gloss})"/>')
    if pointer:
        b += f'<rect x="{c-1.3}" y="{c-rr+1}" width="2.6" height="{rr*.36}" rx="1.1" fill="#101216"/>'
    write(name, svg(size + 2*M, size + 2*M + 2, b, defs))

def knob_pointer(name, size=64):
    r = size / 2 - 2; rr = r - 1.3; c = size / 2
    b = f'<rect x="{c-1.3}" y="{c-rr+1}" width="2.6" height="{rr*.36}" rx="1.1" fill="#101216"/>'
    write(name, svg(size, size, b))

def fader_thumb(name):
    w, h = 28, 17; g = uid()
    defs = lin(g, [(0, "#f7f8fa", None), (1, "#9097a3", None)])
    b = (f'<rect x="{M}" y="{M+2}" width="{w}" height="{h}" rx="3" fill="#000" opacity=".45"/>'
         f'<rect x="{M}" y="{M}" width="{w}" height="{h}" rx="3" fill="url(#{g})" stroke="#14161a" stroke-width="1.1"/>'
         f'<rect x="{M+3}" y="{M+4}" width="{w-6}" height="1" fill="#5d636e"/>'
         f'<rect x="{M+3}" y="{M+h-5}" width="{w-6}" height="1" fill="#5d636e"/>'
         f'<rect x="{M+2}" y="{M+h/2-1}" width="{w-4}" height="2" fill="#38bdf8"/>')
    write(name, svg(w + 2*M, h + 2*M + 2, b, defs))

def fader_track(name, h=246, w=34):
    cx = w / 2
    b = (f'<rect x="{cx-2.5}" y="0" width="5" height="{h}" rx="2.5" fill="#15181d"/>'
         f'<rect x="{cx-2.5}" y="0" width="5" height="{h}" rx="2.5" fill="none" stroke="#fff" stroke-opacity=".12" stroke-width=".8"/>')
    for t in range(11):
        y = h * t / 10; ln = 8 if t % 5 == 0 else 5
        b += (f'<rect x="{cx-4-ln}" y="{y}" width="{ln}" height="1" fill="#23272e"/>'
              f'<rect x="{cx+4}" y="{y}" width="{ln}" height="1" fill="#23272e"/>')
    write(name, svg(w, h + 1, b))

def readout(name, w=58, h=16):
    b = (f'<rect x="{M}" y="{M}" width="{w}" height="{h}" rx="3.5" fill="#0d0f13"/>'
         f'<rect x="{M+.5}" y="{M+.5}" width="{w-1}" height="{h-1}" rx="3.5" fill="none" stroke="#000" stroke-opacity=".6"/>'
         f'<rect x="{M+1.5}" y="{M+1.5}" width="{w-3}" height="{h-3}" rx="2.5" fill="none" stroke="#fff" stroke-opacity=".10" stroke-width=".8"/>')
    write(name, svg(w + 2*M, h + 2*M, b))

def panel(name, w, h, top, bot, corner=5, texture=True):
    g = uid()
    defs = lin(g, [(0, top, None), (1, bot, None)])
    b = (f'<rect x="{M}" y="{M}" width="{w}" height="{h}" rx="{corner}" fill="url(#{g})"/>'
         f'<rect x="{M+1}" y="{M+1}" width="{w-2}" height="{h-2}" rx="{corner-1}" fill="none" stroke="#fff" stroke-opacity=".16"/>'
         f'<rect x="{M}" y="{M}" width="{w}" height="{h}" rx="{corner}" fill="none" stroke="#25282e" stroke-width="1.2"/>')
    write(name, svg(w + 2*M, h + 2*M, b, defs))

def led_meter(name, level_pos=0.0, w=62, h=90):
    """Stereo 24-segment meter incl. dB scale. level_pos 0..1 (evenly spaced marks -48..0)."""
    sc = 20; bx = sc; bw = w - sc
    top, uh = 5, h - 10
    b = f'<rect x="{bx}" y="0" width="{bw}" height="{h}" rx="3" fill="#0d0f13" stroke="#000"/>'
    for i, t in enumerate(["-0", "-3", "-6", "-12", "-24", "-48"]):
        y = top + uh * i / 5
        b += f'<text x="{sc-2}" y="{y+3}" text-anchor="end" font-family="Helvetica,Arial,sans-serif" font-weight="700" font-size="8.5" fill="#e4e8ee">{t}</text>'
    n = 24; sh = uh / n; barw = (bw - 9) / 2
    for ch in range(2):
        x = bx + 3 + ch * (barw + 3)
        for i in range(n):
            segpos = (i + .5) / n; y = top + uh - (i + 1) * sh
            col = "#ff5252" if i >= n - 2 else "#fbbf24" if i >= n - 5 else shade("#2f9bff", .0 + .5 * segpos)
            op = 1 if level_pos >= segpos else .13
            b += f'<rect x="{x}" y="{y+.8}" width="{barw}" height="{sh-1.6}" fill="{col}" opacity="{op}"/>'
    write(name, svg(w, h, b))

def wordmark(name):
    defs = ('<linearGradient id="au" x1="0" x2="1"><stop offset="0" stop-color="#34d399"/><stop offset=".5" stop-color="#38bdf8"/>'
            '<stop offset="1" stop-color="#a78bfa"/></linearGradient>')
    f = 'font-family="Helvetica,Arial,sans-serif" font-weight="800"'
    b = (f'<text x="120" y="29" text-anchor="middle" {f} font-size="21" letter-spacing="3.8" fill="#000" opacity=".55">AURORA</text>'
         f'<text x="120" y="28" text-anchor="middle" {f} font-size="21" letter-spacing="3.8" fill="#fff">AURORA</text>'
         f'<text x="120" y="46" text-anchor="middle" {f} font-size="11" letter-spacing="4.6" fill="#d5dae3">TRIGGER</text>'
         f'<rect x="68" y="54" width="104" height="3" rx="1.5" fill="url(#au)"/>')
    write(name, svg(240, 62, b, defs))

def write(name, content):
    with open(os.path.join(OUT, name + ".svg"), "w") as f:
        f.write(content)

# ---- buttons ---------------------------------------------------------------
LIGHT, WHITE, BLUE = "#dfe2e8", "#fbfcfe", "#38bdf8"
for sz, (w, h) in {"nav": (120, 28), "wide": (120, 26), "pad": (107, 48), "pill": (40, 22), "square": (30, 28)}.items():
    pass
button("btn_nav_off", 120, 28, LIGHT)
button("btn_nav_on", 120, 28, WHITE, glow="#ffffff")
button("btn_nav_down", 120, 28, LIGHT, pressed=True)
button("btn_wide_off", 120, 26, LIGHT)
button("btn_wide_on_blue", 120, 26, BLUE, glow=BLUE)
button("btn_wide_down", 120, 26, LIGHT, pressed=True)
button("btn_drag_midi", 112, 26, "#f2c79a")
button("btn_gate_off", 40, 22, "#c9ced6")
button("btn_gate_on", 40, 22, BLUE, glow=BLUE)
button("btn_sq_off", 30, 28, "#f4f5f7")
button("btn_sq_down", 30, 28, "#f4f5f7", pressed=True)
button("btn_sq_solo_on", 30, 28, "#fbbf24", glow="#fbbf24")
button("btn_sq_mute_on", 30, 28, "#ef4444", glow="#ef4444")
button("btn_sq_phase_on", 30, 28, BLUE, glow=BLUE)
button("pad_loaded", 107, 48, "#dadde4")
button("pad_loaded_hit", 107, 48, "#f4f6f9", glow="#ffffff")
button("pad_empty", 107, 48, "#3d4148", pressed=True)

# ---- knobs -----------------------------------------------------------------
for n, c in {"pink": "#f0708a", "blue": "#5b8def", "purple": "#a855f7", "copper": "#c99a7c", "steel": "#9ca3af"}.items():
    knob(f"knob_{n}", c, 64)
    knob(f"knob_{n}_small", c, 36)
knob("knob_blank_cap", "#9ca3af", 64, pointer=False)
knob_pointer("knob_pointer")

# ---- faders, readouts, panels, meters, brand -------------------------------
fader_thumb("fader_thumb"); fader_track("fader_track")
readout("readout_small", 58, 16); readout("readout_wide", 88, 18); readout("readout_value", 60, 17)
panel("panel_top", 520, 250, "#666a73", "#4d5159")
panel("panel_gate_input", 146, 250, "#666a73", "#4d5159")
panel("panel_sensitivity_output", 208, 250, "#666a73", "#4d5159")
panel("panel_strip", 107, 312, "#aab0bc", "#7b818d")
panel("panel_sidebar", 136, 628, "#80858f", "#5b606a")
panel("strip_label_bar", 101, 20, "#6a707b", "#4d525c", corner=3)
for n, p in {"empty": 0.0, "m12": 0.4, "m3": 0.8, "peak": 1.0}.items():
    led_meter(f"led_meter_{n}", p)
wordmark("logo_aurora_trigger")
print(len(os.listdir(OUT)), "svg files")
