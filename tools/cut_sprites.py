"""Cut the Runway sprite sheets (flat green backgrounds) into game-ready PNGs.
usage: python3 tools/cut_sprites.py   (reads art/refs/*.png, writes engine/content/chars and bg)
Each sheet is keyed (green -> transparent, green spill removed), split into poses at empty columns,
trimmed and scaled to the in-game height. Frames are saved as chars/<name>_<i>.png, anchored bottom-centre."""
import numpy as np
from PIL import Image
import os
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REF = os.path.join(ROOT, 'art', 'refs')
OUT = os.path.join(ROOT, 'engine', 'content')

def key(im):
    a = np.asarray(im.convert('RGB')).astype(np.int32)
    r, g, b = a[..., 0], a[..., 1], a[..., 2]
    green = (g > 120) & (g > r * 1.35) & (g > b * 1.35)
    soft = (g > 90) & (g > r * 1.12) & (g > b * 1.12) & ~green
    alpha = np.where(green, 0, np.where(soft, 140, 255)).astype(np.uint8)
    g2 = np.minimum(g, np.maximum(r, b) + 10)          # despill
    rgba = np.dstack([r, g2, b, alpha]).astype(np.uint8)
    return rgba

def split(rgba, min_w=30, gap=6):
    occ = (rgba[..., 3] > 200).sum(axis=0) > 2
    runs, x, W = [], 0, len(occ)
    while x < W:
        if occ[x]:
            s = x
            empty = 0
            while x < W and empty < gap:
                empty = 0 if occ[x] else empty + 1
                x += 1
            runs.append((s, x - empty))
        else:
            x += 1
    return [r for r in runs if r[1] - r[0] >= min_w]

def trim(rgba):
    m = rgba[..., 3] > 30
    ys, xs = np.where(m)
    return rgba[ys.min():ys.max() + 1, xs.min():xs.max() + 1]

def biggest_blob_only(rgba):
    from scipy import ndimage
    m = rgba[..., 3] > 30
    lab, n = ndimage.label(m)
    if n <= 1: return rgba
    sizes = ndimage.sum(m, lab, range(1, n + 1))
    keep = np.zeros_like(m)
    big = sizes.max()
    for i, s in enumerate(sizes):
        if s > big * 0.04: keep |= lab == i + 1     # keep the body and any big held props, drop specks
    out = rgba.copy(); out[..., 3] = np.where(keep, out[..., 3], 0)
    return out

def cut(sheet, name, height, y0=0.0, y1=1.0, expect=None, pick=None, flip=False):
    im = Image.open(os.path.join(REF, sheet))
    W, H = im.size
    im = im.crop((0, int(H * y0), W, int(H * y1)))
    rgba = key(im)
    runs = split(rgba)
    if expect and len(runs) != expect: print('  warning', name, 'found', len(runs), 'poses, expected', expect)
    frames = [biggest_blob_only(rgba[:, a:b]) for a, b in runs]
    frames = [trim(f) for f in frames]
    if pick: frames = [frames[i] for i in pick if i < len(frames)]
    ref_h = max(f.shape[0] for f in frames)
    s = height / ref_h
    os.makedirs(os.path.join(OUT, 'chars'), exist_ok=True)
    for i, f in enumerate(frames):
        img = Image.fromarray(f, 'RGBA')
        img = img.resize((max(1, int(img.width * s)), max(1, int(img.height * s))), Image.LANCZOS)
        if flip: img = img.transpose(Image.FLIP_LEFT_RIGHT)
        img.save(os.path.join(OUT, 'chars', f'{name}_{i}.png'))
    print(name, len(frames), 'frames, scale', round(s, 3))

if __name__ == '__main__':
    # hero faces right: idle, idle2, run a, run b, jump, aim diagonal, aim up
    cut('hero_sheet.png', 'hero', 150, 0.0, 0.52, expect=7)
    # enemies face left in the art: flip them so every sprite faces right (the game mirrors as needed)
    im = Image.open(os.path.join(REF, 'enemy_sheet.png')); W, H = im.size
    rgba = key(im); runs = split(rgba)
    print('enemy poses', len(runs))
    names = [('usher', 160), ('usher', 160), ('projdog', 120), ('projdog', 120), ('blob', 70), ('blob', 70)]
    groups = {}
    for (a, b), (n, h) in zip(runs, names): groups.setdefault(n, []).append((trim(biggest_blob_only(rgba[:, a:b])), h))
    for n, fr in groups.items():
        ref_h = max(f.shape[0] for f, _ in fr)
        for i, (f, h) in enumerate(fr):
            img = Image.fromarray(f, 'RGBA'); s = h / ref_h
            img = img.resize((int(img.width * s), int(img.height * s)), Image.LANCZOS).transpose(Image.FLIP_LEFT_RIGHT)
            img.save(os.path.join(OUT, 'chars', f'{n}_{i}.png'))
        print(n, len(fr))
    cut('boss_sheet.png', 'inky', 430, 0.05, 1.0, expect=3, flip=True)
    # background: fit the painting to the TV height
    bg = Image.open(os.path.join(REF, 'bg_level1.png')).convert('RGB')
    bg = bg.resize((int(bg.width * 720 / bg.height), 720), Image.LANCZOS)
    os.makedirs(os.path.join(OUT, 'bg'), exist_ok=True)
    bg.save(os.path.join(OUT, 'bg', 'theater.jpg'), quality=88)
    print('bg', bg.size)
