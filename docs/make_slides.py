#!/usr/bin/env python3
"""Writes the two architecture slides (1920x1080 HTML) used in the demo video.

Every element is placed with exact pixel coordinates, so the arrows line up with the rows they connect.
Photos and logos come from img/, icons from icons/ (inlined). build.sh renders the HTML to PNG.
"""
import pathlib

ROOT = pathlib.Path(__file__).parent

BLUE, GREY, AMBER = "#1d4ed8", "#64748b", "#b45309"
INK, SOFT = "#0f172a", "#475569"

CSS = """
* { box-sizing: border-box; }
html, body { margin: 0; background: #ffffff; }
body { font-family: 'Noto Sans', 'Liberation Sans', Arial, sans-serif; color: #0f172a; }
.slide { position: relative; width: 1920px; height: 1080px; overflow: hidden; background: #ffffff; }
.slide > * { position: absolute; }
.kicker { left: 70px; top: 42px; font-size: 19px; font-weight: 600; letter-spacing: .14em; text-transform: uppercase; color: #64748b; }
h1 { left: 70px; top: 66px; margin: 0; font-size: 54px; line-height: 1.15; font-weight: 700; letter-spacing: -.01em; }
.sub { left: 70px; top: 138px; margin: 0; font-size: 26px; color: #475569; white-space: nowrap; }

.card { background: #ffffff; border: 2px solid #cfd8e3; border-radius: 14px; box-shadow: 0 1px 3px rgba(15, 23, 42, .06); z-index: 1; }
.card::before { content: ''; position: absolute; left: -2px; right: -2px; top: -2px; height: 5px; border-radius: 14px 14px 0 0; background: #334155; }
.hd { font-size: 18px; font-weight: 700; letter-spacing: .1em; text-transform: uppercase; color: #64748b; z-index: 2; white-space: nowrap; }
.sub2 { font-size: 19px; color: #64748b; z-index: 2; white-space: nowrap; }
.photo { z-index: 2; filter: drop-shadow(0 10px 10px rgba(15, 23, 42, .28)); }
.cap { z-index: 2; text-align: center; }
.cap b { display: block; font-size: 26px; line-height: 1.25; }
.cap span { display: block; font-size: 19px; color: #64748b; }

.row { z-index: 2; border: 2px solid #d5dde8; border-radius: 10px; background: #f8fafc; padding: 8px 14px; display: flex; flex-direction: column; justify-content: center; }
.row.dashed { border-style: dashed; background: #ffffff; }
.rh { display: flex; align-items: center; gap: 11px; height: 40px; font-size: 26px; font-weight: 700; }
.rh img { display: block; }
.rs { font-size: 19.5px; color: #475569; margin-top: 1px; white-space: nowrap; }
.wm { font-size: 29px; font-weight: 700; letter-spacing: -.01em; }
.irow { z-index: 2; display: flex; align-items: center; gap: 12px; font-size: 19.5px; color: #1e293b; white-space: nowrap; height: 32px; }
.chip { z-index: 2; display: flex; align-items: center; gap: 8px; background: #f1f5f9; border: 1px solid #d5dde8; border-radius: 8px; padding: 0 10px;
        font-family: 'Noto Sans Mono', 'DejaVu Sans Mono', monospace; font-size: 15px; color: #475569; white-space: nowrap; }
.lines { z-index: 2; font-size: 19.5px; color: #334155; line-height: 1.45; white-space: nowrap; }
.lines b { color: #0f172a; }
.lines .no { color: #b91c1c; font-weight: 700; }
.mono { font-family: 'Noto Sans Mono', 'DejaVu Sans Mono', monospace; }
.note { z-index: 2; border: 2px solid #d5dde8; border-radius: 10px; background: #ffffff; padding: 10px 14px; font-size: 19px; line-height: 1.35; color: #334155; }
.note b { display: block; font-size: 20px; color: #0f172a; margin-bottom: 3px; }

svg.arrows { left: 0; top: 0; z-index: 3; }
.pill { z-index: 4; width: 68px; height: 36px; margin: -18px 0 0 -34px; border: 2px solid; border-radius: 18px; background: #ffffff; font-weight: 700; font-size: 22px;
        display: flex; align-items: center; justify-content: center; }
.pill.sm { width: 36px; margin: -18px 0 0 -18px; }
.al { z-index: 4; transform: translateX(-50%); text-align: center; white-space: nowrap; }
.al.big { font-size: 24px; font-weight: 700; }
.al.small { font-size: 20px; color: #64748b; }

.step { width: 560px; display: flex; gap: 14px; align-items: flex-start; font-size: 22px; line-height: 1.32; color: #1e293b; }
.n { flex: 0 0 36px; height: 36px; border: 2px solid; border-radius: 50%; display: flex; align-items: center; justify-content: center; font-weight: 700; font-size: 20px; margin-top: 1px; }
.foot { left: 70px; right: 70px; top: 1022px; display: flex; justify-content: space-between; border-top: 1px solid #e2e8f0; padding-top: 12px; font-size: 16px; color: #94a3b8; }
"""


def icon(name, size=26, color=INK, sw=2):
    svg = (ROOT / "icons" / f"{name}.svg").read_text()
    inner = svg[svg.index(">", svg.index("<svg")) + 1: svg.rindex("</svg>")]
    return (f'<svg width="{size}" height="{size}" viewBox="0 0 24 24" fill="none" stroke="{color}" stroke-width="{sw}" '
            f'stroke-linecap="round" stroke-linejoin="round" style="flex:0 0 auto">{inner}</svg>')


def logo(name, h):
    return f'<img src="img/logos/{name}.svg" style="height:{h}px;width:auto" alt="{name}">'


def at(left, top, w=None, h=None, extra=""):
    s = f"left:{left}px;top:{top}px;"
    if w is not None:
        s += f"width:{w}px;"
    if h is not None:
        s += f"height:{h}px;"
    return s + extra


def div(cls, left, top, w, h, inner="", extra=""):
    return f'<div class="{cls}" style="{at(left, top, w, h, extra)}">{inner}</div>'


def card(left, top, w, h):
    return div("card", left, top, w, h)


def row(left, top, w, h, head, sub, dashed=False):
    cls = "row dashed" if dashed else "row"
    return div(cls, left, top, w, h, f'<div class="rh">{head}</div><div class="rs">{sub}</div>')


def irow(left, top, ic, text):
    return div("irow", left, top, None, 32, icon(ic, 26, SOFT, 2) + f"<span>{text}</span>")


def chip(left, top, w, text):
    return div("chip", left, top, w, 34, icon("key-round", 17, GREY, 2) + f"<span>{text}</span>")


def pill(cx, cy, text, color, small=False):
    cls = "pill sm" if small else "pill"
    return f'<div class="{cls}" style="left:{cx}px;top:{cy}px;color:{color};border-color:{color}">{text}</div>'


def label(cx, top, text, cls, color):
    return f'<div class="al {cls}" style="left:{cx}px;top:{top}px;color:{color}">{text}</div>'


def defs():
    out = ""
    for key, col in (("b", BLUE), ("g", GREY), ("a", AMBER)):
        out += (f'<marker id="m{key}" markerUnits="userSpaceOnUse" markerWidth="22" markerHeight="22" refX="19" refY="11" '
                f'orient="auto-start-reverse"><path d="M0,1.5 L20,11 L0,20.5 z" fill="{col}"/></marker>')
    return f"<defs>{out}</defs>"


def line(x1, y1, x2, y2, color, key, both=True, dash=False, w=6):
    d = ' stroke-dasharray="11 8"' if dash else ""
    start = f' marker-start="url(#m{key})"' if both else ""
    return (f'<line x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}" stroke="{color}" stroke-width="{w}"{d} '
            f'marker-end="url(#m{key})"{start}/>')


def steps(items, top1=850, top2=930):
    colors = {1: BLUE, 6: BLUE, 2: GREY, 5: GREY, 3: AMBER, 4: AMBER}
    cols = (70, 680, 1290)
    out = ""
    for i, text in enumerate(items):
        n = i + 1
        col = cols[i % 3]
        top = top1 if i < 3 else top2
        out += div("step", col, top, None, None,
                   f'<span class="n" style="color:{colors[n]};border-color:{colors[n]}">{n}</span><span>{text}</span>')
    return out


def page(title, sub, body, foot_right):
    return (f'<!doctype html><html lang="en"><head><meta charset="utf-8"><title>{title}</title><style>{CSS}</style></head><body>'
            f'<div class="slide">'
            f'<div class="kicker">Authenticated gateway-assisted DNSSEC for IoT clients</div>'
            f'<h1>{title}</h1><p class="sub">{sub}</p>'
            f'{body}'
            f'<div class="foot"><span>Team Astro · A3-PS004-TC136 · AIORI-3 PS-004</span><span>{foot_right}</span></div>'
            f'</div></body></html>')


# ---------------------------------------------------------------- slide: mode 2, real hardware
def mode2():
    T = 205            # card top
    H = 625            # card height
    b = ""
    # cards
    b += card(70, T, 420, H) + card(700, T, 520, H) + card(1430, T, 420, H)

    # --- IoT device
    b += div("hd", 92, 227, None, None, "IoT device")
    b += div("", 353, 222, None, None, logo("espressif", 26), "z-index:2")
    b += f'<img class="photo" src="img/esp32-s3-devkit.png" style="{at(130, 258, 300)}" alt="ESP32-S3-DevKitC-1">'
    b += div("cap", 92, 524, 376, None, "<b>ESP32-S3-DevKitC-1</b><span>IoT client · ESP-IDF firmware</span>")
    for i, (ic, t) in enumerate((("shield-off", "No DNSSEC code on the device"),
                                 ("eye", "Acts on RCODE + AD only"),
                                 ("list-checks", "Refuses unsigned critical names"),
                                 ("ban", "No gateway: refuse, no plain DNS"))):
        b += irow(96, 604 + i * 42, ic, t)
    b += chip(92, 782, 376, "esp32-1 key + cert · CA cert built in")

    # --- gateway
    b += div("hd", 722, 227, None, None, "Gateway")
    b += f'<img class="photo" src="img/raspberry-pi-zero-2-w.png" style="{at(795, 254, 330)}" alt="Raspberry Pi Zero 2 W">'
    b += div("cap", 722, 452, 476, None, "<b>Raspberry Pi Zero 2 W</b><span>runs natively · 415 MB RAM</span>")
    b += row(722, 530, 476, 84, icon("lock", 34, INK, 2.2) + '<span class="wm">stunnel</span>', "port 853 · checks the device certificate")
    b += row(722, 694, 476, 84, logo("unbound", 36), "DNSSEC validation · sets the AD flag")
    b += chip(722, 790, 476, "gateway key + cert · CA cert only (no ca.key)")

    # --- laptop
    b += div("hd", 1452, 227, None, None, "Laptop · plays the internet")
    b += div("", 1571, 262, None, None, icon("laptop", 128, "#94a3b8", 1.4), "z-index:2")
    b += row(1452, 452, 376, 84, icon("stamp", 30, INK, 2) + "<span>Certificate authority</span>", "ca.key never leaves the laptop", dashed=True)
    b += div("lines", 1452, 600, 376, 40, '<span style="display:flex;align-items:center;gap:12px">runs as a container '
             + logo("docker", 30) + logo("containerlab", 30) + "</span>")
    b += row(1452, 694, 376, 84, logo("knot", 40), "port 5300 · 9 zones, 6 broken")
    b += chip(1452, 790, 376, "DNSSEC keys in a Docker volume")

    # --- arrows
    arrows = defs()
    arrows += line(494, 572, 718, 572, BLUE, "b")                      # 1 / 6  device <-> stunnel
    arrows += line(1200, 736, 1450, 736, AMBER, "a")                   # 3 / 4  unbound <-> knot
    arrows += line(880, 617, 880, 691, GREY, "g", both=False, dash=True, w=4)   # 2  down
    arrows += line(1040, 691, 1040, 617, GREY, "g", both=False, dash=True, w=4)  # 5  up
    b += f'<svg class="arrows" width="1920" height="1080" viewBox="0 0 1920 1080">{arrows}</svg>'
    b += pill(595, 572, "1 · 6", BLUE)
    b += label(595, 520, "mutual TLS", "big", BLUE)
    b += label(595, 598, "TLS 1.2 · over Wi-Fi", "small", GREY)
    b += pill(1325, 736, "3 · 4", AMBER)
    b += label(1325, 684, "DNSSEC", "big", AMBER)
    b += label(1325, 762, "over Wi-Fi", "small", GREY)
    b += pill(840, 654, "2", GREY, small=True) + pill(1080, 654, "5", GREY, small=True)
    b += label(960, 641, "plain DNS", "small", GREY)

    # --- flow
    b += steps(["The ESP32 opens mutual TLS. Both sides show a certificate issued by our own CA.",
                "stunnel hands the query to Unbound as plain DNS, on the same device.",
                "Unbound asks Knot and checks every signature up to its trust anchor.",
                "Knot returns the signed records.",
                "Unbound's verdict: the AD flag, or SERVFAIL with the reason (EDE).",
                "Back through the tunnel. The ESP32 uses the address, flags it, or refuses."])
    return page("Mode 2 · Real hardware",
                "The ESP32 never validates DNSSEC. The Raspberry Pi gateway does, and the device trusts it only over mutual TLS.",
                b, "Photos: Laserlicht, VectorVoyager · Wikimedia Commons · CC BY-SA 4.0 · logos belong to their owners")


# ---------------------------------------------------------------- slide: mode 1, containers
def mode1():
    T, H = 205, 625
    b = card(70, T, 420, H) + card(680, T, 1170, H)

    # --- client
    b += div("hd", 92, 227, None, None, "Client")
    b += div("", 353, 222, None, None, logo("espressif", 26), "z-index:2")
    b += f'<img class="photo" src="img/esp32-s3-devkit.png" style="{at(115, 252, 330)}" alt="ESP32-S3-DevKitC-1">'
    b += div("cap", 92, 534, 376, None, "<b>ESP32-S3-DevKitC-1</b><span class=\"mono\">pio run -e lab</span>")
    b += row(92, 636, 376, 84, icon("terminal", 30, INK, 2) + "<span>kdig</span>", "the same client, from a terminal")
    b += chip(92, 744, 376, "esp32-1 key + cert · CA cert built in")

    # --- laptop (big card)
    b += div("hd", 704, 227, None, None, "Laptop · Docker + Containerlab")
    b += div("", 1650, 218, None, None, logo("docker", 32) + '<span style="width:14px;display:inline-block"></span>' + logo("containerlab", 32),
             "z-index:2;display:flex;align-items:center;left:auto;right:94px")
    b += div("sub2", 704, 256, None, None, '<span class="mono" style="font-size:17px">network clab1 · 10.54.0.0/24 · lab/dnssec.clab.yml</span>')

    CT, CH, CW = 300, 300, 280
    xs = (704, 1125, 1546)
    for x in xs:
        b += div("card", x, CT, CW, CH, "", "border-radius:12px;box-shadow:none;background:#f8fafc")
    heads = (icon("lock", 36, INK, 2.2) + '<span class="wm">stunnel</span>', logo("unbound", 38), logo("knot", 42))
    ips = ("10.54.0.15", "10.54.0.3", "10.54.0.2")
    bodies = (
        '<div>mutual TLS, port 853</div><div><b>published 8530 → 853</b></div><div>certs mounted read-only</div><div>no ca.key inside</div>',
        '<div>validates DNSSEC</div><div>sets AD, or SERVFAIL + EDE</div><div class="no">no published port</div><div>answers only 10.54.0.15</div>',
        '<div>9 zones, 6 of them broken</div><div><b>published 5300 → 53</b></div><div>keys in a named volume</div><div>trust anchor: lab.test</div>',
    )
    bodies = (bodies[0],
              '<div>validates DNSSEC</div><div class="no">no published port</div><div>answers only 10.54.0.15</div><div>trust anchor: lab.test</div>',
              '<div>9 zones, 6 of them broken</div><div><b>published 5300 → 53</b></div><div>keys in a named volume</div>')
    for x, hd, ip, body in zip(xs, heads, ips, bodies):
        b += div("rh", x + 18, CT + 22, CW - 36, 44, hd, "z-index:2")
        b += div("chip", x + 18, CT + 84, 130, 32, f"<span>{ip}</span>")
        b += div("lines", x + 18, CT + 138, CW - 30, 160, body)

    # --- notes under the containers
    NT, NH = 630, 172
    b += div("note", 704, NT, CW, NH, "<b>Why a separate stunnel?</b>Unbound cannot require client certificates.")
    b += div("note", 1125, NT, CW, NH, "<b>Two locks on the back door</b>1 answers only 10.54.0.15<br>2 no host port exists",
             "border-color:#e7c4c4")
    b += div("note", 1546, NT, CW, NH, '<b>Mode 2 keeps only this one</b><span class="mono" style="font-size:16px;line-height:1.5">containerlab deploy<br>--node-filter knot</span>')

    # --- arrows (y = middle of the container cards)
    Y = CT + CH // 2
    arrows = defs()
    arrows += line(494, Y, 700, Y, BLUE, "b")
    arrows += line(986, Y, 1123, Y, GREY, "g", dash=True)
    arrows += line(1407, Y, 1544, Y, AMBER, "a")
    b += f'<svg class="arrows" width="1920" height="1080" viewBox="0 0 1920 1080">{arrows}</svg>'
    b += pill(585, Y, "1 · 6", BLUE)
    b += label(585, Y - 52, "mutual TLS", "big", BLUE)
    b += label(585, Y + 30, "laptop port 8530", "small", GREY)
    b += pill(1055, Y, "2 · 5", GREY)
    b += label(1055, Y - 52, "plain DNS", "big", GREY)
    b += pill(1476, Y, "3 · 4", AMBER)
    b += label(1476, Y - 52, "DNSSEC", "big", AMBER)

    b += steps(["The client opens mutual TLS to stunnel on the laptop's port 8530.",
                "stunnel passes plain DNS to Unbound inside the lab network.",
                "Unbound asks Knot and checks every signature.",
                "Knot returns the signed records.",
                "Verdict: the AD flag, or SERVFAIL with the reason (EDE).",
                "Back through the tunnel to the client."])
    return page("Mode 1 · Everything in containers",
                "The same configs and firmware code as mode 2. One command rebuilds the lab: <b>containerlab deploy</b>.",
                b, "Photo: VectorVoyager · Wikimedia Commons · CC BY-SA 4.0 · logos belong to their owners")


if __name__ == "__main__":
    (ROOT / "slide-mode2-hardware.html").write_text(mode2())
    (ROOT / "slide-mode1-containers.html").write_text(mode1())
    print("wrote slide-mode2-hardware.html and slide-mode1-containers.html")
