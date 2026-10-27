# Icona dell'app Standalone: il pozzetto del Phonogène con i dodici capstan e l'anello di nastro.
import math
from PIL import Image, ImageDraw, ImageFilter

S = 4096                      # disegno sovracampionato, poi ridotto a 1024
img = Image.new("RGBA", (S, S), (0, 0, 0, 0))
d = ImageDraw.Draw(img)

def sc(v): return int(v * S / 1024)

# piastra quadrata arrotondata (forma delle icone macOS), smalto grigio chiaro
m, rad = sc(100), sc(185)
shadow = Image.new("RGBA", (S, S), (0, 0, 0, 0))
ImageDraw.Draw(shadow).rounded_rectangle((m, m + sc(18), S - m, S - m + sc(18)), rad, fill=(0, 0, 0, 120))
img.alpha_composite(shadow.filter(ImageFilter.GaussianBlur(sc(22))))
plate = Image.new("RGBA", (S, S), (0, 0, 0, 0))
pd = ImageDraw.Draw(plate)
for i in range(S - 2 * m):                       # gradiente verticale
    t = i / (S - 2 * m)
    c = tuple(int(a + (b - a) * t) for a, b in zip((226, 224, 216), (196, 194, 186)))
    pd.line((m, m + i, S - m, m + i), fill=c + (255,))
mask = Image.new("L", (S, S), 0)
ImageDraw.Draw(mask).rounded_rectangle((m, m, S - m, S - m), rad, fill=255)
img.paste(plate, (0, 0), mask)
d.rounded_rectangle((m, m, S - m, S - m), rad, outline=(70, 70, 66), width=sc(6))

# pozzetto
cx, cy, wr = S / 2, S / 2 + sc(10), sc(330)
d.ellipse((cx - wr - sc(12), cy - wr - sc(12), cx + wr + sc(12), cy + wr + sc(12)), fill=(43, 43, 41))
for i in range(60):                               # fondo con leggero gradiente
    t = i / 59
    r = wr * (1 - t * 0.02)
    c = tuple(int(a + (b - a) * t) for a, b in zip((165, 163, 155), (206, 204, 195)))
    d.ellipse((cx - r, cy - r + t * sc(6), cx + r, cy + r + t * sc(6)), fill=c)

R = sc(226)
angs = [math.radians(34 + 292 * k / 11) for k in range(12)]
caps = [(cx + R * math.sin(a), cy - R * math.cos(a), sc(17) * 2 ** (k / 12)) for k, a in enumerate(angs)]

# anello di nastro morbido, passando all'esterno dei capstan
pts = []
for (x, y, r), a in zip(caps, angs):
    pts.append((cx + (R + r + sc(5)) * math.sin(a), cy - (R + r + sc(5)) * math.cos(a)))
head = (cx + (R + sc(8)) * math.sin(math.radians(352)), cy - (R + sc(8)) * math.cos(math.radians(352)))
tension = (cx, cy - R * 0.12)
pts += [head, (tension[0] - sc(32), tension[1]), (tension[0], tension[1] + sc(34)), (tension[0] + sc(32), tension[1]),
        (cx + (R + sc(10)) * math.sin(math.radians(19)), cy - (R + sc(10)) * math.cos(math.radians(19)))]

def chaikin(p, n=5):
    for _ in range(n):
        q = []
        for i in range(len(p)):
            a, b = p[i], p[(i + 1) % len(p)]
            q += [(0.75 * a[0] + 0.25 * b[0], 0.75 * a[1] + 0.25 * b[1]), (0.25 * a[0] + 0.75 * b[0], 0.25 * a[1] + 0.75 * b[1])]
        p = q
    return p
loop = chaikin(pts)
d.line(loop + [loop[0]], fill=(30, 22, 15), width=sc(16), joint="curve")
d.line(loop + [loop[0]], fill=(110, 72, 40), width=sc(11), joint="curve")

def metal(x, y, r):
    d.ellipse((x - r + sc(4), y - r + sc(6), x + r + sc(4), y + r + sc(6)), fill=(0, 0, 0, 90))
    steps = 24
    for i in range(steps):
        t = i / (steps - 1)
        rr = r * (1 - 0.55 * t)
        c = int(140 + 110 * t)
        d.ellipse((x - rr - r * 0.18 * t, y - rr - r * 0.22 * t, x + rr - r * 0.18 * t, y + rr - r * 0.22 * t), fill=(c, c, c - 4))
    d.ellipse((x - r, y - r, x + r, y + r), outline=(80, 82, 78), width=sc(3))
    d.ellipse((x - r * 0.2, y - r * 0.2, x + r * 0.2, y + r * 0.2), fill=(80, 82, 78))

for k, (x, y, r) in enumerate(caps):
    metal(x, y, r)
    a = angs[k]
    px, py = cx + (R + r + sc(29)) * math.sin(a), cy - (R + r + sc(29)) * math.cos(a)
    pr = sc(20)
    d.ellipse((px - pr, py - pr, px + pr, py + pr), fill=(22, 22, 21))
    d.ellipse((px - pr * 0.4, py - pr * 0.4, px + pr * 0.4, py + pr * 0.4), fill=(150, 152, 148))
metal(tension[0], tension[1], sc(26))

# testina d'acciaio in alto
hx, hy = head[0], head[1] - sc(26)
d.rounded_rectangle((hx - sc(34), hy - sc(22), hx + sc(34), hy + sc(22)), sc(6), fill=(214, 216, 212), outline=(43, 43, 41), width=sc(4))
d.rectangle((hx - sc(2), hy + sc(8), hx + sc(2), hy + sc(22)), fill=(43, 43, 41))

# spia ambra (nastro in movimento)
lx, ly = S - m - sc(120), m + sc(120)
d.ellipse((lx - sc(46), ly - sc(46), lx + sc(46), ly + sc(46)), fill=(232, 176, 74, 70))
d.ellipse((lx - sc(30), ly - sc(30), lx + sc(30), ly + sc(30)), fill=(42, 42, 40))
d.ellipse((lx - sc(21), ly - sc(21), lx + sc(21), ly + sc(21)), fill=(232, 176, 74))
d.ellipse((lx - sc(13), ly - sc(13), lx - sc(3), ly - sc(3)), fill=(255, 240, 210))

img.resize((1024, 1024), Image.LANCZOS).save("icon.png")
print("ok")
