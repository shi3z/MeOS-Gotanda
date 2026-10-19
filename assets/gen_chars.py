#!/usr/bin/env python3
"""Generates the character art with FLUX (titan, via flux.py): a front view from a text prompt, then back / side views
by reference editing so the character stays the same.   gen_chars.py [name ...]"""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import flux

STYLE = ("chibi pixel art RPG sprite, 16-bit SNES Zelda style, top-down 3/4 view, big head and small body about 2.5 heads tall, "
         "bold black outline, limited palette, chunky pixels, full body, {desc}, {view}, flat solid {bg} background")
BG = {"magenta": "magenta", "lime": "bright lime green"}
# name: (description, background, seed, views to generate by editing)
CHARS = {
    "player":   ("private detective in tan trench coat and dark fedora hat with glowing cyan sunglasses", "magenta", 11, True),
    "drunk":    ("drunk salaryman with a red flushed face, loosened necktie, grey-green suit, holding a green sake bottle, wobbling", "magenta", 21, True),
    "yakuza":   ("yakuza thug in a black suit, white shirt and red tie, black sunglasses, slicked-back black hair, menacing", "magenta", 31, True),
    "hostess":  ("glamorous nightclub hostess with big blonde updo hair, sparkling hot pink evening dress, holding a high-heel shoe like a weapon", "lime", 41, True),
    "boss":     ("big bulky yakuza boss oyabun with grey slicked hair and a scar, purple kimono jacket open showing a green dragon tattoo on his chest, angry", "magenta", 51, True),
    "client":   ("young woman Hanako with purple hair in a pink dress, worried expression", "lime", 61, False),
    "bartender":("bartender in white shirt, black vest and red bow tie, small moustache, wiping a glass", "magenta", 71, False),
    "mama":     ("elegant nightclub mama in a purple evening dress, big blonde hair, holding a feather fan", "lime", 81, False),
    "astro":    ("old astronomer with white hair and a long white beard, grey robe, holding a small telescope", "magenta", 91, False),
    "dock":     ("dock worker in an orange safety vest and yellow hard hat, strong arms", "magenta", 101, False),
    "informant":("shady informant in a dark hooded cloak and grey hat with sunglasses", "lime", 111, False),
    "vendor":   ("street food vendor in a white apron with a red headband, holding oden skewers", "magenta", 121, False),
    "terminal": ("a standalone retro-futuristic computer terminal console cabinet with a glowing green CRT screen and red buttons, an object only, nobody next to it", "magenta", 131, False),
    "cat_free": ("a cute white cat with red-orange and black koi carp patches sitting and looking up, happy", "magenta", 151, False),
    "cat":      ("a white cat with red-orange and black koi carp patches inside a golden bird cage, cute", "magenta", 141, False),
}

def run(names):
    os.makedirs("raw", exist_ok=True)
    for n in names:
        desc, bg, seed, views = CHARS[n]
        front = f"raw/{n}_front.png"
        if not os.path.exists(front):
            open(front, "wb").write(flux.generate(STYLE.format(desc=desc, view="front view", bg=BG[bg]), seed=seed))
            print("front", n)
        if views:
            for v, text in (("back", "seen from behind, back view, facing away from the camera"),
                            ("side", "seen from the side in profile, side view facing right")):
                out = f"raw/{n}_{v}.png"
                if os.path.exists(out): continue
                prompt = f"The same character {text}. Keep the exact same pixel art style, same colors, same outfit, same proportions. Full body, flat solid {BG[bg]} background."
                open(out, "wb").write(flux.generate(prompt, refs=[front], seed=seed + 1))
                print(v, n)

if __name__ == "__main__":
    run(sys.argv[1:] or list(CHARS))
