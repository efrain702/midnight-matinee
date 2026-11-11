#include "gfx.h"
#include <SDL_image.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

SDL_Renderer *R;

#ifdef __WIIU__
static const char *BASE = "fs:/vol/content/";
#else
static const char *BASE = NULL;
#endif

const char *content_path(const char *rel) {
    static char buf[4][512]; static int k = 0;
    k = (k + 1) & 3;
    const char *base = BASE;
    if (!base) { base = getenv("MM_CONTENT"); if (!base) base = "engine/content/"; }
    snprintf(buf[k], sizeof buf[k], "%s%s", base, rel);
    return buf[k];
}

void gfx_init(SDL_Renderer *r) { R = r; IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG); }

Tex tex_load(const char *rel) {
    Tex t = {0};
    SDL_Surface *s = IMG_Load(content_path(rel));
    if (!s) { SDL_Log("missing %s: %s", rel, IMG_GetError()); return t; }
    t.tex = SDL_CreateTextureFromSurface(R, s);
    SDL_SetTextureBlendMode(t.tex, SDL_BLENDMODE_BLEND);
    t.w = s->w; t.h = s->h;
    SDL_FreeSurface(s);
    return t;
}

void tex_free(Tex *t) { if (t->tex) SDL_DestroyTexture(t->tex); t->tex = NULL; }

int sprite_load(Sprite *s, const char *name) {
    char rel[128]; memset(s, 0, sizeof *s);
    snprintf(rel, sizeof rel, "chars/%s.txt", name);
    SDL_RWops *rw = SDL_RWFromFile(content_path(rel), "rb");
    if (!rw) { SDL_Log("missing %s", rel); return 0; }
    Sint64 n = SDL_RWsize(rw); char *txt = malloc(n + 1);
    SDL_RWread(rw, txt, 1, n); txt[n] = 0; SDL_RWclose(rw);
    char *line = strtok(txt, "\n"); Anim *cur = NULL; int fi = 0;
    while (line) {
        if (!strncmp(line, "pages", 5)) sscanf(line + 6, "%d", &s->npages);
        else if (!strncmp(line, "scale", 5)) sscanf(line + 6, "%f", &s->scale);
        else if (!strncmp(line, "anim", 4)) {
            if (s->nanims >= 16) { cur = NULL; line = strtok(NULL, "\n"); continue; }
            cur = &s->anims[s->nanims++];
            cur->base = 0; cur->norig = 0; sscanf(line + 5, "%15s %d %d", cur->name, &cur->n, &cur->base); if (cur->base <= 0) cur->base = cur->n;
            cur->f = calloc(cur->n, sizeof(Frame)); fi = 0;
        } else if (line[0] == 'o' && line[1] == ' ' && cur) {
            char *q = line + 2; int v, used;
            while (cur->norig < 32 && sscanf(q, "%d%n", &v, &used) == 1) { cur->orig[cur->norig++] = (short)v; q += used; }
        } else if (line[0] == 'f' && cur && fi < cur->n) {
            int p, x, y, w, h, ax, ay;
            sscanf(line + 2, "%d %d %d %d %d %d %d", &p, &x, &y, &w, &h, &ax, &ay);
            cur->f[fi++] = (Frame){p, x, y, w, h, ax, ay};
        }
        line = strtok(NULL, "\n");
    }
    free(txt);
    for (int i = 0; i < s->npages; i++) {
        snprintf(rel, sizeof rel, "chars/%s_%d.png", name, i);
        s->pages[i] = tex_load(rel).tex;
    }
    return 1;
}

Anim *sprite_anim(Sprite *s, const char *name) {
    for (int i = 0; i < s->nanims; i++) if (!strcmp(s->anims[i].name, name)) return &s->anims[i];
    if (s->nanims > 0) return &s->anims[0];
    static Anim none; none.n = 1; none.base = 1;      /* missing art: draw nothing instead of crashing */
    return &none;
}

void draw_frame_far(Sprite *s, Anim *a, int idx, float x, float y, int flip, Uint8 alpha, float scale, Uint8 shade) {
    if (!a || a->n == 0 || s->nanims == 0) return;
    if (idx < 0) idx = 0; if (idx >= a->n) idx = a->n - 1;
    Frame *f = &a->f[idx];
    SDL_Texture *t = s->pages[f->page];
    SDL_Rect src = {f->x, f->y, f->w, f->h};
    SDL_FRect dst = {flip ? x - (f->w - f->ax) * scale : x - f->ax * scale, y - f->ay * scale, f->w * scale, f->h * scale};
    SDL_SetTextureAlphaMod(t, alpha);
    SDL_SetTextureColorMod(t, shade, shade, (Uint8)(shade + (255 - shade) / 4));   /* a touch of blue haze */
    SDL_RenderCopyExF(R, t, &src, &dst, 0, NULL, flip ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
    SDL_SetTextureColorMod(t, 255, 255, 255);
}

void draw_frame(Sprite *s, Anim *a, int idx, float x, float y, int flip, Uint8 alpha, Uint8 tint) {
    if (!a || a->n == 0) return;
    if (idx < 0) idx = 0; if (idx >= a->n) idx = a->n - 1;
    Frame *f = &a->f[idx];
    if (s->nanims == 0) return;
    SDL_Texture *t = s->pages[f->page];
    SDL_Rect src = {f->x, f->y, f->w, f->h};
    SDL_FRect dst;
    dst.w = f->w; dst.h = f->h;
    dst.y = y - f->ay;
    dst.x = flip ? x - (f->w - f->ax) : x - f->ax;
    SDL_SetTextureAlphaMod(t, alpha);
    if (tint) SDL_SetTextureColorMod(t, 255, 255 - tint, 255 - tint); else SDL_SetTextureColorMod(t, 255, 255, 255);
    SDL_RenderCopyExF(R, t, &src, &dst, 0, NULL, flip ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
    if (tint > 128) { /* white flash: additive second pass */
        SDL_SetTextureBlendMode(t, SDL_BLENDMODE_ADD);
        SDL_SetTextureAlphaMod(t, (Uint8)((tint - 128) * 2 * alpha / 255));
        SDL_SetTextureColorMod(t, 255, 255, 255);
        SDL_RenderCopyExF(R, t, &src, &dst, 0, NULL, flip ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
        SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);
    }
    SDL_SetTextureColorMod(t, 255, 255, 255);
}

void draw_tex(Tex *t, float x, float y, float w, float h, int flip, Uint8 alpha) {
    if (!t->tex) return;
    SDL_FRect d = {x, y, w, h};
    SDL_SetTextureAlphaMod(t->tex, alpha);
    SDL_RenderCopyExF(R, t->tex, NULL, &d, 0, NULL, flip ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
}

void draw_tex_rot(Tex *t, float cx, float cy, float w, float h, double angle, int flip, Uint8 alpha) {
    if (!t->tex) return;
    SDL_FRect d = {cx - w / 2, cy - h / 2, w, h};
    SDL_SetTextureAlphaMod(t->tex, alpha);
    SDL_RenderCopyExF(R, t->tex, NULL, &d, angle, NULL, flip ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
}

void draw_tex_src(Tex *t, SDL_Rect *src, float x, float y, float w, float h, Uint8 alpha) {
    if (!t->tex) return;
    SDL_FRect d = {x, y, w, h};
    SDL_SetTextureAlphaMod(t->tex, alpha);
    SDL_RenderCopyF(R, t->tex, src, &d);
}

void fill_rect(int x, int y, int w, int h, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    SDL_SetRenderDrawBlendMode(R, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(R, r, g, b, a);
    SDL_Rect rc = {x, y, w, h};
    SDL_RenderFillRect(R, &rc);
}

int font_load(Font *f, const char *name) {
    char rel[128]; memset(f, 0, sizeof *f);
    snprintf(rel, sizeof rel, "ui/%s.png", name); f->tex = tex_load(rel);
    snprintf(rel, sizeof rel, "ui/%s.txt", name);
    SDL_RWops *rw = SDL_RWFromFile(content_path(rel), "rb");
    if (!rw) return 0;
    Sint64 n = SDL_RWsize(rw); char *txt = malloc(n + 1);
    SDL_RWread(rw, txt, 1, n); txt[n] = 0; SDL_RWclose(rw);
    char *line = strtok(txt, "\n");
    if (line) f->size = atoi(line);
    while ((line = strtok(NULL, "\n"))) {
        int c, x, y, w, h, adv;
        if (sscanf(line, "%d %d %d %d %d %d", &c, &x, &y, &w, &h, &adv) == 6 && c < 128)
            f->g[c] = (Glyph){x, y, w, h, adv};
    }
    free(txt);
    return 1;
}

int text_width(Font *f, const char *s, float scale) {
    float w = 0;
    for (; *s; s++) w += f->g[(unsigned char)*s & 127].adv * scale;
    return (int)w;
}

static Font *g_main_font, *g_small_font;
void font_set_small(Font *main, Font *small) { g_main_font = main; g_small_font = small->tex.tex ? small : NULL; }

static void text_pass(Font *f, const char *s, float x, float y, float sx, float sy, Uint8 alpha, int shadow, Uint8 *tint) {
    SDL_SetTextureAlphaMod(f->tex.tex, alpha);
    SDL_SetTextureColorMod(f->tex.tex, shadow ? 0 : tint[0], shadow ? 0 : tint[1], shadow ? 0 : tint[2]);
    for (; *s; s++) {
        Glyph *g = &f->g[(unsigned char)*s & 127];
        if (g->w > 0 && *s != ' ') {
            SDL_Rect src = {g->x, g->y, g->w, g->h};
            SDL_FRect d = {x, y, g->w * sx, g->h * sy};
            SDL_RenderCopyF(R, f->tex.tex, &src, &d);
        }
        x += g->adv * sx;
    }
    SDL_SetTextureColorMod(f->tex.tex, tint[0], tint[1], tint[2]);
}

void draw_text(Font *f, const char *s, float x, float y, float scale, int centered, Uint8 alpha) {
    Uint8 tint[3]; SDL_GetTextureColorMod(f->tex.tex, &tint[0], &tint[1], &tint[2]);   /* callers may tint the font (book ink) */
    float w = text_width(f, s, scale);
    if (centered) x -= w / 2.0f;
    float sx = scale, sy = scale;
    if (f == g_main_font && g_small_font && scale < 0.72f) {
        /* small text: the sturdier outlined face, squeezed to exactly the same width so layouts don't move */
        Font *sf = g_small_font;
        sy = scale * f->size / (float)sf->size;
        float sw = text_width(sf, s, sy);
        sx = sw > 0 ? sy * (w / sw < 1.0f ? w / sw : 1.0f) : sy;
        if (sw * sx / sy < w) x += (w - sw * sx / sy) / 2;      /* narrower: keep it centred in the old box */
        f = sf;
    }
    float off = 2.0f + 2.0f * scale;                              /* soft drop shadow lifts text off busy art */
    if (tint[0] + tint[1] + tint[2] > 300) text_pass(f, s, x + off, y + off, sx, sy, (Uint8)(alpha * 0.6f), 1, tint);   /* no shadow under dark ink */
    text_pass(f, s, x, y, sx, sy, alpha, 0, tint);
    if (f != g_main_font && g_main_font) SDL_SetTextureColorMod(f->tex.tex, 255, 255, 255);
}

/* ---- Midnight Matinee additions ---- */
#include <math.h>
static SDL_Texture *g_white = NULL; static SDL_Renderer *g_white_r = NULL;
static SDL_Texture *white_px(void) {
    if (g_white && g_white_r == R) return g_white;
    SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, 4, 4, 32, SDL_PIXELFORMAT_RGBA32);
    SDL_FillRect(s, NULL, SDL_MapRGBA(s->format, 255, 255, 255, 255));
    g_white = SDL_CreateTextureFromSurface(R, s); g_white_r = R; SDL_FreeSurface(s);
    SDL_SetTextureBlendMode(g_white, SDL_BLENDMODE_BLEND);
    return g_white;
}
void gfx_set_renderer(SDL_Renderer *r) { R = r; }
Tex tex_from_surface(SDL_Surface *s) {
    Tex t = {0}; if (!s) return t;
    t.tex = SDL_CreateTextureFromSurface(R, s); SDL_SetTextureBlendMode(t.tex, SDL_BLENDMODE_BLEND);
    t.w = s->w; t.h = s->h; return t;
}
Tex make_circle_tex(int rad) {
    int d = rad * 2;
    SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, d, d, 32, SDL_PIXELFORMAT_RGBA32);
    Uint32 *px = (Uint32 *)s->pixels;
    for (int y = 0; y < d; y++) for (int x = 0; x < d; x++) {
        float dx = x + 0.5f - rad, dy = y + 0.5f - rad, r = sqrtf(dx * dx + dy * dy);
        float a = rad - r; a = a < 0 ? 0 : a > 1.5f ? 1 : a / 1.5f;
        px[y * (s->pitch / 4) + x] = SDL_MapRGBA(s->format, 255, 255, 255, (Uint8)(a * 255));
    }
    Tex t = tex_from_surface(s); SDL_FreeSurface(s); return t;
}
void draw_ellipse(Tex *disc, float cx, float cy, float rx, float ry, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    if (!disc->tex) return;
    SDL_FRect d = {cx - rx, cy - ry, rx * 2, ry * 2};
    SDL_SetTextureColorMod(disc->tex, r, g, b); SDL_SetTextureAlphaMod(disc->tex, a);
    SDL_RenderCopyF(R, disc->tex, NULL, &d);
}
void draw_glow(Tex *disc, float cx, float cy, float rad, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    if (!disc->tex) return;
    SDL_SetTextureBlendMode(disc->tex, SDL_BLENDMODE_ADD);
    for (int i = 0; i < 3; i++) {
        float k = 1.0f - i * 0.3f;
        draw_ellipse(disc, cx, cy, rad * k, rad * k, r, g, b, (Uint8)(a / 3));
    }
    SDL_SetTextureBlendMode(disc->tex, SDL_BLENDMODE_BLEND);
}
void draw_bar(float x0, float y0, float x1, float y1, float thick, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    SDL_Texture *w = white_px();
    float dx = x1 - x0, dy = y1 - y0, len = sqrtf(dx * dx + dy * dy);
    SDL_FRect d = {(x0 + x1) / 2 - len / 2, (y0 + y1) / 2 - thick / 2, len, thick};
    SDL_SetTextureColorMod(w, r, g, b); SDL_SetTextureAlphaMod(w, a);
    SDL_RenderCopyExF(R, w, NULL, &d, atan2f(dy, dx) * 57.29578f, NULL, SDL_FLIP_NONE);
}
void draw_tex_tint(Tex *t, float x, float y, float w, float h, int flip, Uint8 r, Uint8 g, Uint8 b, Uint8 a, double angle) {
    if (!t->tex) return;
    SDL_FRect d = {x, y, w, h};
    SDL_SetTextureColorMod(t->tex, r, g, b); SDL_SetTextureAlphaMod(t->tex, a);
    SDL_RenderCopyExF(R, t->tex, NULL, &d, angle, NULL, flip ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
    SDL_SetTextureColorMod(t->tex, 255, 255, 255);
}
void draw_text_col(Font *f, const char *s, float x, float y, float scale, int centered, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    if (!f->tex.tex) return;
    SDL_SetTextureColorMod(f->tex.tex, r, g, b);
    draw_text(f, s, x, y, scale, centered, a);
    SDL_SetTextureColorMod(f->tex.tex, 255, 255, 255);
}
