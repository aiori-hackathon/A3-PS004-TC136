#!/usr/bin/env python3
"""Two minimal engineering figures: mode 2 (hardware) and mode 1 (containers).

Writes fig-hardware.svg and fig-containers.svg next to this file and renders each
to a 1920x1080 PNG with headless Chromium. Everything sits on exact coordinates;
text widths are measured so nothing can overflow a box.
"""
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
FONT = "'Liberation Sans', Arial, Helvetica, sans-serif"
INK, GREY, BOX, ZONE = "#1a1a1a", "#555555", "#333333", "#888888"
TLS, SIG, DNS = "#1f3f8f", "#8a4b08", "#555555"
CANVAS_W, CANVAS_H = 1920, 1080

# logos used as the title of their box (file, width / height); the lock is drawn for stunnel, which has no logo
LOGOS = {
    "unbound": ("img/logos/unbound.svg", 669 / 153, 40),
    "knot": ("img/logos/knot.svg", 600 / 225, 42),
    "docker": ("img/logos/docker.svg", 4.2069, 30),
    "containerlab": ("img/logos/containerlab.svg", 1.0, 30),
    "espressif": ("img/logos/espressif.svg", 593.3 / 134.2, 22),
}
LOCK = '<rect width="18" height="11" x="3" y="11" rx="2" ry="2"/><path d="M7 11V7a5 5 0 0 1 10 0v4"/>'

try:
    from PIL import ImageFont
    _F = {False: ImageFont.truetype("/usr/share/fonts/liberation-sans-fonts/LiberationSans-Regular.ttf", 100),
          True: ImageFont.truetype("/usr/share/fonts/liberation-sans-fonts/LiberationSans-Bold.ttf", 100)}
except Exception:  # measuring is a safety net, not required
    _F = None


def measure(s, size, bold=False):
    if _F is None:
        return len(s) * size * 0.55
    return _F[bold].getlength(s) * size / 100.0


class Fig:
    def __init__(self, caption):
        self.parts, self.caption = [], caption
        self.xmax = self.ymax = 0
        self.warnings = []

    def grow(self, x, y):
        self.xmax, self.ymax = max(self.xmax, x), max(self.ymax, y)

    def text(self, x, y, s, size=20, bold=False, italic=False, color=INK, anchor="start", fit=None):
        w = measure(s, size, bold)
        if fit is not None and w > fit:
            self.warnings.append(f"text too wide ({w:.0f} > {fit}): {s}")
        style = ""
        if bold:
            style += ' font-weight="700"'
        if italic:
            style += ' font-style="italic"'
        self.parts.append(f'<text x="{x}" y="{y}" font-size="{size}" fill="{color}" text-anchor="{anchor}"{style}>{s}</text>')
        left = x - (w / 2 if anchor == "middle" else 0)
        self.grow(left + w, y + size * 0.25)

    def rect(self, x, y, w, h, stroke=BOX, sw=2, dash=None, fill="#ffffff"):
        d = f' stroke-dasharray="{dash}"' if dash else ""
        self.parts.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" fill="{fill}" stroke="{stroke}" stroke-width="{sw}"{d}/>')
        self.grow(x + w, y + h)

    def zone(self, x, y, w, h, legend):
        self.rect(x, y, w, h, stroke=ZONE, sw=1.5, dash="10 6", fill="none")
        tw = measure(legend, 24, True)
        self.parts.append(f'<rect x="{x + 16}" y="{y - 15}" width="{tw + 16:.0f}" height="30" fill="#ffffff"/>')
        self.text(x + 24, y + 8, legend, 24, True, color=INK)

    def mark(self, cx, base, kind):
        """the box title as a logo; stunnel has none, so a lock icon and its name at the same height"""
        if kind == "stunnel":
            tw = measure("stunnel", 28, True)
            x0 = cx - (30 + 10 + tw) / 2
            self.parts.append(f'<g transform="translate({x0:.1f} {base - 25}) scale(1.25)" fill="none" stroke="{INK}" '
                              f'stroke-width="2" stroke-linecap="round" stroke-linejoin="round">{LOCK}</g>')
            self.text(x0 + 40, base, "stunnel", 28, True)
            return
        href, aspect, h = LOGOS[kind]
        w = h * aspect
        self.image(href, round(cx - w / 2), base - 34, round(w), h)

    def box(self, x, y, w, h, title, lines=(), dashed=False, mark=None):
        self.rect(x, y, w, h, dash="8 6" if dashed else None)
        cx = x + w / 2
        base = y + (50 if lines else h / 2 + 10)
        if mark:
            self.mark(cx, base, mark)
        else:
            self.text(cx, base, title, 28, True, anchor="middle", fit=w - 40)
        for i, s in enumerate(lines):
            self.text(cx, y + 88 + 28 * i, s, 20, False, color=GREY, anchor="middle", fit=w - 30)

    def image(self, href, x, y, w, h):
        self.parts.append(f'<image href="{href}" x="{x}" y="{y}" width="{w}" height="{h}"/>')
        self.grow(x + w, y + h)

    def line(self, x1, y1, x2, y2, color, sw=3, dash=None, marker=None):
        d = f' stroke-dasharray="{dash}"' if dash else ""
        m = f' marker-end="url(#a-{marker})"' if marker else ""
        self.parts.append(f'<line x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}" stroke="{color}" stroke-width="{sw}"{d}{m}/>')
        self.grow(max(x1, x2), max(y1, y2))

    def arrow(self, x1, y1, x2, y2, color, dash=None):
        self.line(x1, y1, x2, y2, color, 3, dash, marker=color.strip("#"))

    def numeral(self, cx, cy, n, color=INK):
        self.parts.append(f'<circle cx="{cx}" cy="{cy}" r="14" fill="#ffffff" stroke="{color}" stroke-width="1.5"/>')
        self.parts.append(f'<text x="{cx}" y="{cy + 7}" font-size="20" fill="{color}" text-anchor="middle">{n}</text>')

    def pair_h(self, xa, xb, y, color, na, nb, xna, xnb, dash=None):
        """request a->b on the upper line, response b->a on the lower line, centred on y"""
        self.arrow(xa, y - 12, xb, y - 12, color, dash)
        self.arrow(xb, y + 12, xa, y + 12, color, dash)
        self.numeral(xna, y - 12, na, color)
        self.numeral(xnb, y + 12, nb, color)

    def svg(self, name):
        ox = round((CANVAS_W - self.xmax) / 2)
        oy = round((CANVAS_H - self.ymax) / 2)
        markers = "".join(
            f'<marker id="a-{c.strip("#")}" viewBox="0 0 10 10" refX="10" refY="5" markerWidth="4.4" markerHeight="4.4" '
            f'orient="auto" markerUnits="strokeWidth"><path d="M0 0 L10 5 L0 10 z" fill="{c}"/></marker>'
            for c in (TLS, SIG, DNS))
        body = "\n  ".join(self.parts)
        cap = (f'<text x="80" y="{CANVAS_H - 80}" font-size="20" fill="{GREY}" xml:space="preserve">{self.caption}</text>')
        out = (f'<svg xmlns="http://www.w3.org/2000/svg" width="{CANVAS_W}" height="{CANVAS_H}" '
               f'viewBox="0 0 {CANVAS_W} {CANVAS_H}" font-family="{FONT}">\n'
               f'  <title>{self.caption}</title>\n  <defs>{markers}</defs>\n'
               f'  <rect width="{CANVAS_W}" height="{CANVAS_H}" fill="#ffffff"/>\n'
               f'  <g transform="translate({ox} {oy})">\n  {body}\n  </g>\n  {cap}\n</svg>\n')
        (HERE / f"{name}.svg").write_text(out, encoding="utf-8")
        for w in self.warnings:
            print("WARNING:", w)
        print(f"{name}.svg  drawing {self.xmax:.0f}x{self.ymax:.0f}  offset {ox},{oy}")


def hardware():
    f = Fig("Figure 1  Mode 2, real hardware")
    Y1, Y2 = 380, 620                      # link axes: stunnel row and Unbound row
    # zones
    f.zone(0, 120, 440, 500, "IoT device")
    f.zone(640, 0, 520, 720, "Gateway")
    f.zone(1360, 260, 400, 460, "Laptop")
    # IoT device: one node box, centred on the first link axis
    f.rect(40, 180, 360, 400)
    f.image("img/esp32-s3-devkit.png", 70, 200, 300, 254)
    f.image("img/logos/espressif.svg", 400 - 14 - 97, 192, 97, 22)
    f.text(220, 500, "ESP32-S3-DevKitC-1", 28, True, anchor="middle", fit=340)
    f.text(220, 538, "esp32-1 key and certificate", 20, False, color=GREY, anchor="middle", fit=340)
    # Gateway
    f.image("img/raspberry-pi-zero-2-w.png", 740, 40, 320, 188)
    f.text(900, 276, "Raspberry Pi Zero 2 W", 28, True, anchor="middle", fit=480)
    f.box(680, 320, 440, 120, "stunnel", ["port 853, checks the device certificate"], mark="stunnel")
    f.box(680, 560, 440, 120, "Unbound", ["validates DNSSEC, sets the AD flag"], mark="unbound")
    # Laptop
    f.box(1400, 320, 320, 120, "Certificate authority", ["ca.key stays on the laptop"], dashed=True)
    f.box(1400, 560, 320, 120, "Knot DNS", ["port 5300, nine test zones"], mark="knot")
    # link 1 and 6: ESP32 <-> stunnel, mutual TLS over Wi-Fi (gap between zones 440..640)
    f.pair_h(400, 680, Y1, TLS, 1, 6, 492, 588)
    f.text(540, Y1 - 44, "Mutual TLS 1.2", 24, True, color=TLS, anchor="middle", fit=190)
    f.text(540, Y1 + 56, "Wi-Fi", 20, italic=True, color=GREY, anchor="middle")
    f.text(540, Y1 + 80, "authenticated", 20, color=GREY, anchor="middle")
    f.text(540, Y1 + 104, "and encrypted", 20, color=GREY, anchor="middle")
    # link 2 and 5: stunnel <-> Unbound, plain DNS on the same device (vertical)
    f.arrow(888, 440, 888, 560, DNS, "8 6")
    f.arrow(912, 560, 912, 440, DNS, "8 6")
    f.numeral(888, 470, 2, DNS)
    f.numeral(912, 530, 5, DNS)
    f.text(944, 494, "Plain DNS", 24, True, color=DNS, fit=170)
    f.text(944, 522, "localhost", 20, color=GREY)
    # link 3 and 4: Unbound <-> Knot, DNSSEC over Wi-Fi (gap between zones 1160..1360)
    f.pair_h(1120, 1400, Y2, SIG, 3, 4, 1212, 1308)
    f.text(1260, Y2 - 44, "DNSSEC", 24, True, color=SIG, anchor="middle", fit=190)
    f.text(1260, Y2 + 56, "Wi-Fi", 20, italic=True, color=GREY, anchor="middle")
    f.text(1260, Y2 + 80, "authenticated", 20, color=GREY, anchor="middle")
    f.text(1260, Y2 + 104, "by signatures", 20, color=GREY, anchor="middle")
    f.svg("fig-hardware")


def containers():
    f = Fig("Figure 2  Mode 1, everything in containers")
    Y1 = 310                               # link axis: middle between the two client boxes
    f.zone(0, 0, 360, 500, "Client")
    f.zone(610, 180, 1150, 240, "Laptop, Docker network clab1 (10.54.0.0/24)")
    # client: ESP32 node and kdig, joined by a bus into one link
    f.rect(40, 60, 280, 280)
    f.image("img/esp32-s3-devkit.png", 60, 80, 240, 203)
    f.image("img/logos/espressif.svg", 320 - 12 - 97, 70, 97, 22)
    f.text(180, 318, "ESP32-S3", 28, True, anchor="middle", fit=260)
    dw = round(LOGOS["docker"][1] * 30)
    f.image("img/logos/containerlab.svg", 1760 - 16 - 30, 192, 30, 30)
    f.image("img/logos/docker.svg", 1760 - 16 - 30 - 14 - dw, 192, dw, 30)
    f.box(40, 380, 280, 80, "kdig")
    f.line(320, 200, 400, 200, TLS)
    f.line(320, 420, 400, 420, TLS)
    f.line(400, 200, 400, 420, TLS)
    # containers
    f.box(650, 240, 250, 140, "stunnel", ["10.54.0.15", "published 8530 to 853"], mark="stunnel")
    f.box(1060, 240, 250, 140, "Unbound", ["10.54.0.3", "no published port"], mark="unbound")
    f.box(1470, 240, 250, 140, "Knot DNS", ["10.54.0.2", "published 5300 to 53"], mark="knot")
    # links
    f.pair_h(400, 650, Y1, TLS, 1, 6, 455, 565)
    f.text(507, Y1 - 42, "Mutual TLS 1.2", 24, True, color=TLS, anchor="middle", fit=190)
    f.pair_h(900, 1060, Y1, DNS, 2, 5, 932, 1028, dash="8 6")
    f.text(980, Y1 - 42, "Plain DNS", 24, True, color=DNS, anchor="middle", fit=130)
    f.pair_h(1310, 1470, Y1, SIG, 3, 4, 1342, 1438)
    f.text(1390, Y1 - 42, "DNSSEC", 24, True, color=SIG, anchor="middle", fit=130)
    f.svg("fig-containers")


def render(name):
    svg, png = HERE / f"{name}.svg", HERE / f"{name}.png"
    subprocess.run(["chromium-browser", "--headless", "--disable-gpu", "--no-sandbox", "--hide-scrollbars",
                    f"--window-size={CANVAS_W},{CANVAS_H}", f"--screenshot={png}", f"file://{svg}"],
                   check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    print(f"{name}.png")


if __name__ == "__main__":
    hardware()
    containers()
    if "--no-png" not in sys.argv:
        render("fig-hardware")
        render("fig-containers")
