"""Turn Runway paintings into per-show art: engine/content/bg/<show>/z0-5.jpg, fg.png, ground.jpg.
usage: python3 tools/process_bg.py <src dir> <show key> <show dir>   (src has <key>z0..z5.png, <key>fg.png, <key>tx.png)"""
import sys, os, numpy as np
from PIL import Image, ImageFilter
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import cut_sprites as c

def zone(src, dst):
    im = Image.open(src).convert('RGB')
    im = im.resize((round(im.width * 1080 / im.height), 1080), Image.LANCZOS)
    im.save(dst, quality=100, subsampling=0)

def ground(src, dst, size=1024):
    im = Image.open(src).convert('RGB').resize((size, size), Image.LANCZOS)
    a = np.asarray(im).astype(np.float32)
    # even out the lighting: divide by a heavy blur, keep the average colour
    blur = np.asarray(im.filter(ImageFilter.GaussianBlur(size // 6))).astype(np.float32) + 1
    a = np.clip(a / blur * blur.reshape(-1, 3).mean(0), 0, 255)
    # make it wrap: blend with a half-shifted copy, so the tile edges come from the middle of the picture
    sh = np.roll(np.roll(a, size // 2, 0), size // 2, 1)
    y, x = np.mgrid[0:size, 0:size] / (size - 1)
    w = np.minimum(np.minimum(x, 1 - x), np.minimum(y, 1 - y)) * 2   # 1 in the middle, 0 at the edges
    w = np.clip(w * 2.2, 0, 1)[..., None]
    out = a * w + sh * (1 - w)
    Image.fromarray(out.astype(np.uint8)).save(dst, quality=92, subsampling=0)

def foreground(src, dst, dark_top=False):
    im0 = Image.open(src); W0, H0 = im0.size
    im0 = im0.crop((int(W0 * 0.012), int(H0 * 0.012), W0 - int(W0 * 0.012), H0 - int(H0 * 0.012)))   # drop any painted border
    rgba = c.key(im0)
    H = rgba.shape[0]
    rgba[int(H * 0.30):int(H * 0.72), :, 3] = 0          # keep the middle clear for the action, whatever the painting did
    if dark_top:   # a painted sky behind the silhouettes: keep only the dark shapes
        top = rgba[:int(H * 0.30)].astype(np.int32); lum = top[..., :3].mean(axis=2)
        a = np.clip((95 - lum) * 6, 0, 255); rgba[:int(H * 0.30), :, 3] = np.minimum(rgba[:int(H * 0.30), :, 3], a).astype(np.uint8)
    im = Image.fromarray(rgba, 'RGBA')
    im = im.resize((round(im.width * 1080 / im.height), 1080), Image.LANCZOS)
    im.save(dst, optimize=True)

if __name__ == '__main__':
    src, key, show = sys.argv[1:4]
    out = os.path.join(c.OUT, 'bg', show); os.makedirs(out, exist_ok=True)
    for i in range(6):
        p = os.path.join(src, f'{key}z{i}.png')
        if os.path.exists(p): zone(p, os.path.join(out, f'z{i}.jpg'))
    p = os.path.join(src, f'{key}fg.png')
    if os.path.exists(p): foreground(p, os.path.join(out, 'fg.png'), dark_top='--dark-top' in sys.argv)
    p = os.path.join(src, f'{key}tx.png')
    if os.path.exists(p): ground(p, os.path.join(out, 'ground.jpg'))
    print(show, sorted(os.listdir(out)))
