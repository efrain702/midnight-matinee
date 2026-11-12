"""Build the small show posters (show select, loading card, gallery) from each show's boss-room painting and boss sprite.
Usage: python3 tools/make_posters.py   (run from the repo root)"""
from PIL import Image, ImageFilter, ImageEnhance
DIRS = ['lobby', 'pizza', 'reels', 'ink', 'lost', 'mirrors', 'roof', 'encore']
BOSS = ['inky', 'rat', 'splicer', 'press', 'collector', 'vanity', 'headliner', 'unbound']
W, H = 640, 360
for i, (d, b) in enumerate(zip(DIRS, BOSS)):
    bg = ImageEnhance.Brightness(Image.open(f'engine/content/bg/{d}/z5.jpg').convert('RGB').resize((W, H), Image.LANCZOS)).enhance(0.62)
    grad = Image.linear_gradient('L').rotate(90, expand=True).resize((W, H)).point(lambda v: int(max(0, 170 - v * 0.95)))
    bg = Image.composite(Image.new('RGB', (W, H), (10, 4, 4)), bg, grad).convert('RGBA')
    sp = Image.open(f'engine/content/chars/{b}_0.png').convert('RGBA'); sp = sp.crop(sp.getbbox())
    h = int(H * 0.92); w = int(sp.width * h / sp.height)
    if w > W * 0.62: w = int(W * 0.62); h = int(sp.height * w / sp.width)
    sp = sp.resize((w, h), Image.LANCZOS)
    glow = Image.new('RGBA', sp.size, (255, 190, 90, 0)); glow.putalpha(sp.split()[3].filter(ImageFilter.GaussianBlur(10)).point(lambda v: int(v * 0.55)))
    x, y = W - w - 10, H - h + 6
    bg.alpha_composite(glow, (x, y)); bg.alpha_composite(sp, (x, y))
    bg.convert('RGB').save(f'engine/content/ui/posters/{i}.jpg', quality=90)
