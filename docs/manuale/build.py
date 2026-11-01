import math, pathlib, sys
src = pathlib.Path(__file__).parent / "src"
notes = ["DO", "DO#", "RÉ", "RÉ#", "MI", "FA", "FA#", "SOL", "SOL#", "LA", "LA#", "SI"]
W, gap = 1000.0, 13.0
k = (W - 11 * gap) / sum(2 ** (n / 12) for n in range(12))
dmax = k * 2 ** (11 / 12)
base = dmax + 6          # linea del nastro
parts = []
x = 0.0
for n in range(12):
    d = k * 2 ** (n / 12)
    r = d / 2
    cx, cy = x + r, base - r
    parts.append(f'<circle cx="{cx:.1f}" cy="{cy:.1f}" r="{r:.1f}" fill="#E4E5E1" stroke="#1E201D" stroke-width="1.6"/>')
    parts.append(f'<circle cx="{cx:.1f}" cy="{cy:.1f}" r="{r*0.78:.1f}" fill="none" stroke="#1E201D" stroke-width=".8" opacity=".35"/>')
    parts.append(f'<circle cx="{cx:.1f}" cy="{cy:.1f}" r="{max(3.2, r*0.16):.1f}" fill="#1E201D"/>')
    parts.append(f'<text x="{cx:.1f}" y="{base+38:.1f}" text-anchor="middle" class="nt">{notes[n]}</text>')
    ratio = f"{2 ** (n / 12):.3f}".replace(".", ",")
    parts.append(f'<text x="{cx:.1f}" y="{base+60:.1f}" text-anchor="middle" class="rt">{ratio}</text>')
    x += d + gap
tape = f'<rect x="-20" y="{base:.1f}" width="{W+40:.0f}" height="11" fill="#3E4A43"/>'
H = base + 70
svg = (f'<svg viewBox="-20 0 {W+40:.0f} {H:.0f}" role="img" aria-label="Dodici capstan di diametro crescente, dal DO (velocità 1,000) al SI (1,888), appoggiati sul nastro">'
       '<style>.nt{font:600 19px Archivo,sans-serif;font-stretch:88%;letter-spacing:.04em;fill:currentColor}'
       '.rt{font:400 16px Archivo,sans-serif;font-variant-numeric:tabular-nums;fill:currentColor;opacity:.7}</style>'
       + tape + "".join(parts) + "</svg>")
html = (src / "template.html").read_text().replace("{{CAPSTANS}}", svg)
(src / "index.html").write_text(html)
print("index.html ok")
