#!/usr/bin/env python3
"""Tiny client for the FLUX.2 klein server on the A100 box (http://100.104.204.50:8020, async queue).
   flux.py "prompt" out.png [--seed N] [--size 1024] [--ref ref.png ...] [--steps 4]"""
import argparse, base64, json, sys, time, urllib.request

SERVER = "http://100.104.204.50:8020"

def call(path, payload=None):
    req = urllib.request.Request(SERVER + path, data=json.dumps(payload).encode() if payload is not None else None,
                                 headers={"Content-Type": "application/json"})
    with urllib.request.urlopen(req, timeout=120) as r:
        return json.loads(r.read())

def generate(prompt, size=1024, seed=None, refs=(), steps=4, guidance=1.0, w=None, h=None):
    p = {"prompt": prompt, "model": "klein", "width": w or size, "height": h or size, "num_inference_steps": steps,
         "guidance_scale": guidance, "seed": seed}
    if refs:
        p["reference_images"] = [base64.b64encode(open(r, "rb").read()).decode() for r in refs]
    t = call("/generate", p)
    tid = t["task_id"]
    while True:
        s = call("/status/" + tid)
        if s["status"] in ("completed", "done", "success"):
            return s
        if s["status"] in ("failed", "error") or s.get("error"):
            raise RuntimeError(s)
        time.sleep(0.5)

def fetch(res):
    r = res["result"]
    if "image" in r:
        return base64.b64decode(r["image"])
    url = r.get("url") or r.get("image_url") or r.get("path")
    return urllib.request.urlopen(SERVER + url if url.startswith("/") else url, timeout=60).read()

if __name__ == "__main__":
    ap = argparse.ArgumentParser(); ap.add_argument("prompt"); ap.add_argument("out"); ap.add_argument("--seed", type=int)
    ap.add_argument("--size", type=int, default=1024); ap.add_argument("--ref", action="append", default=[]); ap.add_argument("--steps", type=int, default=4)
    ap.add_argument("--guidance", type=float, default=1.0)
    a = ap.parse_args()
    t0 = time.time(); res = generate(a.prompt, a.size, a.seed, a.ref, a.steps, a.guidance)
    open(a.out, "wb").write(fetch(res)); print("saved", a.out, "%.1fs" % (time.time() - t0), json.dumps(res)[:300])
