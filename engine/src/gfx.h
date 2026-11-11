#ifndef GFX_H
#define GFX_H
#include <SDL.h>

#define SCREEN_W 1280
#define SCREEN_H 720

typedef struct { SDL_Texture *tex; int w, h; } Tex;
typedef struct { short page, x, y, w, h, ax, ay; } Frame;
typedef struct { char name[16]; int n, base, norig; short orig[32]; Frame *f; } Anim;   /* orig: where the originally tuned frames sit after in-betweens were added */   /* base: frame count the timing was tuned for */
typedef struct {
    SDL_Texture *pages[4]; int npages;
    Anim anims[16]; int nanims;
    float scale;
} Sprite;

typedef struct { int x, y, w, h, adv; } Glyph;
typedef struct { Tex tex; Glyph g[128]; int size; } Font;

extern SDL_Renderer *R;

const char *content_path(const char *rel);
void gfx_init(SDL_Renderer *r);
Tex tex_load(const char *rel);
void tex_free(Tex *t);
int sprite_load(Sprite *s, const char *name);
Anim *sprite_anim(Sprite *s, const char *name);
/* draw frame so its anchor (feet / head centre) lands on x,y; flip mirrors horizontally */
void draw_frame(Sprite *s, Anim *a, int idx, float x, float y, int flip, Uint8 alpha, Uint8 tint);
/* scaled and shaded (shade 255 = normal colours, lower = darker, for things far in the background) */
void draw_frame_far(Sprite *s, Anim *a, int idx, float x, float y, int flip, Uint8 alpha, float scale, Uint8 shade);
void draw_tex(Tex *t, float x, float y, float w, float h, int flip, Uint8 alpha);
void draw_tex_rot(Tex *t, float cx, float cy, float w, float h, double angle, int flip, Uint8 alpha);
void draw_tex_src(Tex *t, SDL_Rect *src, float x, float y, float w, float h, Uint8 alpha);
void fill_rect(int x, int y, int w, int h, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
int font_load(Font *f, const char *name);
int text_width(Font *f, const char *s, float scale);
void font_set_small(Font *main, Font *small);
void draw_text(Font *f, const char *s, float x, float y, float scale, int centered, Uint8 alpha);
/* Midnight Matinee additions: switch screens (TV / GamePad), code-drawn shapes */
void gfx_set_renderer(SDL_Renderer *r);
Tex tex_from_surface(SDL_Surface *s);
Tex make_circle_tex(int radius);                 /* white disc with soft edge, for ellipses, glows, eyes */
void draw_ellipse(Tex *disc, float cx, float cy, float rx, float ry, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
void draw_glow(Tex *disc, float cx, float cy, float rad, Uint8 r, Uint8 g, Uint8 b, Uint8 a);   /* additive */
void draw_bar(float x0, float y0, float x1, float y1, float thick, Uint8 r, Uint8 g, Uint8 b, Uint8 a); /* rotated thick line */
void draw_tex_tint(Tex *t, float x, float y, float w, float h, int flip, Uint8 r, Uint8 g, Uint8 b, Uint8 a, double angle);
void draw_text_col(Font *f, const char *s, float x, float y, float scale, int centered, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
#endif
