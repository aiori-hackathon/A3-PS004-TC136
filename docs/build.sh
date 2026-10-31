#!/usr/bin/env bash
# Generates the two architecture slides (HTML) and the two plain figures (SVG), and renders all to 1920x1080 PNGs.
# needs: python3 and chromium (headless Firefox cannot start while a normal Firefox is open)
set -euo pipefail
cd "$(dirname "$0")"
python3 make_slides.py
for n in mode1-containers mode2-hardware; do
  chromium-browser --headless --disable-gpu --no-sandbox --hide-scrollbars \
    --window-size=1920,1080 --screenshot="slide-$n.png" "file://$PWD/slide-$n.html" >/dev/null 2>&1
done
python3 make_figures.py
echo "done: slide-*.png and fig-hardware.png, fig-containers.png"
