#!/usr/bin/env python3
"""Turns the FLUX images in raw/ into game art: background removal, scaling to game size, 16-colour palettes,
packed 4 bits per pixel -> ../gotanda_art.h.      build_assets.py [--preview DIR]

Sprite format (see gotanda.c): Art { pal[16] (RGB565), w, h, px } ; px = row-major, two pixels per byte (high nibble first);
index 15 = transparent.  The game keys transparency to the flat floor colour of the room when it decodes a sprite.
"""
import math, os, sys
from collections import deque
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
RAW = os.path.join(HERE, "raw")

def rgb565(c):
    r, g, b = c[:3]
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

# ---------- background removal ----------
def key_alpha(img, tol=64):
    """flood fill from the border: pixels close to the (drifting) background colour become transparent"""
    img = img.convert("RGB")
    w, h = img.size
    px = img.load()
    ring = [px[x, y] for x in range(0, w, 8) for y in (0, 1, h - 2, h - 1)] + [px[x, y] for y in range(0, h, 8) for x in (0, 1, w - 2, w - 1)]
    bg = tuple(sorted(c[i] for c in ring)[len(ring) // 2] for i in range(3))
    bgn = math.sqrt(sum(v * v for v in bg)) or 1
    def near(c, ref, t):
        d2 = (c[0] - ref[0]) ** 2 + (c[1] - ref[1]) ** 2 + (c[2] - ref[2]) ** 2
        if d2 < t * t: return True
        n = math.sqrt(sum(v * v for v in c)) or 1           # the soft ground shadow: same hue as the background, darker
        return 0.35 * bgn <= n <= 1.05 * bgn and sum(a * b for a, b in zip(c, ref)) / (n * bgn) > 0.985
    alpha = Image.new("L", (w, h), 255)
    ap = alpha.load()
    seen = bytearray(w * h)
    q = deque()
    for x in range(w):
        for y in (0, h - 1): q.append((x, y))
    for y in range(h):
        for x in (0, w - 1): q.append((x, y))
    while q:
        x, y = q.popleft()
        if x < 0 or y < 0 or x >= w or y >= h or seen[y * w + x]: continue
        seen[y * w + x] = 1
        if not near(px[x, y], bg, tol): continue
        ap[x, y] = 0
        q.extend(((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)))
    # the soft shadow under the feet: remove darker versions of the background colour that touch the background
    out = img.convert("RGBA"); out.putalpha(alpha)
    return out

def bbox_alpha(img):
    return img.getchannel("A").point(lambda v: 255 if v > 128 else 0).getbbox()

def clean_edges(img):
    """binary alpha; edge pixels that still carry the background tint are pulled towards their opaque neighbours"""
    a = img.getchannel("A").point(lambda v: 255 if v >= 128 else 0)
    img = img.copy(); img.putalpha(a)
    return img

# ---------- fitting ----------
def place(img, w, h, scale, cx=None, bottom=None, anchor="bottom"):
    """scale the cropped sprite by `scale`, paste it into a w x h canvas bottom-centred"""
    box = bbox_alpha(img)
    crop = img.crop(box)
    nw, nh = max(1, round(crop.width * scale)), max(1, round(crop.height * scale))
    # premultiplied resize so edges do not pick up the keyed-out colour
    r, g, b, a = crop.split()
    pm = Image.merge("RGBA", (r, g, b, a))
    small = pm.resize((nw, nh), Image.LANCZOS)
    canvas = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    x = (w - nw) // 2 if cx is None else cx - nw // 2
    y = h - nh if anchor == "bottom" else (h - nh) // 2
    if nw > w: small = small.crop(((nw - w) // 2, 0, (nw - w) // 2 + w, nh)); x = 0
    if nh > h: small = small.crop((0, nh - h, small.width, nh)); y = 0
    canvas.paste(small, (x, y), small)
    return clean_edges(canvas)

# ---------- quantising / packing ----------
def pack(img, transparent=True, ncol=None):
    w, h = img.size
    ncol = ncol or (15 if transparent else 16)
    rgb = Image.new("RGB", (w, h))
    a = img.getchannel("A")
    opaque = [p for p, m in zip(img.convert("RGB").getdata(), a.getdata()) if m >= 128] or [(0, 0, 0)]
    avg = tuple(sum(c[i] for c in opaque) // len(opaque) for i in range(3))
    rgb.putdata([p if m >= 128 else avg for p, m in zip(img.convert("RGB").getdata(), a.getdata())])
    q = rgb.quantize(colors=ncol, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE)
    pal = q.getpalette()[: 3 * ncol]
    colors = [tuple(pal[i * 3:i * 3 + 3]) for i in range(ncol)]
    while len(colors) < 16: colors.append((255, 0, 255))
    idx = list(q.getdata())
    out = []
    for i, (v, m) in enumerate(zip(idx, a.getdata())):
        out.append(v if (m >= 128 or not transparent) else 15)
    data = bytearray()
    for i in range(0, len(out), 2):
        hi = out[i]; lo = out[i + 1] if i + 1 < len(out) else 15
        data.append((hi << 4) | lo)
    if transparent: colors[15] = (255, 0, 255)
    return [rgb565(c) for c in colors], bytes(data)

# ---------- specs ----------
CHAR_SIZE = {"player": (40, 56), "drunk": (44, 60), "yakuza": (44, 60), "hostess": (44, 60), "boss": (64, 84)}
NPCS = ["client", "bartender", "mama", "astro", "dock", "informant", "vendor", "terminal", "cat", "cat_free"]
NPC_SIZE = (48, 64)
OBJECTS = {  # name: (file, w, h)  keyed objects, drawn over the flat floor
    "crystal": ("tile_crystal_1.png", 60, 60), "crate": ("tile_crate_1.png", 60, 60), "stool": ("tile_stool_1.png", 56, 56),
    "lamp": ("tile_lamp_1.png", 60, 64), "puddle": ("tile_puddle_1.png", 60, 40), "counter": ("tile_counter_obj_1.png", 64, 64),
}
OPAQUE = {   # name: file   full 64x64 tiles
    "hole": "tile_hole_1.png", "bridge": "tile_bridge_2.png", "door": "tile_door_1.png", "gate": "tile_gate_2.png",
}
TEXTURES = {"wall_a": "tex_a_1.png", "wall_b": "tex_b_1.png", "wall_c": "tex_c_1.png", "wall_int": "tex_int_1.png"}
ICONS = {"heart": ("tile_heart_1.png", 28), "coin": ("tile_coin_1.png", 28), "metal": ("tile_metal_1.png", 28), "fur": ("tile_fur_1.png", 40), "heart_big": ("tile_heart_1.png", 40)}

def texture_tile(path):
    """a seamless-looking 64x64 tile from a window-grid texture: crop a whole number of periods"""
    im = Image.open(path).convert("RGB")
    g = im.convert("L")
    w, h = g.size
    def period(vals):
        n = len(vals); m = sum(vals) / n
        v = [x - m for x in vals]
        best, bl = -1e18, 0
        for lag in range(60, 400):
            s = sum(v[i] * v[i + lag] for i in range(0, n - lag, 2))
            if s > best: best, bl = s, lag
        return bl
    cols = [sum(g.crop((x, 0, x + 1, h)).getdata()) / h for x in range(w)]
    rows = [sum(g.crop((0, y, w, y + 1)).getdata()) / w for y in range(h)]
    px, py = period(cols), period(rows)
    kx = max(1, round(480 / px)); ky = max(1, round(480 / py))
    bw, bh = kx * px, ky * py
    x0, y0 = (w - bw) // 2, (h - bh) // 2
    tile = im.crop((x0, y0, x0 + bw, y0 + bh)).resize((64, 64), Image.LANCZOS)
    return tile.convert("RGBA")

def emit(name, img, transparent, out, table):
    pal, data = pack(img, transparent)
    out.append("static const unsigned char art_%s_px[%d] = {%s};" % (name, len(data), ",".join(str(b) for b in data)))
    table.append((name, pal, img.width, img.height, 0 if transparent else 1))

def main():
    preview = None
    if "--preview" in sys.argv: preview = sys.argv[sys.argv.index("--preview") + 1]; os.makedirs(preview, exist_ok=True)
    out, table = [], []
    previews = []
    def keep(name, img, transparent=True):
        emit(name, img, transparent, out, table)
        previews.append((name, img))
    # characters
    for name, (w, h) in CHAR_SIZE.items():
        front = key_alpha(Image.open(f"{RAW}/{name}_front.png"))
        fb = bbox_alpha(front)
        scale = (h - 2) / (fb[3] - fb[1])
        for view in ("front", "back", "side"):
            im = front if view == "front" else key_alpha(Image.open(f"{RAW}/{name}_{view}.png"))
            keep(f"{name}_{ {'front': 'd', 'back': 'u', 'side': 'r'}[view] }", place(im, w, h, scale))
    for name in NPCS:
        im = key_alpha(Image.open(f"{RAW}/{name}_front.png"))
        b = bbox_alpha(im)
        scale = min((NPC_SIZE[1] - 2) / (b[3] - b[1]), NPC_SIZE[0] / (b[2] - b[0]) * 1.0)
        keep(f"npc_{name}", place(im, NPC_SIZE[0], NPC_SIZE[1], scale))
    for name, (f, w, h) in OBJECTS.items():
        p = f"{RAW}/{f}"
        if not os.path.exists(p): continue
        im = key_alpha(Image.open(p)); b = bbox_alpha(im)
        scale = min(w / (b[2] - b[0]), h / (b[3] - b[1]))
        keep(f"obj_{name}", place(im, 64, 64, scale, anchor="center") if name != "puddle" else place(im, 64, 64, scale, anchor="center"))
    for name, f in OPAQUE.items():
        keep(f"tile_{name}", Image.open(f"{RAW}/{f}").convert("RGBA").resize((64, 64), Image.LANCZOS), False)
    for name, f in TEXTURES.items():
        keep(f"tile_{name}", texture_tile(f"{RAW}/{f}"), False)
    for name, (f, s) in ICONS.items():
        im = key_alpha(Image.open(f"{RAW}/{f}")); b = bbox_alpha(im)
        scale = s / max(b[2] - b[0], b[3] - b[1])
        keep(f"icon_{name}", place(im, s, s, scale, anchor="center"))

    lines = ["/* generated by assets/build_assets.py from the FLUX images in assets/raw - do not edit */",
             "typedef struct { unsigned short pal[16]; unsigned char w, h, opaque; const unsigned char *px; } Art;", ""]
    lines += out
    lines.append("")
    for i, (name, pal, w, h, op) in enumerate(table):
        lines.append("#define ART_%s %d" % (name.upper(), i))
    lines.append("#define NART %d" % len(table))
    lines.append("static const Art ARTS[NART] = {")
    for name, pal, w, h, op in table:
        lines.append(" {{%s}, %d, %d, %d, art_%s_px}," % (",".join("0x%04X" % p for p in pal), w, h, op, name))
    lines.append("};")
    open(os.path.join(HERE, "..", "gotanda_art.h"), "w").write("\n".join(lines) + "\n")
    print("wrote gotanda_art.h:", len(table), "images")
    if preview:
        # a contact sheet of the quantised 4bpp result (what the game will draw), 3x enlarged, on a mid-grey floor
        cell = 200
        sheet = Image.new("RGB", (cell * 8, cell * ((len(previews) + 7) // 8)), (60, 70, 100))
        for i, (name, im) in enumerate(previews):
            pal, data = pack(im, im.getchannel("A").getextrema()[0] < 128)
            w, h = im.size
            pix = Image.new("RGBA", (w, h))
            pp = pix.load()
            for j in range(w * h):
                v = (data[j // 2] >> 4) if j % 2 == 0 else (data[j // 2] & 15)
                c = pal[v]
                if v == 15 and im.getchannel("A").getextrema()[0] < 128: pp[j % w, j // w] = (0, 0, 0, 0)
                else: pp[j % w, j // w] = (((c >> 11) & 31) * 255 // 31, ((c >> 5) & 63) * 255 // 63, (c & 31) * 255 // 31, 255)
            sc = min(cell // max(w, h), 3)
            big = pix.resize((w * sc, h * sc), Image.NEAREST)
            sheet.paste(big, ((i % 8) * cell + (cell - big.width) // 2, (i // 8) * cell + (cell - big.height) // 2), big)
        sheet.save(os.path.join(preview, "art_sheet.png"))
        print("preview:", os.path.join(preview, "art_sheet.png"))

if __name__ == "__main__":
    main()
