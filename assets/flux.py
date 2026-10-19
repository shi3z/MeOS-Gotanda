#!/usr/bin/env python3
"""Client for the FLUX.2 klein 4B server on `titan` (RTX 3090), reached through an SSH tunnel:

    ssh -f -N -L 8501:127.0.0.1:8501 shi3z@titan
    flux.py "prompt" out.png [--seed N] [--size 1024] [--ref ref.png ...] [--steps 4]

(~2-3 s per 1024px image.)  Start the server on titan with:  cd ~/flux2-klein4b && .venv/bin/python server.py --port 8501
"""
import argparse, base64, json, sys, time, urllib.request

SERVER = "http://127.0.0.1:8501"

def generate(prompt, w=1024, h=1024, seed=-1, refs=(), steps=4, guidance=1.0):
    body = {"prompt": prompt, "width": w, "height": h, "steps": steps, "seed": seed, "guidance_scale": guidance}
    if refs:
        body["images"] = [base64.b64encode(open(r, "rb").read()).decode() for r in refs]
    req = urllib.request.Request(SERVER + "/generate", data=json.dumps(body).encode(), headers={"Content-Type": "application/json"})
    with urllib.request.urlopen(req, timeout=300) as r:
        return r.read()

if __name__ == "__main__":
    ap = argparse.ArgumentParser(); ap.add_argument("prompt"); ap.add_argument("out"); ap.add_argument("--seed", type=int, default=-1)
    ap.add_argument("--size", type=int, default=1024); ap.add_argument("--ref", action="append", default=[]); ap.add_argument("--steps", type=int, default=4)
    a = ap.parse_args()
    t0 = time.time(); png = generate(a.prompt, a.size, a.size, a.seed, a.ref, a.steps)
    open(a.out, "wb").write(png); print("saved", a.out, "%.1fs" % (time.time() - t0))
