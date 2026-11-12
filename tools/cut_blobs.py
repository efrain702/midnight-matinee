"""Cut a sheet whose poses touch each other: split by connected shapes instead of empty columns.
usage: python3 tools/cut_blobs.py <sheet.png in art/refs> <name> <height> <n poses> [order...]"""
import sys, os, numpy as np
from PIL import Image
from scipy import ndimage
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import cut_sprites as c

def cut_blobs(sheet, name, height, n, order=None, ref_index=0, scale=None):
    rgba = c.key(Image.open(os.path.join(c.REF, sheet)))
    m = rgba[..., 3] > 30
    m = ndimage.binary_opening(m, iterations=2)          # break hairline bridges between neighbours
    lab, k = ndimage.label(m)
    sizes = ndimage.sum(m, lab, range(1, k + 1))
    big = sorted(range(k), key=lambda i: -sizes[i])[:n]
    # small bits (sparkles, sweat drops, gun tips) join the nearest big pose
    cents = {i: ndimage.center_of_mass(m, lab, i + 1) for i in range(k)}
    owner = np.zeros(k + 1, int)
    for i in range(k):
        owner[i + 1] = min(big, key=lambda b: abs(cents[i][1] - cents[b][1])) + 1 if sizes[i] > 40 else 0
    poses = sorted(big, key=lambda b: cents[b][1])
    frames = []
    for b in poses:
        keep = (owner[lab] == b + 1) & (rgba[..., 3] > 0)
        keep = ndimage.binary_dilation(keep, iterations=2) & (rgba[..., 3] > 0)
        f = rgba.copy(); f[..., 3] = np.where(keep, f[..., 3], 0)
        frames.append(c.trim(f))
    s = scale or height / frames[ref_index].shape[0]
    if order: frames = [frames[i] for i in order]
    for i, f in enumerate(frames):
        Image.fromarray(f, 'RGBA').resize((max(1, int(f.shape[1] * s)), max(1, int(f.shape[0] * s))), Image.LANCZOS) \
             .save(os.path.join(c.OUT, 'chars', f'{name}_{i}.png'))
    print(name, len(frames), 'frames, scale', round(s, 4), 'components', k)
    return s

if __name__ == '__main__':
    a = sys.argv
    cut_blobs(a[1], a[2], int(a[3]), int(a[4]), [int(x) for x in a[5:]] or None)
