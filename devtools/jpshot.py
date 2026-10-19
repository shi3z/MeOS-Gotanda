#!/usr/bin/env python3
"""Screenshots of a MeOS app with Japanese text, from the SDK simulator (which prints non-ASCII as '?').

    jpshot.py APP.c --frames N --shot 10,50 --keys "25:enter,30:right" --out DIR [--define DEBUG_ROOM=5] [--font PATH]

Compiles a copy of the simulator that also logs every text call containing non-ASCII characters, runs the app, and
draws those characters over the PNGs with a system CJK font.  Needs: cc, pillow.  Set SDK to the MeOS-app-sdk directory.
"""
import argparse, os, re, subprocess, sys, tempfile
from PIL import Image, ImageDraw, ImageFont

ap = argparse.ArgumentParser()
ap.add_argument("app"); ap.add_argument("--frames", default="120"); ap.add_argument("--shot", default="")
ap.add_argument("--keys", default=""); ap.add_argument("--out", default="shots"); ap.add_argument("--define", action="append", default=[])
ap.add_argument("--font", default="/usr/share/fonts/opentype/noto/NotoSansCJK-Bold.ttc")
a = ap.parse_args()
sdk = os.environ.get("SDK", os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "meos-sdk-src"))
src = open(os.path.join(sdk, "sim", "meos_sim.c")).read()
hook = '''
    { int na = 0; for (int q = 0; q < len; q++) if ((unsigned char)s[q] >= 0x80) na = 1;
      if (na) { static FILE *lf; if (!lf) lf = fopen(getenv("MEOS_TEXTLOG"), "w");
        fprintf(lf, "%d %d %d %d %u %u ", frame, x, y, sc, fg, bg); for (int q = 0; q < len; q++) fprintf(lf, "%02x", (unsigned char)s[q]); fprintf(lf, "\\n"); fflush(lf); } }
'''
marker = "    if (len < 0 || len > 255 || sc < 1 || sc > 8) return -3;\n"
assert marker in src
src = src.replace(marker, marker + hook, 1)
tmp = tempfile.mkdtemp()
open(os.path.join(tmp, "sim_jp.c"), "w").write(src)
binp = os.path.join(tmp, "sim")
cmd = ["cc", "-O1", "-w", "-D_start=meos_app_start", "-I" + os.path.join(sdk, "include"), "-I" + os.path.join(sdk, "sim")]
cmd += ["-D" + d for d in a.define] + ["-o", binp, os.path.join(tmp, "sim_jp.c"), a.app]
subprocess.check_call(cmd)
os.makedirs(a.out, exist_ok=True)
name = os.path.splitext(os.path.basename(a.app))[0]
man = a.app[:-2] + ".manifest"
perms = re.search(r"^perms=([A-Za-z]*)", open(man).read(), re.M).group(1) if os.path.exists(man) else ""
log = os.path.join(tmp, "text.log")
env = dict(os.environ, MEOS_PERMS=perms, MEOS_KEYS=a.keys, MEOS_FRAMES=a.frames, MEOS_SHOTS=a.shot, MEOS_OUT=os.path.join(a.out, name), MEOS_TEXTLOG=log)
subprocess.run([binp], env=env, stderr=subprocess.DEVNULL, check=True)
entries = []
if os.path.exists(log):
    for l in open(log):
        f, x, y, sc, fg, bg, hx = l.split()
        entries.append((int(f), int(x), int(y), int(sc), int(fg), int(bg), bytes.fromhex(hx).decode("utf-8", "replace")))
def rgb(p): return (((p >> 11) & 31) * 255 // 31, ((p >> 5) & 63) * 255 // 63, (p & 31) * 255 // 31)
# The simulator draws every non-ASCII character as '?'.  A logged character is still on screen if its cell in the
# screenshot is exactly that '?' (same colours): later fills, sprites or text overwrite it, so stale entries drop out.
font = [[int(v, 16) for v in re.findall(r"0x([0-9A-Fa-f]{2})", l)] for l in open(os.path.join(sdk, "sim", "font8x8.h")) if l.strip().startswith("{0x")]
QMARK = font[ord("?") - 0x20]
def qcell(sc, fg, bg):
    im = Image.new("RGB", (8 * sc, 8 * sc))
    px = im.load()
    for r in range(8 * sc):
        for c in range(8 * sc):
            px[c, r] = fg if (QMARK[r // sc] >> (c // sc)) & 1 else bg
    return im
for f in [int(v) for v in a.shot.split(",") if v]:
    ppm = os.path.join(a.out, "%s_%04d.ppm" % (name, f))
    img = Image.open(ppm).convert("RGB"); os.remove(ppm)
    base = img.copy()
    d = ImageDraw.Draw(img)
    for (fr, x, y, sc, fg, bg, s) in entries:
        if fr >= f: continue
        cell = 8 * sc; font_img = ImageFont.truetype(a.font, cell, index=0)
        ref = qcell(sc, rgb(fg), rgb(bg))
        for i, ch in enumerate(s):
            if ord(ch) < 0x80: continue
            cx = x + i * cell
            if cx < 0 or y < 0 or cx + cell > img.width or y + cell > img.height: continue
            if base.crop((cx, y, cx + cell, y + cell)).tobytes() != ref.tobytes(): continue
            d.rectangle([cx, y, cx + cell - 1, y + cell - 1], fill=rgb(bg))
            d.text((cx + cell // 2, y + cell // 2), ch, font=font_img, fill=rgb(fg), anchor="mm")
    img.save(os.path.join(a.out, "%s_%04d.png" % (name, f)))
    print("wrote", os.path.join(a.out, "%s_%04d.png" % (name, f)))
