#!/usr/bin/env python3
"""Generates tile / object / item art with FLUX (titan, via flux.py).   gen_tiles.py [name ...]
opaque tiles fill the whole square; objects are drawn on a flat magenta background (keyed out later)."""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import flux

BASE = "pixel art game tile, 16-bit SNES Zelda style, top-down 3/4 view, bold dark outline, limited palette, chunky pixels, "
OPAQUE = "square tile that fills the entire image edge to edge, "
OBJ = "single object centered, flat solid magenta background"
# name: (prompt, seeds)
TILES = {
    "wall_a":   (OPAQUE + "front view of a cyberpunk neon city building facade, dark purple wall with rows of lit yellow windows and a glowing pink neon strip along the bottom edge", (1, 2)),
    "wall_b":   (OPAQUE + "front view of a cyberpunk neon city building facade, dark blue wall with lit warm windows and a glowing cyan neon strip along the bottom edge", (3, 4)),
    "wall_c":   (OPAQUE + "front view of a cyberpunk neon city building facade, dark indigo wall with lit windows, small shop awning and a glowing yellow neon strip along the bottom edge", (5, 6)),
    "wall_int": (OPAQUE + "interior wall of a Japanese bar, dark wine-red plaster wall with a wooden baseboard and a little paper lantern", (7, 8)),
    "crystal":  ("a cluster of glowing blue ice crystals, " + OBJ, (1, 2)),
    "crate":    ("a wooden cargo crate with metal corners, " + OBJ, (1, 2)),
    "counter":  (OPAQUE + "a polished wooden bar counter seen from the front, glossy top edge, wooden panels, horizontally tileable", (1, 2)),
    "stool":    ("a round red cushioned bar stool with chrome leg, " + OBJ, (1, 2)),
    "hole":     (OPAQUE + "a pit in cracked ice: a deep black void with a jagged icy blue rim, seen from above", (1, 2)),
    "bridge":   (OPAQUE + "a bridge of pale blue ice planks over a dark pit, seen from above, planks side by side", (1, 2)),
    "lamp":     ("a tall neon street lamp post with a warm glowing yellow lantern, " + OBJ, (1, 2)),
    "door":     (OPAQUE + "front view of a building entrance, a lit orange-yellow open doorway with a dark interior and a small neon sign above, framed by dark purple wall", (1, 2)),
    "gate":     (OPAQUE + "front view of a yakuza compound iron gate with gold bars and red paper lanterns on a dark wall", (1, 2)),
    "puddle":   ("a puddle of water reflecting pink and cyan neon light, seen from above, " + OBJ, (1, 2)),
    "heart":    ("a shiny red heart, game pickup icon, " + OBJ, (1,)),
    "coin":     ("a shiny gold coin with a yen mark, game pickup icon, " + OBJ, (1,)),
    "metal":    ("a pile of steel scrap plates and bolts, game pickup icon, " + OBJ, (1,)),
    "fur":      ("a small tuft of white cat fur with orange and black patches, game pickup icon, " + OBJ, (1,)),
}

def run(names):
    for n in names:
        prompt, seeds = TILES[n]
        for s in seeds:
            out = f"raw/tile_{n}_{s}.png"
            if os.path.exists(out): continue
            open(out, "wb").write(flux.generate(BASE + prompt, seed=s * 100 + sum(map(ord, n)) % 97))
            print(n, s)

if __name__ == "__main__":
    run(sys.argv[1:] or list(TILES))
