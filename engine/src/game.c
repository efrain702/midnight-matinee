/* MIDNIGHT MATINEE - Level 1 "Opening Night"
 * Run-and-gun through Uncle Inky's Jamboree Palace. The GamePad is the security office:
 * camera feeds, PING (mark enemies) and LIGHTS (flood a zone, scare the Ink Fiend).
 */
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "gfx.h"
#include "audio.h"
#include "game.h"

#define TILE 48
#define LW 240
#define LH 15
#define FLOOR_Y (13 * TILE)
#define DT (1.0f / 60.0f)
#define ARENA_X (212 * TILE)

static float frand(void) { return rand() / (float)RAND_MAX; }
static float clampf(float v, float a, float b) { return v < a ? a : v > b ? b : v; }
static float signf(float v) { return v < 0 ? -1.0f : 1.0f; }

/* ------------------------------------------------------------ assets */
#define HERO_FRAMES 8
typedef struct { Tex hero[3][HERO_FRAMES]; Tex usher[5], dog[5], blob[4], inky[3], gore[12], goat[5], meat[5], slice[5], rat[5], cleaver, gore2[12], bg2; Tex bg, disc, vignette, mask, grain[4]; Font font, small; } Assets;
static Assets A;
static Font padFont, padSmall; static Tex padDisc;

/* ------------------------------------------------------------ level */
static char map[LH][LW + 1];
enum { E_USHER, E_DOG, E_BLOB, E_MINIBLOB, E_GOAT, E_MEATBALL, E_SLICE, E_CRATE, E_HATCRATE };
typedef struct { int type, alive, hp, face, state, flash, onground; float x, y, vx, vy, t, timer, marked, stun, plat0, plat1, burn, btick, land; } Enemy;
#define MAXE 96
static Enemy en[MAXE]; static int nen;
typedef struct { int tx, used; } Breaker;
static Breaker breakers[8]; static int nbreak;
typedef struct { int tx; float t; } Drip;
static Drip drips[8]; static int ndrip;
static int checkpoints[12], ncheck, cur_check;

/* ---- the shows (levels) ---- */
typedef struct {
    const char *title, *subtitle, *boss_name, *boss_line, *boss_music;
    const char *zone[6]; int x0[7]; const char *music[6];
} LevelDef;
static const LevelDef LEVELS[] = {
    {"OPENING NIGHT", "UNCLE INKY'S JAMBOREE PALACE", "UNCLE INKY", "THE BANDLEADER WHO NEVER LEFT", "mm_boss1",
     {"LOBBY", "AUDITORIUM", "BOILER ROOM", "BACKSTAGE", "PROJECTION BOOTH", "MAIN STAGE"}, {0, 40, 90, 130, 175, 210, LW},
     {"mm_level1", "mm_level1", "mm_boiler", "mm_backstage", "mm_booth", "mm_level1"}},
    {"PIZZA PARTY", "THE JAMBOREE PIZZA PARLOR", "BIG CHEESE", "THE CHEF WHO ATE THE BIRTHDAY", "mm_boss2",
     {"PARTY ROOM", "DINING HALL", "PIZZA COUNTER", "KITCHEN", "DELIVERY DOCK", "THE RAT'S DEN"}, {0, 40, 85, 125, 170, 210, LW},
     {"mm_level2", "mm_level2", "mm_level2", "mm_kitchen", "mm_kitchen", "mm_level2"}},
};
#define NUM_LEVELS 2
static int level_no;
static const char *const *ZONE_NAME = LEVELS[0].zone;
static const int *ZONE_X0 = LEVELS[0].x0;
static int zone_of(float x) { int tx = (int)(x / TILE); for (int z = 0; z < 6; z++) if (tx < ZONE_X0[z + 1]) return z; return 5; }

static void put(int x, int y, char c) { if (x >= 0 && x < LW && y >= 0 && y < LH) map[y][x] = c; }
static void block(int x0, int x1, int y0, int y1) { for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) put(x, y, '#'); }
static void plat(int x0, int x1, int y) { for (int x = x0; x <= x1; x++) put(x, y, '='); }
static void pit(int x0, int x1) { for (int x = x0; x <= x1; x++) { put(x, 13, '~'); put(x, 14, '~'); } }
static void spawn(int type, int tx, int row) {
    if (nen >= MAXE) return;
    Enemy *e = &en[nen++]; memset(e, 0, sizeof *e);
    e->type = type; e->alive = 1; e->x = tx * TILE + TILE / 2; e->y = row * TILE; e->face = -1;
    e->hp = type == E_USHER ? 3 : type == E_DOG ? 4 : type == E_BLOB ? 2 : type == E_MINIBLOB ? 1 :
            type == E_GOAT ? 4 : type == E_MEATBALL ? 3 : type == E_SLICE ? 2 : 1;
    if (type == E_SLICE) { e->plat0 = e->y - 30; e->timer = frand() * 3; }   /* plat0 = home altitude */
    e->timer = 1.0f + frand(); e->plat0 = 0; e->plat1 = LW * TILE;
}
static void breaker(int tx) { if (nbreak < 8) breakers[nbreak++] = (Breaker){tx, 0}; put(tx, 11, 'B'); }
static void checkpoint(int tx) { if (ncheck < 12) checkpoints[ncheck++] = tx; put(tx, 12, 'K'); }
static void closet(int tx) { put(tx, 11, 'C'); put(tx, 12, 'C'); }
static void drip(int tx) { if (ndrip < 8) drips[ndrip++] = (Drip){tx, frand() * 2}; }

static void build_level1(void);
static void build_level2(void);
static void build_level(void) {
    ZONE_NAME = LEVELS[level_no].zone; ZONE_X0 = LEVELS[level_no].x0;
    if (level_no == 1) build_level2(); else build_level1();
}
static void build_level1(void) {
    for (int y = 0; y < LH; y++) { memset(map[y], '.', LW); map[y][LW] = 0; }
    nen = nbreak = ndrip = ncheck = 0;
    block(0, LW - 1, 13, 14);
    block(0, 0, 0, 12); block(LW - 1, LW - 1, 0, 12);
    /* LOBBY */
    checkpoint(2);
    block(9, 10, 11, 12);
    plat(13, 18, 10); spawn(E_CRATE, 15, 10);
    spawn(E_USHER, 21, 13); spawn(E_USHER, 31, 13);
    breaker(26);
    block(34, 36, 12, 12);
    /* AUDITORIUM */
    checkpoint(41);
    block(44, 46, 12, 12); spawn(E_BLOB, 45, 12);
    block(47, 49, 11, 12);
    pit(50, 52);
    block(53, 55, 11, 12);
    plat(57, 63, 9); spawn(E_DOG, 60, 9);
    spawn(E_BLOB, 66, 13);
    pit(69, 70);
    plat(72, 76, 10); spawn(E_HATCRATE, 74, 10);
    spawn(E_USHER, 76, 13); spawn(E_BLOB, 81, 13);
    breaker(84);
    block(86, 88, 12, 12);
    /* BOILER ROOM */
    checkpoint(91);
    pit(95, 97); drip(99); spawn(E_BLOB, 101, 13);
    pit(103, 105); drip(108); spawn(E_BLOB, 108, 13);
    plat(110, 114, 9); pit(111, 113); spawn(E_BLOB, 116, 13); drip(117);
    spawn(E_CRATE, 118, 13); spawn(E_USHER, 120, 13);
    plat(121, 127, 9); spawn(E_DOG, 124, 9);
    breaker(128);
    /* BACKSTAGE: the Ink Fiend hunts here, hide in the prop closets */
    checkpoint(132);
    closet(138); block(142, 143, 12, 12); closet(147);
    spawn(E_USHER, 150, 13); pit(152, 153); closet(156);
    spawn(E_BLOB, 159, 13); block(161, 162, 11, 12); closet(165); closet(171);
    /* PROJECTION BOOTH: catwalks over an ink flood */
    checkpoint(176);
    pit(182, 191); plat(180, 184, 10); plat(186, 190, 8); spawn(E_DOG, 188, 8);
    spawn(E_USHER, 196, 13);
    plat(199, 204, 10); spawn(E_CRATE, 201, 10); spawn(E_BLOB, 200, 13);
    breaker(206);
    /* MAIN STAGE (boss arena) */
    checkpoint(209);
    for (int i = 0; i < nen; i++) {   /* dogs patrol their platform */
        Enemy *e = &en[i]; if (e->type != E_DOG) continue;
        int row = (int)(e->y / TILE), tx = (int)(e->x / TILE), a = tx, b = tx;
        while (a > 0 && map[row][a - 1] == '=') a--;
        while (b < LW - 1 && map[row][b + 1] == '=') b++;
        e->plat0 = a * TILE + 30; e->plat1 = (b + 1) * TILE - 30;
    }
}


/* SHOW 2: PIZZA PARTY. '>' '<' conveyor belts, 'O' brick ovens that blast fire, pits are hot cheese */
static void conveyor(int x0, int x1, int y, char dir) { for (int x = x0; x <= x1; x++) put(x, y, dir); }
static void oven(int x0, int y) { for (int x = x0; x < x0 + 2; x++) for (int yy = y; yy <= 12; yy++) put(x, yy, 'O'); }
static void build_level2(void) {
    for (int y = 0; y < LH; y++) { memset(map[y], '.', LW); map[y][LW] = 0; }
    nen = nbreak = ndrip = ncheck = 0;
    block(0, LW - 1, 13, 14);
    block(0, 0, 0, 12); block(LW - 1, LW - 1, 0, 12);
    /* PARTY ROOM: long party tables, a goat band and swooping pizza bats */
    checkpoint(2);
    plat(8, 14, 10); spawn(E_CRATE, 11, 10);
    spawn(E_GOAT, 19, 13); spawn(E_SLICE, 24, 7);
    plat(26, 32, 10); spawn(E_SLICE, 33, 6); spawn(E_GOAT, 36, 13);
    breaker(30);
    /* DINING HALL: hot cheese pits and rolling meatballs */
    checkpoint(41);
    spawn(E_MEATBALL, 50, 13); pit(54, 56); plat(53, 57, 10);
    spawn(E_SLICE, 60, 7); block(62, 64, 12, 12); spawn(E_MEATBALL, 68, 13);
    pit(71, 73); spawn(E_GOAT, 77, 13); plat(78, 82, 10); spawn(E_HATCRATE, 80, 10);
    /* PIZZA COUNTER: climb the counter, goats on top */
    checkpoint(86);
    block(92, 103, 11, 12); spawn(E_GOAT, 98, 11); spawn(E_SLICE, 96, 6);
    plat(105, 109, 9); spawn(E_MEATBALL, 112, 13); spawn(E_CRATE, 107, 9);
    block(114, 116, 12, 12); spawn(E_SLICE, 119, 7); breaker(121);
    /* KITCHEN: fire ovens and conveyor belts */
    checkpoint(126);
    oven(131, 11); conveyor(134, 140, 12, '>'); oven(142, 11); spawn(E_MEATBALL, 138, 12);
    spawn(E_SLICE, 145, 7); conveyor(147, 153, 12, '<'); oven(155, 11); spawn(E_GOAT, 151, 12);
    pit(158, 160); plat(157, 161, 10); oven(163, 11); spawn(E_CRATE, 166, 13); breaker(167);
    /* DELIVERY DOCK: conveyors over a cheese vat */
    checkpoint(171);
    pit(176, 196); conveyor(175, 181, 11, '>'); conveyor(184, 189, 10, '<'); conveyor(192, 197, 11, '>');
    spawn(E_SLICE, 183, 6); spawn(E_SLICE, 191, 5); spawn(E_GOAT, 201, 13); spawn(E_MEATBALL, 205, 13);
    breaker(203);
    /* THE RAT'S DEN (boss) */
    checkpoint(209);
    for (int i = 0; i < nen; i++) en[i].plat1 = LW * TILE;
}
static int solid_at(int tx, int ty) { if (tx < 0 || tx >= LW) return 1; if (ty < 0 || ty >= LH) return 0; char c = map[ty][tx]; return c == '#' || c == 'O' || c == '>' || c == '<'; }
static int oneway_at(int tx, int ty) { if (tx < 0 || tx >= LW || ty < 0 || ty >= LH) return 0; return map[ty][tx] == '='; }

static int move_box(float *x, float *y, float *vx, float *vy, float w, float h, int drop);
/* standing on a conveyor belt: it carries you along */
static void conveyor_carry(float *x, float *y, float w, float h) {
    int tx = (int)(*x / TILE), ty = (int)((*y + 2) / TILE);
    if (tx < 0 || tx >= LW || ty < 0 || ty >= LH) return;
    char c = map[ty][tx]; if (c != '>' && c != '<') return;
    float cv = c == '>' ? 150 : -150, vy0 = 0; move_box(x, y, &cv, &vy0, w, h, 0);
}
/* x = centre, y = feet. returns 1 = landed, 2 = hit a wall */
static int move_box(float *x, float *y, float *vx, float *vy, float w, float h, int drop) {
    int res = 0;
    float nx = *x + *vx * DT;
    float top = *y - h, bot = *y - 1;
    int ty0 = (int)floorf(top / TILE), ty1 = (int)floorf(bot / TILE);
    if (*vx > 0) { int tx = (int)floorf((nx + w / 2) / TILE); for (int ty = ty0; ty <= ty1; ty++) if (solid_at(tx, ty)) { nx = tx * TILE - w / 2 - 0.01f; *vx = 0; res |= 2; break; } }
    else if (*vx < 0) { int tx = (int)floorf((nx - w / 2) / TILE); for (int ty = ty0; ty <= ty1; ty++) if (solid_at(tx, ty)) { nx = (tx + 1) * TILE + w / 2 + 0.01f; *vx = 0; res |= 2; break; } }
    *x = nx;
    float ny = *y + *vy * DT;
    int tx0 = (int)floorf((*x - w / 2 + 1) / TILE), tx1 = (int)floorf((*x + w / 2 - 1) / TILE);
    if (*vy >= 0) {
        int ty = (int)floorf(ny / TILE), pty = (int)floorf((*y - 0.01f) / TILE);
        for (int tx = tx0; tx <= tx1; tx++) if (solid_at(tx, ty) || (!drop && oneway_at(tx, ty) && pty < ty)) { ny = ty * TILE; *vy = 0; res |= 1; break; }
    } else {
        int ty = (int)floorf((ny - h) / TILE);
        for (int tx = tx0; tx <= tx1; tx++) if (solid_at(tx, ty)) { ny = (ty + 1) * TILE + h; *vy = 0; break; }
    }
    *y = ny;
    return res;
}
static int overlap(float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh) {
    return fabsf(ax - bx) * 2 < aw + bw && ay - ah < by && by - bh < ay;   /* boxes: centre x, feet y */
}

/* ------------------------------------------------------------ players */
enum { W_NAIL, W_RIVET, W_TORCH, W_PLUNGER, NWEAP };
static const char *WEAP_NAME[NWEAP] = {"NAIL GUN", "RIVET SPREAD", "WELDING TORCH", "PLUNGER LAUNCHER"};
typedef struct {
    int active, pad, alive, gear, lives, face, onground, hidden, crouch, weapon, aim_dx, aim_dy, shooting, closet_tx;
    float x, y, vx, vy, inv, cool, anim, dead_t, shoot_t, bob;
} Player;
static Player pl[2];
static int mode;            /* 0 solo (GamePad does everything), 1 security office co-op */
static int op_pad;          /* who runs the cameras */
static int score;

/* projectiles */
enum { P_NAIL, P_RIVET, P_FLAME, P_PLUNGER, P_LIGHT, P_NOTE, P_TICKET, P_DRIP, P_CONFETTI, P_PIZZA, P_CHEESE };
typedef struct { int type, alive, enemy, dmg, bounces, last_hit; float x, y, vx, vy, life, grav, r; } Proj;
#define MAXP 160
static Proj pr[MAXP];
enum { PK_DOT, PK_BLOOD, PK_SMOKE, PK_EMBER, PK_CASING, PK_DEBRIS };
typedef struct { float x, y, vx, vy, life, max, size, grav, ang, vang; Uint8 r, g, b, add, kind; } Part;
#define MAXPART 500
static Part parts[MAXPART];

/* pickups (from crates) */
typedef struct { int alive, kind; float x, y, vy, t; } Pickup;   /* kind 0..3 weapon (cycles), 10 = hard hat */
static Pickup pick[8];

/* world state */
static float power = 100, lit[6], camx, shake, msg_t, warn_t, cam_flash;
static char msg[64];
static int scene, menu_sel, cam_sel = 0, quit_req, frame_no, darkness_on;
enum { SC_TITLE, SC_PLAY, SC_OVER, SC_CLEAR };
static float scene_t, ping_cool, op_idle;

/* the Ink Fiend */
static struct { int state; float x, y, t, cycle; int seen; } fiend;   /* 0 asleep, 1 warning, 2 hunting, 3 retreating */
/* boss */
static struct {
    int active, state, hp, maxhp, pose, flash, slams, intro, dead, summoned, face;
    float x, y, t, timer, targetx, sweepx, bob, vy;
} boss;
static int arena_locked;

static void message(const char *s, float t) { snprintf(msg, sizeof msg, "%s", s); msg_t = t; }

static void part(float x, float y, float vx, float vy, float life, float size, float grav, Uint8 r, Uint8 g, Uint8 b, int add) {
    for (int i = 0; i < MAXPART; i++) if (parts[i].life <= 0) {
        parts[i] = (Part){x, y, vx, vy, life, life, size, grav, 0, 0, r, g, b, (Uint8)add, PK_DOT}; return;
    }
}
static void ink_burst(float x, float y, int n, float spd) {
    for (int i = 0; i < n; i++) { float a = frand() * 6.283f, s = spd * (0.3f + frand());
        part(x, y, cosf(a) * s, sinf(a) * s - 120, 0.5f + frand() * 0.5f, 6 + frand() * 10, 900, 12, 10, 10, 0); }
}
static void spark_burst(float x, float y, int n, Uint8 r, Uint8 g, Uint8 b) {
    for (int i = 0; i < n; i++) { float a = frand() * 6.283f, s = 150 + frand() * 300;
        part(x, y, cosf(a) * s, sinf(a) * s, 0.25f + frand() * 0.3f, 3 + frand() * 4, 300, r, g, b, 1); }
}

/* ---------------- effects: kinds of particle, gibs, splatter decals, shockwaves, hit-pause ---------------- */
static Part *part_k(int kind, float x, float y, float vx, float vy, float life, float size, float grav, Uint8 r, Uint8 g, Uint8 b, int add) {
    for (int i = 0; i < MAXPART; i++) if (parts[i].life <= 0) {
        parts[i] = (Part){x, y, vx, vy, life, life, size, grav, frand() * 360, (frand() - 0.5f) * 900, r, g, b, (Uint8)add, (Uint8)kind}; return &parts[i];
    }
    return NULL;
}
static void blood_spurt(float x, float y, float dirx, int n, float spd) {   /* animatronic "blood": red oil mixed with ink */
    for (int i = 0; i < n; i++) {
        float a = atan2f(-0.6f, dirx) + (frand() - 0.5f) * 1.6f, v = spd * (0.4f + frand());
        int ink = rand() % 3 == 0;
        part_k(PK_BLOOD, x, y, cosf(a) * v, sinf(a) * v, 1.4f, 3 + frand() * 6, 1500, ink ? 14 : 150 + rand() % 40, ink ? 10 : 12, ink ? 12 : 18, 0);
    }
}
static void smoke(float x, float y, int n, Uint8 shade) {
    for (int i = 0; i < n; i++) part_k(PK_SMOKE, x + (frand() - 0.5f) * 20, y, (frand() - 0.5f) * 60, -40 - frand() * 70, 0.9f + frand() * 0.6f, 10 + frand() * 10, -20, shade, shade, shade, 0);
}
static void embers(float x, float y, int n) {
    for (int i = 0; i < n; i++) part_k(PK_EMBER, x, y, (frand() - 0.5f) * 220, -100 - frand() * 220, 0.5f + frand() * 0.6f, 3 + frand() * 3, 200, 255, 140 + rand() % 80, 40, 1);
}

typedef struct { Tex *t; SDL_Rect src; float x, y, vx, vy, ang, vang, life, w, h; int flip, bleed, rest; } Gib;
#define MAXGIB 140
static Gib gibs[MAXGIB]; static int gib_next;
static void gib(Tex *t, SDL_Rect src, float x, float y, float vx, float vy, float w, float h, int flip, int bleed) {
    if (!t || !t->tex) return;
    Gib *g = &gibs[gib_next]; gib_next = (gib_next + 1) % MAXGIB;
    *g = (Gib){t, src, x, y, vx, vy, frand() * 360, (frand() - 0.5f) * 1100, 6.0f + frand() * 2, w, h, flip, bleed, 0};
}
static void gore_part2(int idx, float x, float y, float spd, int bleed) {
    Tex *t = &A.gore2[idx]; if (!t->tex) return;
    float a = -1.57f + (frand() - 0.5f) * 2.4f, v = spd * (0.5f + frand());
    gib(t, (SDL_Rect){0, 0, t->w, t->h}, x, y, cosf(a) * v, sinf(a) * v, (float)t->w, (float)t->h, rand() & 1, bleed);
}
static void gore_part(int idx, float x, float y, float spd, int bleed) {
    Tex *t = &A.gore[idx]; if (!t->tex) return;
    float a = -1.57f + (frand() - 0.5f) * 2.4f, v = spd * (0.5f + frand());
    gib(t, (SDL_Rect){0, 0, t->w, t->h}, x, y, cosf(a) * v, sinf(a) * v, (float)t->w, (float)t->h, rand() & 1, bleed);
}
/* chop the sprite into a grid of chunks that fly apart */
static void shatter(Tex *t, float cx, float feet, int flip, int cols, int rows, float spd, int bleed) {
    if (!t || !t->tex) return;
    int cw = t->w / cols, ch = t->h / rows;
    for (int r = 0; r < rows; r++) for (int c = 0; c < cols; c++) {
        SDL_Rect src = {c * cw, r * ch, cw, ch};
        float lx = flip ? (t->w - (c + 0.5f) * cw) : (c + 0.5f) * cw;
        float x = cx - t->w / 2.0f + lx, y = feet - t->h + (r + 0.5f) * ch;
        float dx = x - cx, dy = y - (feet - t->h * 0.5f);
        gib(t, src, x, y, dx * 4 + (frand() - 0.5f) * spd, dy * 3 - spd * (0.6f + frand() * 0.6f), (float)cw, (float)ch, flip, bleed && (rand() % 2));
    }
}

typedef struct { float x, y, rx, ry, ang; int tex; Uint8 r, g, b, a; } Splat;
#define MAXSPLAT 260
static Splat splats[MAXSPLAT]; static int splat_next;
static void splat(float x, float y, float size, int floor, Uint8 r, Uint8 g, Uint8 b) {
    Splat *s = &splats[splat_next]; splat_next = (splat_next + 1) % MAXSPLAT;
    int tex = (size > 9 && A.gore[8].tex) ? 8 + (rand() & 1) : -1;
    *s = (Splat){x, y, size * (floor ? 2.2f : 1.0f), size * (floor ? 0.5f : 1.0f), floor ? 0 : frand() * 360, tex, r, g, b, (Uint8)(200 + rand() % 55)};
}

typedef struct { float x, y, r, max, life; Uint8 cr, cg, cb; } Ring;
static Ring rings[16];
static void ring(float x, float y, float max, Uint8 r, Uint8 g, Uint8 b) {
    for (int i = 0; i < 16; i++) if (rings[i].life <= 0) { rings[i] = (Ring){x, y, 10, max, 0.45f, r, g, b}; return; }
}
static int hitstop;
static Proj *proj(int type, float x, float y, float vx, float vy, int enemy, int dmg) {
    for (int i = 0; i < MAXP; i++) if (!pr[i].alive) {
        Proj *p = &pr[i]; memset(p, 0, sizeof *p);
        p->type = type; p->alive = 1; p->x = x; p->y = y; p->vx = vx; p->vy = vy; p->enemy = enemy; p->dmg = dmg;
        p->life = 2.5f; p->r = 8; p->last_hit = -1; return p;
    }
    return NULL;
}

static void reset_player(Player *p, int full) {
    int tx = checkpoints[cur_check];
    p->x = tx * TILE + TILE / 2 + (p == &pl[1] ? 40 : 0); p->y = FLOOR_Y - 2; p->vx = p->vy = 0;
    p->alive = 1; p->gear = 3; p->inv = 2.0f; p->hidden = 0; p->face = 1; p->dead_t = 0;
    if (full) { p->lives = 3; p->weapon = getenv("MM_WEAPON") ? atoi(getenv("MM_WEAPON")) % NWEAP : W_NAIL; }
}

static void start_level(void) {
    build_level();
    memset(pr, 0, sizeof pr); memset(parts, 0, sizeof parts); memset(pick, 0, sizeof pick);
    memset(gibs, 0, sizeof gibs); memset(splats, 0, sizeof splats); memset(rings, 0, sizeof rings); hitstop = 0;
    memset(&fiend, 0, sizeof fiend); memset(&boss, 0, sizeof boss); memset(lit, 0, sizeof lit);
    arena_locked = 0; power = 100; cur_check = 0; score = 0; cam_sel = 0;
    reset_player(&pl[0], 1); pl[0].active = 1;
    if (mode == 1) { pl[0].pad = 1; op_pad = 0; } else { pl[0].pad = 0; op_pad = 0; }
    pl[1].active = 0;
    camx = 0; scene = SC_PLAY; scene_t = 0;
    { char b[64]; snprintf(b, sizeof b, "SHOW %d: %s", level_no + 1, LEVELS[level_no].title); message(b, 2.5f); }
    music_play(LEVELS[level_no].music[0], 1);
}

static void hurt_player(Player *p, float from_x) {
    if (!p->alive || p->inv > 0 || p->hidden) return;
    p->gear--; p->inv = 1.6f; shake = 8;
    p->vx = (p->x < from_x ? -1 : 1) * 260; p->vy = -420;
    sfx("hurt");
    if (p->gear == 2) { for (int i = 0; i < 6; i++) part(p->x, p->y - 140, (frand() - 0.5f) * 300, -300 - frand() * 200, 0.9f, 9, 900, 230, 180, 50, 0); message("THERE GOES THE HARD HAT!", 1.4f); }
    if (p->gear == 1) { for (int i = 0; i < 10; i++) part(p->x, p->y - 70, (frand() - 0.5f) * 400, -300 - frand() * 200, 1.0f, 8, 900, 70, 100, 150, 0); message("DOWN TO YOUR BOXERS!", 1.4f); }
    if (p->gear <= 0) { p->alive = 0; p->dead_t = 0; p->vy = -700; sfx("boom"); ink_burst(p->x, p->y - 60, 20, 300); }
}
static void kill_player(Player *p) { if (!p->alive) return; p->gear = 1; p->inv = 0; p->hidden = 0; hurt_player(p, p->x); }

/* ------------------------------------------------------------ init */
static Tex load_or_empty(const char *rel) { Tex t = tex_load(rel); return t; }
static Tex make_vignette(void) {
    int w = 320, h = 180; SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA32);
    Uint32 *px = (Uint32 *)s->pixels;
    for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) {
        float dx = (x - w / 2.0f) / (w / 2.0f), dy = (y - h / 2.0f) / (h / 2.0f), d = sqrtf(dx * dx * 0.8f + dy * dy);
        float a = clampf((d - 0.55f) / 0.7f, 0, 1); a = a * a;
        px[y * (s->pitch / 4) + x] = SDL_MapRGBA(s->format, 20, 10, 4, (Uint8)(a * 235));
    }
    Tex t = tex_from_surface(s); SDL_FreeSurface(s); return t;
}
static Tex make_light_mask(void) {   /* black everywhere except a soft hole in the middle: lights-out flashlight */
    int w = 256; SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, w, w, 32, SDL_PIXELFORMAT_RGBA32);
    Uint32 *px = (Uint32 *)s->pixels;
    for (int y = 0; y < w; y++) for (int x = 0; x < w; x++) {
        float dx = (x - w / 2.0f) / (w / 2.0f), dy = (y - w / 2.0f) / (w / 2.0f), d = sqrtf(dx * dx + dy * dy);
        float a = clampf((d - 0.18f) / 0.5f, 0, 1);
        px[y * (s->pitch / 4) + x] = SDL_MapRGBA(s->format, 4, 2, 6, (Uint8)(a * 248));
    }
    Tex t = tex_from_surface(s); SDL_FreeSurface(s); return t;
}

void game_init(void) {
    srand(1234);
    font_load(&A.font, "font"); font_load(&A.small, "font_small"); font_set_small(&A.font, &A.small);
    const char *sets[3] = {"hero", "hero_b", "hero_c"};
    char rel[64];
    for (int s = 0; s < 3; s++) for (int i = 0; i < HERO_FRAMES; i++) {
        snprintf(rel, sizeof rel, "chars/%s_%d.png", sets[s], i);
        A.hero[s][i] = load_or_empty(rel);
        if (!A.hero[s][i].tex && s > 0) A.hero[s][i] = A.hero[0][i];
        if (!A.hero[s][i].tex && i > 0) A.hero[s][i] = A.hero[s][0];   /* gear art not made yet: reuse full gear */
    }
    for (int i = 0; i < 5; i++) {
        snprintf(rel, sizeof rel, "chars/usher_%d.png", i); A.usher[i] = load_or_empty(rel); if (!A.usher[i].tex && i) A.usher[i] = A.usher[i % 2];
        snprintf(rel, sizeof rel, "chars/projdog_%d.png", i); A.dog[i] = load_or_empty(rel); if (!A.dog[i].tex && i) A.dog[i] = A.dog[i % 2];
        if (i < 4) { snprintf(rel, sizeof rel, "chars/blob_%d.png", i); A.blob[i] = load_or_empty(rel); if (!A.blob[i].tex && i) A.blob[i] = A.blob[i % 2]; }
    }
    for (int i = 0; i < 12; i++) { snprintf(rel, sizeof rel, "chars/gore_%d.png", i); A.gore[i] = load_or_empty(rel); }
    for (int i = 0; i < 12; i++) { snprintf(rel, sizeof rel, "chars/gore2_%d.png", i); A.gore2[i] = load_or_empty(rel); if (!A.gore2[i].tex) A.gore2[i] = A.gore[i]; }
    for (int i = 0; i < 5; i++) {
        snprintf(rel, sizeof rel, "chars/goat_%d.png", i); A.goat[i] = load_or_empty(rel); if (!A.goat[i].tex && i) A.goat[i] = A.goat[0];
        snprintf(rel, sizeof rel, "chars/meat_%d.png", i); A.meat[i] = load_or_empty(rel); if (!A.meat[i].tex && i) A.meat[i] = A.meat[0];
        snprintf(rel, sizeof rel, "chars/slice_%d.png", i); A.slice[i] = load_or_empty(rel); if (!A.slice[i].tex && i) A.slice[i] = A.slice[0];
    }
    for (int i = 0; i < 5; i++) { snprintf(rel, sizeof rel, "chars/rat_%d.png", i); A.rat[i] = load_or_empty(rel); }
    A.cleaver = load_or_empty("chars/cleaver.png");
    A.bg2 = load_or_empty("bg/pizza.jpg");
    for (int i = 0; i < 3; i++) { snprintf(rel, sizeof rel, "chars/inky_%d.png", i); A.inky[i] = load_or_empty(rel); }
    A.bg = load_or_empty("bg/theater.jpg");
    for (int i = 0; i < 4; i++) { snprintf(rel, sizeof rel, "ui/grain%d.png", i); A.grain[i] = load_or_empty(rel); }
    A.disc = make_circle_tex(64); A.vignette = make_vignette(); A.mask = make_light_mask();
    build_level();
    scene = SC_TITLE; music_play("mm_title", 1);
}
void game_init_pad(void) {
    font_load(&padFont, "font"); font_load(&padSmall, "font_small"); padDisc = make_circle_tex(32);
}
int game_wants_quit(void) { return quit_req; }

void game_goto(const char *name) {
    if (!strcmp(name, "title")) { scene = SC_TITLE; return; }
    if (!strncmp(name, "level2", 6) || !strcmp(name, "boss2") || !strcmp(name, "kitchen") || !strcmp(name, "dock")) {
        level_no = 1; start_level();
        if (!strcmp(name, "boss2")) { cur_check = ncheck - 1; reset_player(&pl[0], 0); }
        if (!strcmp(name, "kitchen")) { cur_check = 3; reset_player(&pl[0], 0); }
        if (!strcmp(name, "dock")) { cur_check = 4; reset_player(&pl[0], 0); }
        return;
    }
    start_level();
    if (!strcmp(name, "boss")) { cur_check = ncheck - 1; reset_player(&pl[0], 0); }
    if (!strcmp(name, "fiend")) { cur_check = 3; reset_player(&pl[0], 0); }
    if (!strcmp(name, "booth")) { cur_check = 4; reset_player(&pl[0], 0); }
    if (!strcmp(name, "dark")) { power = 0; }
    if (!strcmp(name, "coop")) { mode = 1; start_level(); }
}

/* ------------------------------------------------------------ player update */
static void fire(Player *p) {
    float dx = p->aim_dx, dy = p->aim_dy;
    float len = sqrtf(dx * dx + dy * dy); if (len < 0.1f) { dx = p->face; dy = 0; len = 1; }
    dx /= len; dy /= len;
    float mx = p->x + p->face * 40, my = p->y - 78;
    if (p->aim_dy < 0 && p->aim_dx != 0) { mx = p->x + p->face * 42; my = p->y - 122; }
    else if (p->aim_dy < 0) { mx = p->x + p->face * 18; my = p->y - 152; }
    else if (p->aim_dy > 0) { mx = p->x + p->aim_dx * 20; my = p->y - 30; }
    else if (p->crouch) { my = p->y - 48; }
    p->shoot_t = 0.15f;
    for (int k = 0; k < (p->weapon == W_TORCH ? 1 : 3); k++) part(mx + dx * 10, my + dy * 10, dx * (200 + frand() * 300) + (frand() - 0.5f) * 120, dy * (200 + frand() * 300) + (frand() - 0.5f) * 120, 0.12f, 4, 0, 255, 230, 150, 1);
    if (p->weapon == W_NAIL || p->weapon == W_RIVET) {   /* spent casing flips out the side */
        Part *c = part_k(PK_CASING, p->x + p->face * 10, my + 6, -p->face * (120 + frand() * 120), -260 - frand() * 160, 2.5f, 3, 1600, 220, 180, 80, 0);
        if (c) c->vang = 1400;
    }
    if (p->weapon == W_RIVET) smoke(mx, my, 1, 120);
    switch (p->weapon) {
    case W_NAIL: proj(P_NAIL, mx, my, dx * 1150, dy * 1150, 0, 1); p->cool = 0.11f; sfx("throw"); break;
    case W_RIVET:
        for (int k = -1; k <= 1; k++) { float a = atan2f(dy, dx) + k * 0.2f; Proj *q = proj(P_RIVET, mx, my, cosf(a) * 950, sinf(a) * 950, 0, 1); if (q) q->r = 9; }
        p->cool = 0.24f; sfx("rivet"); break;
    case W_TORCH: {
        float a = atan2f(dy, dx) + (frand() - 0.5f) * 0.25f;
        Proj *q = proj(P_FLAME, mx, my, cosf(a) * 720, sinf(a) * 720, 0, 1); if (q) { q->life = 0.24f; q->r = 16; }
        p->cool = 0.05f; if (frame_no % 8 == 0) sfx("sizzle"); break; }
    case W_PLUNGER: {
        Proj *q = proj(P_PLUNGER, mx, my, dx * 520 + p->vx * 0.3f, dy * 520 - 380, 0, 2); if (q) { q->grav = 1500; q->r = 14; }
        p->cool = 0.45f; sfx("boing"); break; }
    }
}

static void update_player(Player *p, const Input *in) {
    if (!p->active) return;
    if (!p->alive) {
        p->dead_t += DT; p->vy += 1800 * DT; p->y += p->vy * DT;
        if (p->dead_t > 1.6f) {
            p->lives--;
            if (p->lives > 0) { reset_player(p, 0); p->weapon = W_NAIL; }
            else { p->active = 0; p->alive = 0; }
        }
        return;
    }
    if (p->inv > 0) p->inv -= DT;
    if (p->cool > 0) p->cool -= DT;
    if (p->shoot_t > 0) p->shoot_t -= DT;
    unsigned h = in->held, pr_ = in->pressed;
    int L = (h & B_LEFT) != 0, R = (h & B_RIGHT) != 0, U = (h & B_UP) != 0, D = (h & B_DOWN) != 0;

    /* hiding in a prop closet */
    int ctx = (int)(p->x / TILE);
    if (p->hidden) {
        if ((pr_ & (B_UP | B_JUMP | B_DOWN))) { p->hidden = 0; p->inv = 0.4f; sfx("closet"); }
        return;
    }
    if ((pr_ & B_UP) && p->onground && map[12][ctx] == 'C') {
        p->hidden = 1; p->closet_tx = ctx; p->x = ctx * TILE + TILE / 2; p->vx = 0; sfx("closet");
        message("HIDING... (UP TO COME OUT)", 1.2f); return;
    }
    /* flip a breaker */
    if ((pr_ & B_UP) && p->onground) for (int i = 0; i < nbreak; i++) if (!breakers[i].used && abs(breakers[i].tx - ctx) <= 1) {
        breakers[i].used = 1; power = clampf(power + 40, 0, 100); sfx("power"); message("BREAKER ON: +40% POWER", 1.6f);
        spark_burst(breakers[i].tx * TILE + 24, 11 * TILE + 20, 20, 255, 230, 120);
    }

    int lock = (h & B_LOCK) != 0;
    p->crouch = D && p->onground && !L && !R && !lock;
    float target = 0;
    if (!lock && !p->crouch) { if (L) target = -340; if (R) target = 340; }
    if (L) p->face = -1; if (R) p->face = 1;
    float accel = p->onground ? 3200 : 2000;
    if (p->vx < target) p->vx = fminf(target, p->vx + accel * DT); else if (p->vx > target) p->vx = fmaxf(target, p->vx - accel * DT);

    /* aim: Contra style */
    p->aim_dx = p->face; p->aim_dy = 0;
    if (U) { p->aim_dy = -1; p->aim_dx = (L || R) ? p->face : 0; }
    else if (D && !p->onground) { p->aim_dy = 1; p->aim_dx = (L || R) ? p->face : 0; }
    else if (D && lock) { p->aim_dy = 1; p->aim_dx = (L || R) ? p->face : 0; }

    int drop = D && (pr_ & B_JUMP) && p->onground && oneway_at(ctx, (int)(p->y / TILE));
    if ((pr_ & B_JUMP) && p->onground && !drop) { p->vy = -920; p->onground = 0; sfx("jump"); }
    if (!(h & B_JUMP) && p->vy < -300) p->vy += 3600 * DT;    /* short hop when the button is let go */
    p->vy += 2500 * DT; if (p->vy > 1300) p->vy = 1300;
    float w = 46, hh = p->crouch ? 80 : 124;
    int r = move_box(&p->x, &p->y, &p->vx, &p->vy, w, hh, drop);
    p->onground = (r & 1) != 0;
    if (p->onground) conveyor_carry(&p->x, &p->y, w, hh);
    if (arena_locked) p->x = clampf(p->x, ARENA_X + 30, ARENA_X + SCREEN_W - 30);
    p->x = clampf(p->x, 30, LW * TILE - 30);
    if (p->vx != 0 && p->onground) p->anim += DT * fabsf(p->vx) / 340; else p->anim = 0;
    p->bob += DT;

    if ((h & B_SHOOT) && p->cool <= 0) fire(p);

    /* fell into the ink */
    if (p->y > FLOOR_Y + 30 || p->y > LH * TILE + 100) { sfx("splash"); ink_burst(p->x, FLOOR_Y, 25, 300); kill_player(p); }
    /* checkpoints */
    for (int i = cur_check + 1; i < ncheck; i++) if (p->x > checkpoints[i] * TILE) {
        cur_check = i; sfx("checkpoint"); message("GHOST LIGHT LIT: CHECKPOINT", 1.5f);
        spark_burst(checkpoints[i] * TILE + 24, 12 * TILE - 40, 30, 255, 220, 140);
    }
    /* pickups */
    for (int i = 0; i < 8; i++) if (pick[i].alive && overlap(p->x, p->y, w, hh, pick[i].x, pick[i].y, 50, 50)) {
        pick[i].alive = 0; sfx("pickup");
        if (pick[i].kind == 10) { if (p->gear < 3) p->gear++; message(p->gear == 3 ? "FULL GEAR!" : "OVERALLS BACK ON!", 1.4f); score += 500; }
        else { int k = ((int)(pick[i].t / 0.6f)) % NWEAP; p->weapon = k; message(WEAP_NAME[k], 1.4f); score += 200; }
    }
}

/* ------------------------------------------------------------ enemies */
static Player *nearest_player(float x, float y, float *dist) {
    Player *best = NULL; float bd = 1e9f;
    for (int i = 0; i < 2; i++) { Player *p = &pl[i]; if (!p->active || !p->alive || p->hidden) continue;
        float d = fabsf(p->x - x) + fabsf(p->y - y) * 0.5f; if (d < bd) { bd = d; best = p; } }
    if (dist) *dist = bd; return best;
}
static void enemy_box(Enemy *e, float *w, float *h) {
    switch (e->type) {
    case E_USHER: *w = 60; *h = 150; break;
    case E_DOG: *w = 92; *h = 112; break;
    case E_BLOB: *w = 64; *h = 60; break;
    case E_MINIBLOB: *w = 40; *h = 38; break;
    case E_GOAT: *w = 64; *h = 150; break;
    case E_MEATBALL: *w = 72; *h = 70; break;
    case E_SLICE: *w = 76; *h = 54; break;
    default: *w = 56; *h = 48; break;
    }
}
static Tex *enemy_tex(Enemy *e) {
    int walk = ((int)(e->t * 7)) % 4;
    switch (e->type) {
    case E_USHER: return &A.usher[e->state == 1 ? 4 : (fabsf(e->vx) > 5 ? walk : 0)];
    case E_DOG: return &A.dog[e->state >= 1 ? 4 : walk];
    case E_BLOB: case E_MINIBLOB: return &A.blob[!e->onground ? 2 : e->land > 0 ? 3 : e->timer < 0.25f ? 1 : 0];
    case E_GOAT: return &A.goat[e->state == 1 ? 4 : (fabsf(e->vx) > 5 ? walk : 0)];
    case E_MEATBALL: return &A.meat[e->state == 1 ? 4 : ((int)(e->t * 10)) % 4];
    case E_SLICE: return &A.slice[e->state == 1 ? 4 : ((int)(e->t * 10)) % 4];
    }
    return NULL;
}
static void gore_enemy(Enemy *e) {
    float cx = e->x, cy = e->y - 60, k = e->type == E_MINIBLOB ? 0.6f : 1.0f;
    Tex *t = enemy_tex(e);
    switch (e->type) {
    case E_USHER:   /* porcelain doll: head cracks open, arms fly, oil-blood everywhere */
        sfx("shatter");
        shatter(t, e->x, e->y, e->face > 0, 3, 4, 420, 1);
        gore_part(2, cx, cy - 60, 520, 0); gore_part(2, cx, cy - 60, 480, 0); gore_part(3, cx, cy - 70, 600, 1);
        gore_part(3, cx, cy - 70, 560, 1); gore_part(4, cx, cy - 20, 500, 1); gore_part(4, cx, cy - 20, 460, 1);
        gore_part(10, cx, cy, 500, 0); gore_part(11, cx, cy, 380, 1); gore_part(0, cx, cy, 450, 0);
        blood_spurt(cx, cy - 40, -e->face, 40, 620); blood_spurt(cx, cy - 40, e->face, 20, 500);
        break;
    case E_DOG:     /* projector dog: blows apart into brass, lens glass and springs */
        shatter(t, e->x, e->y, e->face > 0, 4, 3, 480, 1);
        gore_part(6, cx, cy, 600, 0); gore_part(0, cx, cy, 560, 0); gore_part(0, cx, cy, 520, 0); gore_part(1, cx, cy, 540, 0);
        gore_part(7, cx, cy, 500, 0); gore_part(5, cx, cy, 460, 1); gore_part(3, cx, cy - 20, 580, 1);
        blood_spurt(cx, cy, -e->face, 26, 560); spark_burst(cx, cy, 24, 255, 210, 110); smoke(cx, cy, 8, 70);
        ring(cx, cy, 110, 255, 220, 150); sfx("boom"); sfx("clank");
        break;
    case E_GOAT:    /* party goat: horns, accordion keys and party hat everywhere */
        sfx("clank"); shatter(t, e->x, e->y, e->face > 0, 3, 4, 420, 1);
        gore_part2(0, cx, cy - 70, 560, 0); gore_part2(0, cx, cy - 70, 520, 0); gore_part2(1, cx, cy, 500, 0); gore_part2(1, cx, cy, 460, 0);
        gore_part2(2, cx, cy - 80, 600, 0); gore_part2(3, cx, cy - 60, 580, 1); gore_part2(10, cx, cy, 480, 0);
        blood_spurt(cx, cy - 40, -e->face, 36, 600);
        for (int i = 0; i < 24; i++) part(cx, cy - 40, (frand() - 0.5f) * 700, -200 - frand() * 500, 1.2f, 4, 900, 120 + rand() % 136, 80 + rand() % 176, 80 + rand() % 176, 0);   /* confetti */
        break;
    case E_MEATBALL: /* meatball: chunks and sauce */
        sfx("splat"); shatter(t, e->x, e->y, e->face > 0, 3, 3, 380, 1);
        for (int k = 0; k < 4; k++) gore_part2(4, cx, cy, 520, 1);
        gore_part2(3, cx, cy, 560, 1);
        for (int i = 0; i < 30; i++) { float a = frand() * 6.283f, v = 200 + frand() * 450;
            part_k(PK_BLOOD, cx, cy, cosf(a) * v, sinf(a) * v - 250, 1.4f, 4 + frand() * 7, 1500, 170 + rand() % 40, 20, 14, 0); }
        break;
    case E_SLICE:   /* pizza bat: pepperoni, wings, cheese */
        sfx("splat"); shatter(t, e->x, e->y, e->face > 0, 3, 2, 360, 0);
        gore_part2(6, cx, cy, 500, 0); gore_part2(6, cx, cy, 460, 0); gore_part2(7, cx, cy, 520, 0); gore_part2(7, cx, cy, 480, 0); gore_part2(5, cx, cy, 440, 0);
        blood_spurt(cx, cy, -e->face, 14, 450);
        break;
    default:        /* ink blob: bursts into a big ink splash that paints the room */
        sfx("splat");
        shatter(t, e->x, e->y, e->face > 0, 2, 2, 300 * k, 0);
        for (int i = 0; i < (int)(30 * k); i++) { float a = frand() * 6.283f, v = 200 + frand() * 450;
            part_k(PK_BLOOD, cx, e->y - 30 * k, cosf(a) * v, sinf(a) * v - 250, 1.4f, 4 + frand() * 8 * k, 1500, 12, 8, 12, 0); }
        break;
    }
    hitstop = e->type == E_MINIBLOB ? 1 : 3;
}
static void enemy_damage(Enemy *e, int dmg) {
    if (e->marked > 0) dmg *= 2;
    e->hp -= dmg; e->flash = 6; sfx("hit");
    if (e->type != E_CRATE && e->type != E_HATCRATE) {   /* every hit draws a little oil-blood */
        float w, h; enemy_box(e, &w, &h);
        blood_spurt(e->x, e->y - h * 0.6f, e->face > 0 ? -1 : 1, 6, 380);
        e->vx += (e->face > 0 ? -1 : 1) * 60;
    }
    if (e->hp > 0) return;
    e->alive = 0; shake = 6;
    float cx = e->x, cy = e->y - 40;
    if (e->type == E_CRATE || e->type == E_HATCRATE) {
        for (int i = 0; i < 8; i++) if (!pick[i].alive) { pick[i] = (Pickup){1, e->type == E_HATCRATE ? 10 : 0, cx, e->y - 30, -400, frand() * 2}; break; }
        spark_burst(cx, cy, 16, 200, 150, 90); sfx("poof");
        for (int i = 0; i < 8; i++) part_k(PK_DEBRIS, cx, cy, (frand() - 0.5f) * 500, -200 - frand() * 400, 2.5f, 10 + frand() * 8, 1600, 150, 100, 55, 0);
        return;
    }
    score += e->type == E_DOG ? 400 : e->type == E_USHER ? 300 : 100;
    ink_burst(cx, cy, 10, 260); sfx("poof");
    gore_enemy(e);
    if (e->type == E_BLOB) for (int k = -1; k <= 1; k += 2) {   /* blobs split in two */
        for (int i = 0; i < MAXE; i++) if (!en[i].alive && (en[i].type == E_MINIBLOB || i >= nen)) {
            if (i >= nen) nen = i + 1;
            Enemy *m = &en[i]; memset(m, 0, sizeof *m); m->type = E_MINIBLOB; m->alive = 1; m->hp = 1;
            m->x = e->x + k * 20; m->y = e->y - 4; m->vx = k * 180; m->vy = -400; m->timer = 0.6f; m->plat0 = 0; m->plat1 = LW * TILE; break;
        }
    }
}

static void update_enemies(void) {
    float speed_k = darkness_on ? 1.35f : 1.0f;
    for (int i = 0; i < nen; i++) {
        Enemy *e = &en[i]; if (!e->alive) continue;
        if (e->x < camx - 500 || e->x > camx + SCREEN_W + 400) continue;   /* only near the screen */
        if (e->flash > 0) e->flash--;
        if (e->marked > 0) e->marked -= DT;
        if (lit[zone_of(e->x)] > 7.0f) e->stun = 1.0f;     /* the lights just slammed on: dazzled */
        if (e->stun > 0) { e->stun -= DT; continue; }
        float w, h; enemy_box(e, &w, &h);
        float dist; Player *t = nearest_player(e->x, e->y, &dist);
        e->t += DT * speed_k;
        if (e->type == E_SLICE) {   /* flies: bob around home, then dive-bomb */
            if (e->state == 0) {
                e->face = t && t->x < e->x ? -1 : 1;
                float hx = t ? t->x - e->face * 220 : e->x;
                e->vx += ((hx - e->x) * 1.2f - e->vx) * 2 * DT; e->vx = clampf(e->vx, -200, 200);
                e->y = e->plat0 + sinf(e->t * 3) * 26;
                e->timer -= DT * speed_k;
                if (t && dist < 520 && e->timer <= 0) { e->state = 1; float dx = t->x - e->x, dy = (t->y - 70) - e->y, l = sqrtf(dx * dx + dy * dy) + 1;
                    e->vx = dx / l * 480 * speed_k; e->vy = dy / l * 480 * speed_k; sfx("spit"); }
            } else {
                e->vy -= 300 * DT;   /* pull out of the dive */
                if (e->y < e->plat0 && e->vy < 0) { e->state = 0; e->vy = 0; e->timer = 1.6f + frand(); }
            }
            e->x += e->vx * DT; if (e->state == 1) e->y += e->vy * DT;
            if (e->y > FLOOR_Y - 20) { e->y = FLOOR_Y - 20; e->vy = -300; }
            for (int k = 0; k < 2; k++) { Player *p = &pl[k]; if (p->active && p->alive && !p->hidden && overlap(p->x, p->y, 40, 110, e->x, e->y, 60, 46)) hurt_player(p, e->x); }
            continue;
        }
        switch (e->type) {
        case E_USHER:
            if (t && dist < 760) e->face = t->x < e->x ? -1 : 1;
            if (e->state == 0) {
                e->vx = (t && dist < 760 && fabsf(t->x - e->x) > 120) ? e->face * 75 * speed_k : 0;
                e->timer -= DT * speed_k;
                if (e->timer <= 0 && t && dist < 700) { e->state = 1; e->timer = 0.5f; e->vx = 0; }
            } else {
                e->timer -= DT * speed_k;
                if (e->timer <= 0) {
                    Proj *q = proj(P_LIGHT, e->x + e->face * 40, e->y - 98, e->face * 430, 0, 1, 1); if (q) q->r = 12;
                    sfx("spit"); e->state = 0; e->timer = 2.4f + frand();
                }
            }
            break;
        case E_DOG:
            if (e->state == 0) {                     /* patrol */
                if (e->vx == 0) e->vx = e->face * 90;
                e->vx = signf(e->vx) * 90 * speed_k; e->face = e->vx < 0 ? -1 : 1;
                if (e->x < e->plat0) { e->x = e->plat0; e->vx = 90; } if (e->x > e->plat1) { e->x = e->plat1; e->vx = -90; }
                e->timer -= DT;
                if (t && fabsf(t->x - e->x) < 620 && fabsf(t->y - e->y) < 170 && e->timer <= 0) {
                    e->state = 1; e->timer = 0.9f / speed_k; e->face = t->x < e->x ? -1 : 1; e->vx = 0; sfx("charge"); }
            } else if (e->state == 1) {              /* lens warming up */
                e->vx = 0; e->timer -= DT; if (e->timer <= 0) { e->state = 2; e->timer = 0.45f; sfx("beam"); }
            } else if (e->state == 2) {              /* projector beam */
                e->timer -= DT;
                float bx0 = e->x + e->face * 50, by = e->y - 76;
                {   /* where the beam ends: a wall, or 700 px out */
                    float ex = bx0; for (int k = 0; k < 70; k++) { ex += e->face * 10; if (solid_at((int)(ex / TILE), (int)(by / TILE))) break; }
                    if (frame_no % 2 == 0) part(ex, by, (frand() - 0.5f) * 300, -100 - frand() * 200, 0.3f, 4, 600, 255, 240, 180, 1);
                    if (frame_no % 10 == 0) { smoke(ex, by, 1, 60); splat(ex, by + (frand() - 0.5f) * 10, 7, 0, 30, 20, 12); }
                }
                for (int k = 0; k < 2; k++) { Player *p = &pl[k]; if (!p->active || !p->alive) continue;
                    float px0 = fminf(bx0, bx0 + e->face * 700), px1 = fmaxf(bx0, bx0 + e->face * 700);
                    if (p->x > px0 && p->x < px1 && by > p->y - (p->crouch ? 80 : 124) && by < p->y) hurt_player(p, e->x); }
                if (e->timer <= 0) { e->state = 0; e->timer = 2.0f; e->vx = e->face * 90; }
            }
            break;
        case E_GOAT:   /* like the usherettes, but the accordion fires a fan of confetti bombs */
            if (t && dist < 760) e->face = t->x < e->x ? -1 : 1;
            if (e->state == 0) {
                e->vx = (t && dist < 760 && fabsf(t->x - e->x) > 160) ? e->face * 85 * speed_k : 0;
                e->timer -= DT * speed_k;
                if (e->timer <= 0 && t && dist < 700) { e->state = 1; e->timer = 0.55f; e->vx = 0; }
            } else {
                e->timer -= DT * speed_k;
                if (e->timer <= 0) {
                    for (int k = 0; k < 3; k++) { Proj *q = proj(P_CONFETTI, e->x + e->face * 40, e->y - 90, e->face * (220 + k * 120), -420 - k * 60, 1, 1); if (q) { q->grav = 1100; q->r = 11; } }
                    sfx("boing"); e->state = 0; e->timer = 2.6f + frand();
                }
            }
            break;
        case E_MEATBALL:   /* rolls at you, then pounces */
            if (t && dist < 900) {
                if (e->state == 0) {
                    e->face = t->x < e->x ? -1 : 1;
                    float target = e->face * 230 * speed_k; e->vx += (target - e->vx) * 2.5f * DT;
                    if (e->onground && fabsf(t->x - e->x) < 170 && e->timer <= 0) { e->state = 1; e->vy = -520; e->vx = e->face * 260; e->timer = 1.4f; }
                } else if (e->onground && e->vy >= 0) e->state = 0;
            }
            e->timer -= DT;
            if (e->onground && frame_no % 6 == 0 && fabsf(e->vx) > 60) part_k(PK_BLOOD, e->x, e->y - 6, -e->vx * 0.3f, -80, 0.5f, 3, 1200, 160, 20, 14, 0);
            break;
        case E_BLOB: case E_MINIBLOB:
            e->timer -= DT * speed_k;
            if (e->onground) { e->vx *= 0.85f; if (e->timer <= 0 && t && dist < 800) {
                e->face = t->x < e->x ? -1 : 1; e->vx = e->face * (e->type == E_BLOB ? 170 : 220); e->vy = -430; e->timer = 0.9f + frand() * 0.5f; } }
            break;
        default: e->vx = 0; break;
        }
        e->vy += 2200 * DT; if (e->vy > 1200) e->vy = 1200;
        int was_ground = e->onground;
        int r = move_box(&e->x, &e->y, &e->vx, &e->vy, w, h, 0);
        e->onground = (r & 1) != 0;
        if (!was_ground && e->onground && (e->type == E_BLOB || e->type == E_MINIBLOB)) { e->land = 0.18f; part_k(PK_BLOOD, e->x, e->y - 6, -80, -120, 0.6f, 4, 1200, 12, 8, 12, 0); part_k(PK_BLOOD, e->x, e->y - 6, 80, -120, 0.6f, 4, 1200, 12, 8, 12, 0); }
        if (e->land > 0) e->land -= DT;
        if ((r & 2) && (e->type == E_DOG || e->type == E_MEATBALL)) e->vx = -e->vx * 0.8f;
        conveyor_carry(&e->x, &e->y, w, h);
        if (e->y > FLOOR_Y + 40) { e->alive = 0; ink_burst(e->x, FLOOR_Y, 10, 200); continue; }
        if (e->type != E_CRATE && e->type != E_HATCRATE)
            for (int k = 0; k < 2; k++) { Player *p = &pl[k]; if (p->active && p->alive && !p->hidden && overlap(p->x, p->y, 40, 110, e->x, e->y, w * 0.8f, h * 0.85f)) hurt_player(p, e->x); }
    }
    for (int i = 0; i < ndrip; i++) {   /* ink dripping from the rafters */
        drips[i].t -= DT; float x = drips[i].tx * TILE + 24;
        if (drips[i].t <= 0 && x > camx - 50 && x < camx + SCREEN_W + 50) { Proj *q = proj(P_DRIP, x, 10, 0, 60, 1, 1); if (q) { q->grav = 1300; q->r = 10; } drips[i].t = 1.9f; }
    }
}


/* brick ovens: every few seconds a fire column roars up out of the top. The security office's LIGHTS
   (which also kicks the kitchen breakers) shut the ovens off in that zone */
static float oven_phase(int tx) { return fmodf(scene_t + tx * 0.37f, 3.4f); }
static int oven_burning(int tx) { return lit[zone_of(tx * TILE)] <= 0 && oven_phase(tx) < 1.1f; }
static void update_ovens(void) {
    int t0 = (int)(camx / TILE) - 2, t1 = t0 + SCREEN_W / TILE + 4;
    for (int tx = t0; tx <= t1; tx++) { if (tx < 1 || tx >= LW) continue;
        for (int ty = 1; ty < LH; ty++) { if (map[ty][tx] != 'O' || map[ty - 1][tx] == 'O' || map[ty][tx - 1] == 'O') continue;
            if (!oven_burning(tx)) break;
            float fx = tx * TILE + TILE, top = ty * TILE;
            if (oven_phase(tx) < 0.05f) sfx("sizzle");
            if (frame_no % 2 == 0) { part(fx + (frand() - 0.5f) * 60, top - frand() * 150, (frand() - 0.5f) * 60, -260, 0.3f, 12, -200, 255, 140 + rand() % 80, 40, 1); }
            if (frame_no % 5 == 0) smoke(fx, top - 160, 1, 60);
            for (int k = 0; k < 2; k++) { Player *p = &pl[k]; if (p->active && p->alive && !p->hidden && overlap(p->x, p->y, 40, 120, fx, top, 80, 170)) hurt_player(p, fx); }
            break;
        }
    }
}
/* ------------------------------------------------------------ the Ink Fiend (backstage) */
static int any_player_in_zone(int z) { for (int i = 0; i < 2; i++) if (pl[i].active && pl[i].alive && zone_of(pl[i].x) == z) return 1; return 0; }
static void update_fiend(void) {
    if (level_no != 0) return;   /* the Ink Fiend lives backstage in Show 1 */
    int in_zone = any_player_in_zone(3);
    if (fiend.state == 0) {
        if (!in_zone) return;
        fiend.cycle -= DT;
        if (!fiend.seen) { fiend.seen = 1; fiend.cycle = 3.5f; message("SOMETHING IS MOVING BACKSTAGE...", 2.5f); }
        if (fiend.cycle <= 0) { fiend.state = 1; fiend.t = 1.8f; warn_t = 1.8f; sfx("fiendwarn"); }
        return;
    }
    if (fiend.state == 1) {
        fiend.t -= DT;
        if (fiend.t <= 0) { fiend.state = 2; fiend.x = camx - 120; fiend.y = FLOOR_Y; fiend.t = 0; sfx("roar1"); }
        return;
    }
    if (lit[3] > 0 && fiend.state == 2) { fiend.state = 3; sfx("roar2"); message("THE LIGHTS DROVE IT BACK!", 1.5f); }
    if (fiend.state == 2) {
        fiend.t += DT;
        float d; Player *t = nearest_player(fiend.x, fiend.y, &d);
        float dir = 1, spd = 395;
        if (t) dir = t->x < fiend.x ? -1 : 1; else spd = 300;
        if (!t && fiend.t > 1.0f) dir = 1;
        fiend.x += dir * spd * DT;
        if (frame_no % 3 == 0) part(fiend.x + (frand() - 0.5f) * 140, fiend.y - 60 - frand() * 160, 0, 40, 0.8f, 8 + frand() * 8, 300, 10, 8, 8, 0);
        for (int k = 0; k < 2; k++) { Player *p = &pl[k]; if (p->active && p->alive && !p->hidden && fabsf(p->x - fiend.x) < 90) hurt_player(p, fiend.x); }
        if (fiend.x > camx + SCREEN_W + 200 || fiend.t > 9.0f || !in_zone) { fiend.state = 3; }
    }
    if (fiend.state == 3) {
        fiend.x -= 600 * DT;
        if (fiend.x < camx - 300 || fiend.x > camx + SCREEN_W + 300) { fiend.state = 0; fiend.cycle = 7.0f + frand() * 3; }
    }
}

/* ------------------------------------------------------------ boss: Uncle Inky */
enum { BS_INTRO, BS_IDLE, BS_SLAM_AIM, BS_SLAM_HIT, BS_NOTES, BS_SWEEP_WARN, BS_SWEEP, BS_TICKETS, BS_DEAD };
static void boss_start(void) {
    boss.active = 1; boss.state = BS_INTRO; boss.timer = 2.6f; boss.maxhp = boss.hp = level_no == 1 ? 140 : 110; boss.face = -1;
    if (getenv("MM_BOSS_HP")) boss.hp = atoi(getenv("MM_BOSS_HP"));   /* tests */
    boss.x = ARENA_X + SCREEN_W - 230; boss.y = FLOOR_Y;
    arena_locked = 1; music_play(LEVELS[level_no].boss_music, 1); sfx(level_no == 1 ? "angry" : "roar1"); shake = 10;
}
static void boss_hit_player_box(float x, float y, float w, float h) {
    for (int k = 0; k < 2; k++) { Player *p = &pl[k]; if (p->active && p->alive && overlap(p->x, p->y, 40, p->crouch ? 80 : 120, x, y, w, h)) hurt_player(p, x); }
}
static void update_boss_rat(void);
static void update_boss(void) {
    if (!boss.active) {
        if (!arena_locked) for (int k = 0; k < 2; k++) if (pl[k].active && pl[k].alive && pl[k].x > ARENA_X + 150) boss_start();
        return;
    }
    if (level_no == 1) { update_boss_rat(); return; }
    float angry = boss.hp < boss.maxhp / 2 ? 0.7f : 1.0f;
    boss.t += DT; boss.bob = sinf(boss.t * 2.2f) * 10;
    if (boss.flash > 0) boss.flash--;
    boss.timer -= DT;
    Player *t = nearest_player(boss.x, boss.y, NULL);
    switch (boss.state) {
    case BS_INTRO: boss.pose = 0; if (boss.timer <= 0) { boss.state = BS_IDLE; boss.timer = 1.0f; } break;
    case BS_IDLE:
        boss.pose = angry < 1 ? 2 : 0;
        if (boss.timer <= 0) {
            int pick_ = rand() % (angry < 1 ? 4 : 3);
            if (pick_ == 0) { boss.state = BS_SLAM_AIM; boss.slams = angry < 1 ? 4 : 3; boss.timer = 0.85f * angry; boss.targetx = t ? t->x : boss.x - 400; }
            else if (pick_ == 1) { boss.state = BS_NOTES; boss.timer = 0.4f; boss.slams = 3; }
            else if (pick_ == 2) { boss.state = BS_SWEEP_WARN; boss.timer = 0.9f * angry; sfx("charge"); }
            else { boss.state = BS_TICKETS; boss.timer = 2.2f; boss.slams = 0; }
            if (angry < 1 && !boss.summoned) { boss.summoned = 1; spawn(E_BLOB, (int)((ARENA_X + 300) / TILE), 13); spawn(E_BLOB, (int)((ARENA_X + 700) / TILE), 13); sfx("summon"); }
        }
        break;
    case BS_SLAM_AIM:
        boss.pose = 1;
        if (boss.timer > 0.3f && t) boss.targetx += (t->x - boss.targetx) * 0.08f;   /* tracks you, then commits */
        if (boss.timer <= 0) {
            boss.state = BS_SLAM_HIT; boss.timer = 0.32f; shake = 14; sfx("slam"); hitstop = 2;
            ink_burst(boss.targetx, FLOOR_Y, 16, 300); ring(boss.targetx, FLOOR_Y - 10, 180, 255, 230, 190);
            for (int k = 0; k < 10; k++) part_k(PK_DEBRIS, boss.targetx + (frand() - 0.5f) * 60, FLOOR_Y - 8, (frand() - 0.5f) * 700, -300 - frand() * 500, 2.0f, 8 + frand() * 10, 1700, 110, 70, 40, 0);
            for (int k = 0; k < 12; k++) part_k(PK_BLOOD, boss.targetx, FLOOR_Y - 10, (frand() - 0.5f) * 600, -300 - frand() * 600, 1.4f, 5 + frand() * 6, 1500, 12, 8, 12, 0);
            smoke(boss.targetx, FLOOR_Y - 10, 5, 80);
        }
        break;
    case BS_SLAM_HIT:
        boss.pose = 1;
        boss_hit_player_box(boss.targetx, FLOOR_Y, 86, FLOOR_Y);
        if (boss.timer <= 0) {
            if (--boss.slams > 0) { boss.state = BS_SLAM_AIM; boss.timer = 0.6f * angry; boss.targetx = t ? t->x : boss.targetx; }
            else { boss.state = BS_IDLE; boss.timer = 1.3f * angry; boss.summoned = boss.hp < boss.maxhp / 2 ? boss.summoned : 0; }
        }
        break;
    case BS_NOTES:
        boss.pose = 0;
        if (boss.timer <= 0) {
            float tx = t ? t->x : boss.x - 500, dx = tx - (boss.x - 120);
            Proj *q = proj(P_NOTE, boss.x - 120, boss.y - 300, dx * (0.75f + 0.25f * boss.slams) / 1.1f, -700, 1, 1);
            if (q) { q->grav = 1300; q->r = 18; q->bounces = 2; q->life = 4; }
            sfx("spit"); boss.timer = 0.45f * angry;
            if (--boss.slams <= 0) { boss.state = BS_IDLE; boss.timer = 1.4f * angry; }
        }
        break;
    case BS_SWEEP_WARN:
        boss.pose = 0; if (boss.timer <= 0) { boss.state = BS_SWEEP; boss.sweepx = boss.x - 140; sfx("beam"); } break;
    case BS_SWEEP:
        boss.sweepx -= 760 / angry * DT;
        if (frame_no % 2 == 0) part(boss.sweepx, FLOOR_Y - 6, (frand() - 0.5f) * 200, -150 - frand() * 250, 0.4f, 4, 900, 255, 230, 150, 1);
        if (frame_no % 5 == 0) splat(boss.sweepx, FLOOR_Y + 2, 12, 1, 40, 26, 14);
        boss_hit_player_box(boss.sweepx, FLOOR_Y, 70, 52);
        if (boss.sweepx < ARENA_X - 60) { boss.state = BS_IDLE; boss.timer = 1.2f * angry; }
        break;
    case BS_TICKETS:
        boss.pose = 2;
        if (frame_no % 9 == 0) { Proj *q = proj(P_TICKET, ARENA_X + 40 + frand() * (SCREEN_W - 340), -20, (frand() - 0.5f) * 60, 160, 1, 1); if (q) { q->r = 14; q->grav = 300; } }
        if (boss.timer <= 0) { boss.state = BS_IDLE; boss.timer = 1.2f; }
        break;
    case BS_DEAD:
        if (boss.pose >= 0) boss.pose = 2;
        if (frame_no % 4 == 0) { ink_burst(boss.x + (frand() - 0.5f) * 260, boss.y - frand() * 380, 8, 300); sfx("boom");
            blood_spurt(boss.x + (frand() - 0.5f) * 200, boss.y - frand() * 300, frand() < 0.5f ? -1 : 1, 10, 700);
            gore_part(rand() % 8, boss.x + (frand() - 0.5f) * 200, boss.y - 100 - frand() * 250, 700, rand() & 1);
            if (frame_no % 12 == 0) ring(boss.x + (frand() - 0.5f) * 200, boss.y - frand() * 300, 120, 255, 200, 120); }
        if (!boss.summoned && boss.timer < 1.0f) {   /* final blow: he comes apart, mask and all */
            boss.summoned = 1; shatter(&A.inky[2], boss.x, boss.y + 10, 0, 5, 5, 800, 1); boss.pose = -1;
            for (int k = 0; k < 6; k++) gore_part(2, boss.x - 30, boss.y - 330, 800, 0);
            for (int k = 0; k < 4; k++) gore_part(4, boss.x, boss.y - 200, 700, 1);
            gore_part(3, boss.x - 20, boss.y - 320, 900, 1); gore_part(3, boss.x + 20, boss.y - 320, 900, 1);
            blood_spurt(boss.x, boss.y - 250, -1, 60, 900); blood_spurt(boss.x, boss.y - 250, 1, 40, 800);
            ring(boss.x, boss.y - 200, 300, 255, 220, 160); shake = 24; hitstop = 10;
        }
        boss.y += 40 * DT;
        if (boss.timer <= 0 && scene == SC_PLAY) { scene = SC_CLEAR; scene_t = 0; music_play("mm_clear", 0); }
        break;
    }
    /* body contact */
    if (boss.state != BS_DEAD && boss.state != BS_INTRO) boss_hit_player_box(boss.x, boss.y, 200, 300);
}


static float sx(float x);
static void draw_sprite(Tex *t, float x, float feet, float scale, int face_right, Uint8 flash, Uint8 alpha, double angle, float sy);
/* ---- Show 2 boss: BIG CHEESE, the rat chef ---- */
enum { RS_CHOP_AIM = 20, RS_CHOP, RS_PIZZA, RS_WAVE_WARN, RS_WAVE, RS_LEAP, RS_FALL };
static void update_boss_rat(void) {
    float angry = boss.hp < boss.maxhp / 2 ? 0.7f : 1.0f;
    boss.t += DT; if (boss.flash > 0) boss.flash--;
    boss.timer -= DT;
    Player *t = nearest_player(boss.x, boss.y, NULL);
    float lo = ARENA_X + 220, hi = ARENA_X + SCREEN_W - 180;
    if (t && boss.state != RS_LEAP && boss.state != RS_FALL && boss.state != BS_DEAD) boss.face = t->x < boss.x ? -1 : 1;
    switch (boss.state) {
    case BS_INTRO: boss.pose = 0; if (boss.timer <= 0) { boss.state = BS_IDLE; boss.timer = 1.0f; } break;
    case BS_IDLE:
        boss.pose = angry < 1 ? 4 : 0; boss.bob = fabsf(sinf(boss.t * 6)) * -10;
        if (t) { float want = t->x - boss.face * 380; boss.x += clampf(want - boss.x, -1, 1) * 110 / angry * DT; }
        boss.x = clampf(boss.x, lo, hi);
        if (boss.timer <= 0) {
            int pick_ = rand() % (angry < 1 ? 4 : 3);
            if (pick_ == 0) { boss.state = RS_CHOP_AIM; boss.slams = angry < 1 ? 3 : 2; boss.timer = 0.8f * angry; boss.targetx = t ? t->x : boss.x; }
            else if (pick_ == 1) { boss.state = RS_PIZZA; boss.slams = 3; boss.timer = 0.3f; }
            else if (pick_ == 2) { boss.state = RS_WAVE_WARN; boss.timer = 0.8f * angry; sfx("charge"); }
            else { boss.state = RS_LEAP; boss.vy = -2100; sfx("jump"); }
            if (angry < 1 && !boss.summoned) { boss.summoned = 1; spawn(E_MEATBALL, (int)((ARENA_X + 200) / TILE), 13); spawn(E_MEATBALL, (int)((ARENA_X + 900) / TILE), 13); sfx("summon"); }
        }
        break;
    case RS_CHOP_AIM:   /* a cleaver shadow tracks you, then commits */
        boss.pose = 1;
        if (boss.timer > 0.28f && t) boss.targetx += (t->x - boss.targetx) * 0.09f;
        if (boss.timer <= 0) { boss.state = RS_CHOP; boss.timer = 0.3f; shake = 14; hitstop = 2; sfx("slam");
            ring(boss.targetx, FLOOR_Y - 10, 150, 255, 230, 190); spark_burst(boss.targetx, FLOOR_Y - 10, 20, 255, 240, 200);
            for (int k = 0; k < 8; k++) part_k(PK_DEBRIS, boss.targetx, FLOOR_Y - 8, (frand() - 0.5f) * 700, -300 - frand() * 500, 2.0f, 8 + frand() * 10, 1700, 110, 70, 40, 0); }
        break;
    case RS_CHOP:
        boss.pose = 2; boss_hit_player_box(boss.targetx, FLOOR_Y, 110, FLOOR_Y);
        if (boss.timer <= 0) { if (--boss.slams > 0) { boss.state = RS_CHOP_AIM; boss.timer = 0.55f * angry; boss.targetx = t ? t->x : boss.targetx; }
            else { boss.state = BS_IDLE; boss.timer = 1.3f * angry; } }
        break;
    case RS_PIZZA:      /* flings pizzas that bounce along the floor */
        boss.pose = boss.timer > 0.5f * angry - 0.22f && boss.slams < 3 ? 3 : 0;
        if (boss.timer <= 0) {
            Proj *q = proj(P_PIZZA, boss.x + boss.face * 120, boss.y - 260, boss.face * (330 + 70 * boss.slams), -520, 1, 1);
            if (q) { q->grav = 1400; q->r = 26; q->bounces = 4; q->life = 4; }
            sfx("boing"); boss.timer = 0.5f * angry;
            if (--boss.slams <= 0) { boss.state = BS_IDLE; boss.timer = 1.2f * angry; }
        }
        break;
    case RS_WAVE_WARN: boss.pose = 1; if (boss.timer <= 0) { boss.state = RS_WAVE; boss.sweepx = boss.x + boss.face * 120; sfx("splash"); shake = 8; } break;
    case RS_WAVE:       /* a wave of hot cheese rolls across the floor: jump it */
        boss.sweepx += boss.face * 700 / angry * DT;
        boss_hit_player_box(boss.sweepx, FLOOR_Y, 80, 64);
        if (frame_no % 2 == 0) part_k(PK_BLOOD, boss.sweepx, FLOOR_Y - 30, (frand() - 0.5f) * 200, -200 - frand() * 200, 0.8f, 6, 1200, 250, 190, 60, 0);
        if (boss.sweepx < ARENA_X - 60 || boss.sweepx > ARENA_X + SCREEN_W + 60) { boss.state = BS_IDLE; boss.timer = 1.1f * angry; }
        break;
    case RS_LEAP:       /* jumps off the top of the screen... */
        boss.pose = 4; boss.vy += 1800 * DT; boss.y += boss.vy * DT;
        if (boss.y < -200 || (boss.vy > 0 && boss.y > -60)) { boss.state = RS_FALL; boss.timer = 1.0f; boss.targetx = t ? t->x : boss.x; boss.vy = 0; }
        break;
    case RS_FALL:       /* ...and crashes down where you were standing */
        boss.pose = 4;
        if (boss.timer > 0.25f && t) boss.targetx += (t->x - boss.targetx) * 0.06f;
        if (boss.timer <= 0) {
            boss.x = clampf(boss.targetx, lo, hi); boss.vy += 3200 * DT; boss.y += boss.vy * DT;
            if (boss.y >= FLOOR_Y) { boss.y = FLOOR_Y; boss.state = BS_IDLE; boss.timer = 1.4f; shake = 20; hitstop = 4; sfx("slam"); sfx("boom");
                ring(boss.x, FLOOR_Y - 10, 260, 255, 220, 160); smoke(boss.x, FLOOR_Y - 10, 8, 80);
                boss_hit_player_box(boss.x, FLOOR_Y, 260, 300);
                for (int k = 0; k < 2; k++) { Player *p = &pl[k]; if (p->active && p->alive && p->onground && fabsf(p->x - boss.x) < 520) hurt_player(p, boss.x); }  /* shockwave: be in the air */
            }
        }
        break;
    case BS_DEAD:
        if (boss.pose >= 0) boss.pose = 4;
        if (frame_no % 4 == 0) { sfx("boom"); blood_spurt(boss.x + (frand() - 0.5f) * 200, boss.y - frand() * 300, frand() < 0.5f ? -1 : 1, 10, 700);
            gore_part2(rand() % 12, boss.x + (frand() - 0.5f) * 200, boss.y - 100 - frand() * 250, 700, rand() & 1);
            for (int k = 0; k < 6; k++) part_k(PK_BLOOD, boss.x, boss.y - 200, (frand() - 0.5f) * 800, -300 - frand() * 500, 1.4f, 6, 1500, 250, 190, 60, 0); }
        if (!boss.summoned && boss.timer < 1.0f) {
            boss.summoned = 1; shatter(&A.rat[4], boss.x, boss.y + 10, boss.face > 0, 5, 5, 800, 1); boss.pose = -1;
            for (int k = 0; k < 6; k++) gore_part2(11, boss.x, boss.y - 200, 800, 1);
            gore_part2(3, boss.x - 20, boss.y - 330, 900, 1); gore_part2(3, boss.x + 20, boss.y - 330, 900, 1);
            blood_spurt(boss.x, boss.y - 250, -1, 60, 900); blood_spurt(boss.x, boss.y - 250, 1, 40, 800);
            ring(boss.x, boss.y - 200, 300, 255, 220, 160); shake = 24; hitstop = 10;
        }
        if (boss.timer <= 0 && scene == SC_PLAY) { scene = SC_CLEAR; scene_t = 0; music_play("mm_clear", 0); }
        break;
    }
    if (boss.state != BS_DEAD && boss.state != BS_INTRO && boss.state != RS_LEAP && boss.state != RS_FALL) boss_hit_player_box(boss.x, boss.y, 190, 300);
}
static void draw_boss_rat(float ox, float oy) {
    float x = sx(boss.x) + ox, y = boss.y + oy + boss.bob;
    if (boss.state == RS_CHOP_AIM) { float k = 1 - boss.timer / 0.8f, tx = sx(boss.targetx) + ox;
        draw_ellipse(&A.disc, tx, FLOOR_Y + oy, 40 + 30 * k, 9 + 6 * k, 10, 6, 10, (Uint8)(120 + 120 * k));
        if ((frame_no / 4) % 2) draw_text_col(&A.small, "!", tx, FLOOR_Y + oy - 60, 0.8f, 1, 255, 60, 40, 255); }
    if (boss.state == RS_CHOP) { float tx = sx(boss.targetx) + ox;   /* his cleaver, giant, slams down blade-first */
        float k = clampf((0.3f - boss.timer) / 0.06f, 0, 1), h = A.cleaver.h ? (float)A.cleaver.h : 300;
        float cy = -h + (FLOOR_Y + oy + 16) * k;   /* bottom edge reaches the floor */
        if (A.cleaver.tex) draw_tex_srcx(&A.cleaver, NULL, tx - A.cleaver.w * 0.5f, cy, (float)A.cleaver.w, h, boss.face < 0, 0, 255, 255, 255, 255);
        else { fill_rect((int)tx - 50, 0, 100, FLOOR_Y + (int)oy - 16, 190, 196, 205, 255); } }
    if (boss.state == RS_FALL && boss.timer > 0) { float k = 1 - boss.timer, tx = sx(boss.targetx) + ox;
        draw_ellipse(&A.disc, tx, FLOOR_Y + oy, 60 + 120 * k, 12 + 12 * k, 10, 6, 10, (Uint8)(100 + 140 * k)); }
    if (boss.state == RS_WAVE_WARN && (frame_no / 5) % 2) draw_glow(&A.disc, x + boss.face * 120, FLOOR_Y + oy - 30, 70, 255, 200, 80, 255);
    if (boss.state == RS_WAVE) { float bx = sx(boss.sweepx) + ox;
        for (int k = 0; k < 6; k++) draw_ellipse(&A.disc, bx - boss.face * k * 22, FLOOR_Y + oy - 22 - (5 - k) * 6, 40 - k * 4, 30 - k * 3, 245, 190 - k * 10, 60, 255);
        draw_ellipse(&A.disc, bx - 8, FLOOR_Y + oy - 50, 8, 5, 255, 240, 200, 220); }
    if (boss.pose < 0) return;
    Tex *t = &A.rat[boss.pose];
    double ang = boss.state == BS_DEAD ? sinf(frame_no * 0.5f) * 6 : sinf(boss.t * 1.6f) * 2;
    draw_sprite(t, x, y + 10, 1, boss.face > 0, boss.flash ? 170 : 0, 255, ang, 1.0f + 0.02f * sinf(boss.t * 5));
}
/* ------------------------------------------------------------ projectiles, pickups, particles */
static void update_proj(void) {
    for (int i = 0; i < MAXP; i++) {
        Proj *p = &pr[i]; if (!p->alive) continue;
        p->life -= DT; if (p->life <= 0) { p->alive = 0; continue; }
        p->vy += p->grav * DT; p->x += p->vx * DT; p->y += p->vy * DT;
        if (p->x < camx - 100 || p->x > camx + SCREEN_W + 100 || p->y > LH * TILE + 50) { p->alive = 0; continue; }
        int tx = (int)(p->x / TILE), ty = (int)(p->y / TILE);
        if (solid_at(tx, ty) && p->y > 0) {
            if (p->type == P_NOTE && p->bounces > 0) { p->bounces--; p->vy = -650; p->y = ty * TILE - 1; sfx("boing"); splat(p->x, ty * TILE, 14, 1, 12, 8, 12); continue; }
            if (p->type == P_PIZZA && p->vy >= 0 && p->bounces > 0) { p->bounces--; p->vy = -380; p->y = ty * TILE - 1; splat(p->x, ty * TILE, 10, 1, 170, 30, 20); continue; }
            if (p->type == P_CONFETTI) { for (int i = 0; i < 12; i++) part(p->x, p->y, (frand() - 0.5f) * 400, -100 - frand() * 300, 0.8f, 4, 900, 120 + rand() % 136, 80 + rand() % 176, 80 + rand() % 176, 0); spark_burst(p->x, p->y, 6, 255, 220, 120); }
            if (p->type == P_DRIP || p->type == P_NOTE) { ink_burst(p->x, p->y, 6, 140); splat(p->x, ty * TILE, 12, 1, 12, 8, 12); }
            else if (p->type == P_RIVET) { spark_burst(p->x, p->y, 10, 255, 160, 60); smoke(p->x, p->y, 2, 90); ring(p->x, p->y, 40, 255, 170, 80); }
            else if (p->type == P_NAIL) { spark_burst(p->x, p->y, 5, 255, 230, 160); splat(p->x, p->y, 3, 0, 40, 36, 34); }
            else if (p->type == P_PLUNGER) { sfx("boing"); splat(p->x, p->y, 9, 0, 170, 30, 30); }
            else if (p->type == P_FLAME) { embers(p->x, p->y, 2); if (rand() % 3 == 0) splat(p->x, p->y, 6, 0, 30, 20, 14); }
            else if (!p->enemy) spark_burst(p->x, p->y, 4, 255, 220, 150);
            if (p->type == P_LIGHT) { spark_burst(p->x, p->y, 8, 255, 240, 170); splat(p->x, p->y, 8, 0, 40, 28, 16); }
            p->alive = 0; continue;
        }
        /* trails */
        if (p->type == P_FLAME) { if (frame_no % 2 == 0) part(p->x, p->y, p->vx * 0.1f, -60, 0.2f, 10, -100, 140, 190, 255, 1); if (rand() % 4 == 0) embers(p->x, p->y, 1); }
        if (p->type == P_RIVET && frame_no % 2 == 0) part(p->x, p->y, 0, 0, 0.18f, 6, 0, 255, 120, 30, 1);
        if (p->type == P_LIGHT && frame_no % 2 == 0) part(p->x, p->y, 0, 0, 0.25f, 9, 0, 255, 230, 150, 1);
        if (p->type == P_NOTE && frame_no % 3 == 0) part_k(PK_BLOOD, p->x, p->y, 0, 0, 1.0f, 4, 900, 12, 8, 12, 0);
        if (p->type == P_PLUNGER) p->r = 14;
        if (p->enemy) {
            for (int k = 0; k < 2; k++) { Player *q = &pl[k];
                if (q->active && q->alive && !q->hidden && overlap(q->x, q->y, 40, q->crouch ? 76 : 118, p->x, p->y + p->r, p->r * 2, p->r * 2)) {
                    hurt_player(q, p->x); if (p->type != P_NOTE) p->alive = 0; } }
        } else {
            for (int j = 0; j < nen && p->alive; j++) { Enemy *e = &en[j]; if (!e->alive || j == p->last_hit) continue;
                float w, h; enemy_box(e, &w, &h);
                if (overlap(e->x, e->y, w, h, p->x, p->y + p->r, p->r * 2, p->r * 2)) {
                    if (p->type == P_FLAME) e->burn = 2.2f;                      /* set them on fire */
                    if (p->type == P_RIVET) { ring(p->x, p->y, 50, 255, 170, 80); smoke(p->x, p->y, 2, 90); }
                    if (p->type == P_PLUNGER) { e->stun = 0.6f; sfx("boing"); message("THWONK!", 0.5f); }
                    enemy_damage(e, p->dmg); spark_burst(p->x, p->y, 7, 255, 240, 200);
                    if (p->type == P_FLAME) p->last_hit = j; else p->alive = 0; } }
            if (p->alive && boss.active && boss.state != BS_DEAD && boss.state != BS_INTRO &&
                overlap(boss.x, boss.y + boss.bob - 40, 220, 360, p->x, p->y + p->r, p->r * 2, p->r * 2)) {
                boss.hp -= p->dmg; boss.flash = 5; spark_burst(p->x, p->y, 6, 255, 240, 200);
                if (rand() % 3 == 0) blood_spurt(p->x, p->y, p->vx > 0 ? -1 : 1, 4, 400);
                if (p->type != P_FLAME) p->alive = 0; else p->life -= 0.1f;
                if (frame_no % 6 == 0) sfx("hit");
                if (boss.hp <= 0) {   /* his band dies with him */
                    for (int j = 0; j < nen; j++) if (en[j].alive && en[j].type < E_CRATE) { gore_enemy(&en[j]); en[j].alive = 0; }
                    for (int j = 0; j < MAXP; j++) if (pr[j].enemy) pr[j].alive = 0;
                    boss.pose = 2;
                }
                if (boss.hp <= 0) { boss.summoned = 0; boss.state = BS_DEAD; boss.timer = 3.5f; score += 10000; message(level_no == 1 ? "BIG CHEESE IS TOAST!" : "UNCLE INKY TAKES A BOW!", 3.0f); shake = 16; sfx("roar2"); hitstop = 18; }
                else if (boss.hp < boss.maxhp / 2 && boss.hp + p->dmg >= boss.maxhp / 2) { sfx("angry"); message("HE'S STEAMED!", 1.5f); shake = 10; }
            }
        }
    }
    for (int i = 0; i < 8; i++) { Pickup *k = &pick[i]; if (!k->alive) continue;
        k->t += DT; k->vy += 1400 * DT; float vx = 0;
        move_box(&k->x, &k->y, &vx, &k->vy, 40, 40, 0); if (k->t > 14) k->alive = 0; }
    for (int i = 0; i < MAXPART; i++) { Part *q = &parts[i]; if (q->life <= 0) continue;
        q->life -= DT; q->vy += q->grav * DT; q->x += q->vx * DT; q->y += q->vy * DT; q->ang += q->vang * DT;
        if (q->kind == PK_SMOKE) { q->size += 18 * DT; q->vx *= 0.98f; }
        if (q->kind == PK_BLOOD || q->kind == PK_CASING || q->kind == PK_DEBRIS) {
            int tx = (int)(q->x / TILE), ty = (int)(q->y / TILE);
            int hit = solid_at(tx, ty) || map[ty < 0 ? 0 : ty >= LH ? LH - 1 : ty][tx < 0 ? 0 : tx >= LW ? LW - 1 : tx] == '~';
            if (hit && q->y > 0) {
                if (q->kind == PK_BLOOD) {   /* lands: leaves a stain on the floor or wall */
                    int floor = q->vy > 0 && !solid_at(tx, (int)((q->y - 14) / TILE));
                    splat(q->x, floor ? ty * TILE + 2 : q->y, q->size * 1.2f + 2, floor, q->r, q->g, q->b); q->life = 0;
                } else {                     /* casings and splinters bounce and settle */
                    if (q->kind == PK_CASING && q->vy > 300 && rand() % 4 == 0 && q->x > camx && q->x < camx + SCREEN_W) sfx("casing");
                    q->y = ty * TILE - 1; q->vy *= -0.35f; q->vx *= 0.6f; q->vang *= 0.5f;
                    if (fabsf(q->vy) < 60) { q->vy = 0; q->grav = 0; q->vx = 0; q->vang = 0; }
                }
            }
        }
    }
    /* gibs */
    for (int i = 0; i < MAXGIB; i++) { Gib *g = &gibs[i]; if (g->life <= 0) continue;
        g->life -= DT; if (g->rest) continue;
        g->vy += 1700 * DT; g->x += g->vx * DT; g->y += g->vy * DT; g->ang += g->vang * DT;
        if (g->bleed && frame_no % 4 == 0) part_k(PK_BLOOD, g->x, g->y, (frand() - 0.5f) * 40, 0, 1.0f, 3 + frand() * 2, 1200, 140, 12, 16, 0);
        int tx = (int)(g->x / TILE), ty = (int)((g->y + g->h * 0.3f) / TILE);
        if (solid_at(tx, ty) && g->vy > 0) {
            g->y = ty * TILE - g->h * 0.3f; g->vy *= -0.4f; g->vx *= 0.55f; g->vang *= 0.5f;
            if (g->bleed) splat(g->x, ty * TILE + 2, 10, 1, 130, 10, 14);
            if (fabsf(g->vy) < 80) { g->rest = 1; g->vang = 0; }
        }
        if (g->y > LH * TILE + 100) g->life = 0;
    }
    for (int i = 0; i < 16; i++) if (rings[i].life > 0) { rings[i].life -= DT; rings[i].r += (rings[i].max - rings[i].r) * 0.25f; }
    /* burning enemies */
    for (int i = 0; i < nen; i++) { Enemy *e = &en[i]; if (!e->alive || e->burn <= 0) continue;
        float w, h; enemy_box(e, &w, &h);
        e->burn -= DT; e->btick -= DT;
        if (frame_no % 2 == 0) part(e->x + (frand() - 0.5f) * w, e->y - frand() * h, (frand() - 0.5f) * 40, -120, 0.35f, 9 + frand() * 6, -150, 255, 120 + rand() % 90, 30, 1);
        if (frame_no % 6 == 0) { smoke(e->x, e->y - h, 1, 50); embers(e->x, e->y - h * 0.5f, 1); }
        if (e->btick <= 0) { e->btick = 0.45f; enemy_damage(e, 1); }
    }
}

/* ------------------------------------------------------------ security office (GamePad) actions */
static void op_ping(void) {
    if (ping_cool > 0 || power < 6) { sfx("tick"); return; }
    power -= 6; ping_cool = 1.0f; int n = 0;
    for (int i = 0; i < nen; i++) if (en[i].alive && zone_of(en[i].x) == cam_sel && en[i].type != E_CRATE && en[i].type != E_HATCRATE) { en[i].marked = 6; n++; }
    sfx("coin"); cam_flash = 0.3f;
    char b[48]; snprintf(b, sizeof b, "OFFICE PINGED %d TARGET%s", n, n == 1 ? "" : "S"); message(b, 1.4f);
}
static void op_lights(void) {
    if (power < 10 || lit[cam_sel] > 0) { sfx("tick"); return; }
    power -= 10; lit[cam_sel] = 8.0f; sfx("lights"); cam_flash = 0.4f;
    char b[48]; snprintf(b, sizeof b, "LIGHTS UP: %s", ZONE_NAME[cam_sel]); message(b, 1.4f);
}
static void auto_camera(void) {   /* nobody on the cameras for a while: follow the action */
    op_idle += DT; if (op_idle < 6.0f) return;
    int z = zone_of(pl[0].active ? pl[0].x : pl[1].x);
    if (fiend.state >= 1) z = 3;
    if (boss.active) z = 5;
    if (z != cam_sel && frame_no % 30 == 0) { cam_sel = z; cam_flash = 0.15f; }
}
static void op_input(const Input *in) {
    if (in->pressed & (B_CAMPREV | B_CAMNEXT | B_PING | B_LIGHTS)) op_idle = 0;
    if (in->pressed & B_CAMPREV) { cam_sel = (cam_sel + 5) % 6; sfx("menu"); cam_flash = 0.15f; }
    if (in->pressed & B_CAMNEXT) { cam_sel = (cam_sel + 1) % 6; sfx("menu"); cam_flash = 0.15f; }
    if (in->pressed & B_PING) op_ping();
    if (in->pressed & B_LIGHTS) op_lights();
}

/* GamePad button layout (854x480) shared by drawing and touch */
static SDL_Rect cam_btn(int i) { return (SDL_Rect){560 + (i % 2) * 142, 58 + (i / 2) * 82, 134, 74}; }
static const SDL_Rect BTN_PING = {16, 404, 170, 62}, BTN_LIGHTS = {196, 404, 170, 62};
static int in_rect(SDL_Rect r, float x, float y) { return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h; }

void game_touch(float fx, float fy) {
    float x = fx * PAD_W, y = fy * PAD_H;
    if (scene != SC_PLAY) return;
    op_idle = 0;
    for (int i = 0; i < 6; i++) if (in_rect(cam_btn(i), x, y)) { cam_sel = i; sfx("menu"); cam_flash = 0.15f; return; }
    if (in_rect(BTN_PING, x, y)) op_ping();
    if (in_rect(BTN_LIGHTS, x, y)) op_lights();
}

/* ------------------------------------------------------------ step */
void game_step(const Input pads[MAX_PADS], const int connected[MAX_PADS]) {
    frame_no++; scene_t += DT;
    if (shake > 0) shake = fmaxf(0, shake - 30 * DT);
    if (msg_t > 0) msg_t -= DT;
    if (warn_t > 0) warn_t -= DT;
    if (cam_flash > 0) cam_flash -= DT;
    if (ping_cool > 0) ping_cool -= DT;
    unsigned anyp = 0; for (int i = 0; i < MAX_PADS; i++) anyp |= pads[i].pressed;

    if (scene == SC_TITLE) {
        if (anyp & B_LEFT) { level_no = (level_no + NUM_LEVELS - 1) % NUM_LEVELS; sfx("menu"); }
        if (anyp & B_RIGHT) { level_no = (level_no + 1) % NUM_LEVELS; sfx("menu"); }
        if (anyp & B_UP) { menu_sel = (menu_sel + 2) % 3; sfx("menu"); }
        if (anyp & B_DOWN) { menu_sel = (menu_sel + 1) % 3; sfx("menu"); }
        if (anyp & (B_START | B_JUMP | B_SHOOT)) {
            if (menu_sel == 0) { mode = 0; sfx("checkpoint"); start_level(); }
            else if (menu_sel == 1) {
                if (connected[1]) { mode = 1; sfx("checkpoint"); start_level(); }
                else { sfx("tick"); message("CONNECT A WII REMOTE OR PRO CONTROLLER FOR THE RUNNER", 2.5f); }
            } else { sfx("tick"); message("POSSESSED PARTY MODE IS COMING SOON", 2.0f); }
        }
        return;
    }
    if (scene == SC_OVER || scene == SC_CLEAR) {
        if (scene_t > 2.0f && (anyp & (B_START | B_JUMP))) {
            if (scene == SC_CLEAR && level_no + 1 < NUM_LEVELS) {   /* on to the next show, keeping your crew */
                Player keep[2]; memcpy(keep, pl, sizeof keep); int sc = score;
                level_no++; start_level(); score = sc;
                for (int i = 0; i < 2; i++) if (keep[i].active) { pl[i].active = 1; pl[i].pad = keep[i].pad; pl[i].lives = keep[i].lives; pl[i].weapon = keep[i].weapon; reset_player(&pl[i], 0); }
            } else { scene = SC_TITLE; music_play("mm_title", 1); }
        }
        if (scene == SC_CLEAR) update_proj();
        return;
    }
    if (hitstop > 0) { hitstop--; return; }
    /* drop-in second runner */
    int p2pad = mode == 1 ? 2 : 1;
    if (!pl[1].active && connected[p2pad] && (pads[p2pad].pressed & B_START) && pl[0].active) {
        reset_player(&pl[1], 1); pl[1].active = 1; pl[1].pad = p2pad; pl[1].x = pl[0].x - 50; pl[1].y = pl[0].y; message("PLAYER 2 CLOCKS IN!", 1.5f); sfx("checkpoint");
    }
    /* power drains; at zero the lights go out */
    float drain = 0.45f + (boss.active ? 0.25f : 0);
    power = clampf(power - drain * DT, 0, 100);
    for (int z = 0; z < 6; z++) if (lit[z] > 0) lit[z] -= DT;
    int cz = zone_of(camx + SCREEN_W / 2);
    int was_dark = darkness_on;
    darkness_on = power <= 0 && lit[cz] <= 0;
    if (darkness_on && !was_dark && power <= 0) { sfx("thunder"); message("POWER OUT! FIND A BREAKER", 2.0f); }

    if (!boss.active) {   /* each area has its own take on the theme; missing tracks fall back to the main one */
        const char *const *ZONE_MUSIC = LEVELS[level_no].music;
        Player *lead = pl[0].active ? &pl[0] : &pl[1];
        const char *want = ZONE_MUSIC[zone_of(lead->x)];
        static int avail[NUM_LEVELS][6] = {{-1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1}};
        int z = zone_of(lead->x);
        if (avail[level_no][z] < 0) avail[level_no][z] = music_available(want);
        music_play(avail[level_no][z] ? want : LEVELS[level_no].music[0], 1);
    }
    for (int i = 0; i < 2; i++) update_player(&pl[i], &pads[pl[i].pad]);
    op_input(&pads[op_pad]); auto_camera();
    update_enemies(); update_fiend(); update_ovens(); update_boss(); update_proj();

    /* camera */
    float tx = 0; int n = 0;
    for (int i = 0; i < 2; i++) if (pl[i].active && (pl[i].alive || pl[i].dead_t < 1)) { tx += pl[i].x; n++; }
    if (n) {
        tx /= n;
        float want = tx - SCREEN_W * 0.42f;
        if (arena_locked) want = ARENA_X;
        want = clampf(want, 0, LW * TILE - SCREEN_W);
        camx += (want - camx) * (arena_locked ? 0.08f : 0.18f);
    }
    /* game over */
    if (!pl[0].active && !pl[1].active) { scene = SC_OVER; scene_t = 0; music_play("mm_gameover", 0); }
}

/* ------------------------------------------------------------ drawing: TV */
static float sx(float x) { return x - camx; }

static void draw_tiles(float ox, float oy) {
    int t0 = (int)(camx / TILE) - 1, t1 = t0 + SCREEN_W / TILE + 3;
    for (int ty = 0; ty < LH; ty++) for (int tx = t0; tx <= t1; tx++) {
        if (tx < 0 || tx >= LW) continue;
        char c = map[ty][tx]; float x = sx(tx * TILE) + ox, y = ty * TILE + oy;
        if (c == '#') {
            int top = ty == 0 || map[ty - 1][tx] != '#';
            fill_rect((int)x, (int)y, TILE, TILE, 72, 44, 28, 255);
            fill_rect((int)x, (int)y + 15, TILE, 2, 40, 22, 14, 255);
            fill_rect((int)x, (int)y + 31, TILE, 2, 40, 22, 14, 255);
            fill_rect((int)x + ((tx * 7 + ty * 3) % 3) * 16, (int)y, 2, TILE, 44, 26, 16, 255);
            if (top) { fill_rect((int)x, (int)y, TILE, 6, 150, 104, 62, 255); fill_rect((int)x, (int)y + 6, TILE, 3, 20, 10, 6, 255);
                if ((tx * 13) % 7 == 0) draw_ellipse(&A.disc, x + 24, y + 9, 7, 11 + (tx % 3) * 4, 12, 8, 8, 255); }
        } else if (c == '=') {
            fill_rect((int)x, (int)y, TILE, 10, 58, 52, 50, 255);
            fill_rect((int)x, (int)y, TILE, 3, 150, 140, 120, 255);
            fill_rect((int)x + 4, (int)y + 10, 3, 14, 40, 36, 34, 255);
            draw_ellipse(&A.disc, x + 10, y + 5, 2, 2, 200, 180, 140, 255); draw_ellipse(&A.disc, x + 38, y + 5, 2, 2, 200, 180, 140, 255);
        } else if (c == '~' && level_no == 1) {   /* hot cheese */
            fill_rect((int)x, (int)y, TILE, TILE, 200, 120, 30, 255);
            if (ty == 13) { float w = sinf(frame_no * 0.06f + tx * 0.9f) * 4;
                draw_ellipse(&A.disc, x + 24, y + 4 + w, 30, 10, 245, 185, 60, 255);
                draw_ellipse(&A.disc, x + 16, y + 2 + w, 7, 3, 255, 240, 170, 200);
                if ((frame_no + tx * 13) % 50 < 12) draw_ellipse(&A.disc, x + 30, y - 2 + w, 5 + (frame_no % 12) * 0.4f, 5, 255, 210, 90, 255); }
        } else if (c == '~') {
            fill_rect((int)x, (int)y, TILE, TILE, 8, 6, 8, 255);
            if (ty == 13) { float w = sinf(frame_no * 0.06f + tx * 0.9f) * 4;
                draw_ellipse(&A.disc, x + 24, y + 4 + w, 30, 10, 14, 10, 14, 255);
                draw_ellipse(&A.disc, x + 16, y + 2 + w, 6, 2, 90, 80, 90, 120); }
        } else if (c == 'O') {   /* brick oven */
            fill_rect((int)x, (int)y, TILE, TILE, 120, 52, 34, 255);
            for (int r = 0; r < 3; r++) fill_rect((int)x, (int)y + r * 16 + 15, TILE, 2, 60, 26, 16, 255);
            fill_rect((int)x + ((tx + ty) % 2) * 24, (int)y, 2, TILE, 60, 26, 16, 255);
            if (map[ty - 1][tx] != 'O') { fill_rect((int)x, (int)y, TILE, 6, 160, 80, 50, 255);
                int left = map[ty][tx - 1] != 'O';
                if (left) { float ph = oven_phase(tx); int on = oven_burning(tx);
                    fill_rect((int)x + 10, (int)y + 20, 76, 30, 20, 8, 4, 255);
                    draw_glow(&A.disc, x + 48, y + 34, on ? 60 : (ph > 2.9f ? 40 : 20), 255, 130, 30, on ? 255 : 160);
                    if (on) {   /* the fire column */
                        float k = fminf(1, ph / 0.15f) * fminf(1, (1.1f - ph) / 0.2f), hgt = 170 * k;
                        draw_ellipse(&A.disc, x + 48, y - hgt * 0.5f, 34, hgt * 0.55f, 255, 120, 30, 220);
                        draw_ellipse(&A.disc, x + 48, y - hgt * 0.45f, 20, hgt * 0.45f, 255, 220, 120, 255);
                        draw_glow(&A.disc, x + 48, y - hgt * 0.5f, 80, 255, 140, 40, 200);
                    } else if (ph > 2.9f && (frame_no / 4) % 2) draw_text_col(&A.small, "!", x + 48, y - 40, 0.7f, 1, 255, 120, 40, 255);
                    if (lit[zone_of(tx * TILE)] > 0) draw_text_col(&A.small, "OFF", x + 48, y - 24, 0.4f, 1, 160, 220, 255, 255);
                } }
        } else if (c == '>' || c == '<') {   /* conveyor belt */
            fill_rect((int)x, (int)y, TILE, 20, 50, 46, 44, 255);
            fill_rect((int)x, (int)y + 20, TILE, TILE - 20, 30, 26, 24, 255);
            int dir = c == '>' ? 1 : -1, off = ((frame_no * 3 * dir) % 16 + 16) % 16;
            for (int k = -1; k < 4; k++) fill_rect((int)x + k * 16 + off, (int)y + 3, 8, 3, 150, 140, 120, 255);
            draw_ellipse(&A.disc, x + 12, y + 30, 8, 8, 90, 84, 80, 255); draw_ellipse(&A.disc, x + 36, y + 30, 8, 8, 90, 84, 80, 255);
        } else if (c == 'B') {
            int used = 0; for (int i = 0; i < nbreak; i++) if (breakers[i].tx == tx) used = breakers[i].used;
            fill_rect((int)x + 6, (int)y + 2, 36, 50, 30, 28, 26, 255); fill_rect((int)x + 9, (int)y + 5, 30, 44, 110, 104, 92, 255);
            fill_rect((int)x + 20, (int)y + (used ? 10 : 28), 8, 14, 40, 30, 20, 255);
            Uint8 on = used ? 1 : (frame_no / 20) % 2;
            draw_ellipse(&A.disc, x + 24, y + 44, 4, 4, used ? 80 : 255, used ? 255 : 60, 60, 255);
            if (!used && on) draw_glow(&A.disc, x + 24, y + 44, 18, 255, 60, 40, 200);
            if (!used) draw_text_col(&A.small, "UP", x + 24, y - 22, 0.45f, 1, 255, 220, 160, 200);
        } else if (c == 'C' && ty == 11) {
            fill_rect((int)x + 2, (int)y - 6, 44, 102, 30, 18, 12, 255); fill_rect((int)x + 6, (int)y - 2, 36, 94, 110, 66, 38, 255);
            fill_rect((int)x + 10, (int)y + 2, 28, 40, 92, 54, 30, 255); fill_rect((int)x + 10, (int)y + 48, 28, 38, 92, 54, 30, 255);
            draw_ellipse(&A.disc, x + 36, y + 46, 3, 3, 220, 190, 90, 255);
            draw_text_col(&A.small, "PROPS", x + 24, y + 12, 0.32f, 1, 240, 220, 170, 255);
        } else if (c == 'K') {
            int on = 0; for (int i = 0; i <= cur_check; i++) if (checkpoints[i] == tx) on = 1;
            fill_rect((int)x + 21, (int)y - 50, 6, 98, 40, 34, 30, 255);
            draw_ellipse(&A.disc, x + 24, y - 56, 12, 14, on ? 255 : 90, on ? 236 : 80, on ? 170 : 70, 255);
            if (on) draw_glow(&A.disc, x + 24, y - 56, 60 + sinf(frame_no * 0.1f) * 4, 255, 210, 120, 180);
        }
    }
}

static void draw_sprite(Tex *t, float x, float feet, float scale, int face_right, Uint8 flash, Uint8 alpha, double angle, float sy) {
    if (!t->tex) return;
    float w = t->w * scale, h = t->h * scale * sy;
    Uint8 c = 255;
    draw_tex_tint(t, x - w / 2, feet - h, w, h, !face_right, c, c, c, alpha, angle);
    if (flash) { SDL_SetTextureBlendMode(t->tex, SDL_BLENDMODE_ADD); draw_tex_tint(t, x - w / 2, feet - h, w, h, !face_right, 255, 255, 255, flash, angle); SDL_SetTextureBlendMode(t->tex, SDL_BLENDMODE_BLEND); }
}

static void draw_enemy(Enemy *e, float ox, float oy) {
    float x = sx(e->x) + ox, y = e->y + oy;
    Uint8 fl = e->flash ? 200 : 0; Tex *et = enemy_tex(e);
    switch (e->type) {
    case E_USHER: {
        draw_sprite(et, x, y + 2, 1, e->face > 0, fl, 255, sinf(e->t * 14) * (fabsf(e->vx) > 5 ? 3 : 0), 1);
        if (e->state == 1) draw_glow(&A.disc, x + e->face * 44, y - 98, 40 + (0.5f - e->timer) * 40, 255, 240, 170, 230);
        break; }
    case E_DOG: {
        float sq = e->state == 1 ? 1.0f - 0.06f * sinf(e->t * 40) : 1;
        draw_sprite(et, x, y + 2, 1, e->face > 0, fl, 255, 0, sq);
        float lx = x + e->face * 50, ly = y - 76;
        if (e->state == 1) draw_glow(&A.disc, lx, ly, 20 + (0.9f - e->timer) * 50, 255, 250, 200, 255);
        if (e->state == 2) {
            float x1 = lx + e->face * 700;
            draw_bar(lx, ly, x1, ly, 34 + sinf(frame_no) * 4, 255, 236, 170, 120);
            draw_bar(lx, ly, x1, ly, 12, 255, 255, 240, 230);
            draw_glow(&A.disc, lx, ly, 50, 255, 250, 220, 255);
        }
        break; }
    case E_BLOB: case E_MINIBLOB: {
        float s = e->type == E_MINIBLOB ? 0.6f : 1.0f;
        float squash = e->onground ? 1.0f + 0.05f * sinf(e->t * 8) : 1.0f;
        draw_sprite(et, x, y + 2, s, e->face > 0, fl, 255, 0, squash);
        break; }
    case E_CRATE: case E_HATCRATE: {
        float w = 52, h = 46, bx = x - w / 2, by = y - h;
        fill_rect((int)bx, (int)by, (int)w, (int)h, 30, 18, 10, 255);
        fill_rect((int)bx + 3, (int)by + 3, (int)w - 6, (int)h - 6, e->type == E_HATCRATE ? 200 : 168, e->type == E_HATCRATE ? 160 : 112, e->type == E_HATCRATE ? 40 : 60, 255);
        draw_bar(bx + 4, by + 4, bx + w - 4, by + h - 4, 4, 90, 56, 28, 255);
        draw_bar(bx + w - 4, by + 4, bx + 4, by + h - 4, 4, 90, 56, 28, 255);
        draw_text_col(&A.small, e->type == E_HATCRATE ? "GEAR" : "TOOLS", x, by - 22, 0.36f, 1, 255, 230, 170, 220);
        if (e->flash) fill_rect((int)bx, (int)by, (int)w, (int)h, 255, 255, 255, 120);
        break; }
    }
    if (e->marked > 0) {   /* pinged by the security office */
        float r = 50 + sinf(frame_no * 0.2f) * 6, cy = y - 50;
        draw_bar(x - r, cy, x - r + 18, cy, 4, 255, 50, 40, 230); draw_bar(x + r - 18, cy, x + r, cy, 4, 255, 50, 40, 230);
        draw_bar(x, cy - r, x, cy - r + 18, 4, 255, 50, 40, 230); draw_bar(x, cy + r - 18, x, cy + r, 4, 255, 50, 40, 230);
    }
    if (e->burn > 0 && et && et->tex) {   /* on fire: flickering orange glow over the body */
        float w = et->w, h = et->h * (e->type == E_MINIBLOB ? 0.6f : 1.0f);
        draw_glow(&A.disc, x, y - h * 0.5f, h * 0.7f, 255, 120, 30, (Uint8)(120 + 60 * sinf(frame_no * 0.7f)));
        (void)w;
    }
    if (e->stun > 0) for (int k = 0; k < 3; k++) { float a = frame_no * 0.15f + k * 2.1f; draw_ellipse(&A.disc, x + cosf(a) * 26, y - 140 + sinf(a) * 8, 5, 5, 255, 240, 140, 255); }
}

static void draw_player(Player *p, float ox, float oy) {
    if (!p->active) return;
    if (p->hidden) {
        float x = sx(p->closet_tx * TILE + 24) + ox;
        draw_ellipse(&A.disc, x - 6, 11 * TILE + 26 + oy, 3, 2, 255, 255, 255, 230); draw_ellipse(&A.disc, x + 6, 11 * TILE + 26 + oy, 3, 2, 255, 255, 255, 230);
        return;
    }
    if (p->inv > 0 && p->alive && ((int)(p->inv * 20)) % 2) return;
    int set = p->gear >= 3 ? 0 : p->gear == 2 ? 1 : 2;
    int fr = 0; float sy = 1;
    if (!p->alive) fr = 5;
    else if (!p->onground) fr = p->aim_dy < 0 ? (p->aim_dx ? 6 : 7) : 5;
    else if (p->aim_dy < 0) fr = p->aim_dx ? 6 : 7;
    else if (fabsf(p->vx) > 20) fr = 1 + ((int)(p->anim * 8)) % 4;
    else fr = 0;
    if (p->crouch) sy = 0.68f;
    else if (p->onground && fabsf(p->vx) < 20) sy = 1.0f + 0.015f * sinf(p->bob * 4);
    double ang = p->alive ? 0 : p->dead_t * 600;
    float x = sx(p->x) + ox, y = p->y + oy + 4;
    draw_ellipse(&A.disc, x, FLOOR_Y + oy - 2 > y ? y - 2 : y - 2, 34, 7, 0, 0, 0, p->onground ? 90 : 40);
    draw_sprite(&A.hero[set][fr], x, y, 1, p->face > 0, 0, 255, ang, sy);
    if (p->shoot_t > 0.08f) {
        float mx = x + p->face * 52, my = y - 82;
        if (p->aim_dy < 0 && p->aim_dx) { mx = x + p->face * 50; my = y - 130; } else if (p->aim_dy < 0) { mx = x + p->face * 18; my = y - 160; }
        else if (p->crouch) my = y - 50;
        draw_glow(&A.disc, mx, my, 22, 255, 220, 140, 255);
    }
    if (p == &pl[1]) draw_text_col(&A.small, "P2", x, y - 178, 0.4f, 1, 120, 200, 255, 255);
}

static void draw_proj(float ox, float oy) {
    for (int i = 0; i < MAXP; i++) { Proj *p = &pr[i]; if (!p->alive) continue;
        float x = sx(p->x) + ox, y = p->y + oy;
        switch (p->type) {
        case P_NAIL: { float a = atan2f(p->vy, p->vx); float ex = x - cosf(a) * 22, ey = y - sinf(a) * 22;
            draw_bar(ex, ey, x, y, 4, 200, 200, 210, 255); draw_bar(ex - cosf(a) * 14, ey - sinf(a) * 14, ex, ey, 2, 255, 230, 160, 160);
            draw_ellipse(&A.disc, ex, ey, 4, 4, 160, 160, 170, 255); break; }
        case P_RIVET: draw_glow(&A.disc, x, y, 22, 255, 140, 40, 220); draw_ellipse(&A.disc, x, y, 7, 7, 255, 210, 120, 255); break;
        case P_FLAME: { float k = p->life / 0.24f; draw_glow(&A.disc, x, y, 34 * (1.4f - k), 120, 180, 255, (Uint8)(200 * k)); draw_ellipse(&A.disc, x, y, 10 * k + 4, 10 * k + 4, 230, 245, 255, (Uint8)(255 * k)); break; }
        case P_PLUNGER: draw_bar(x - 14, y, x + 6, y, 5, 140, 90, 50, 255); draw_ellipse(&A.disc, x + 10, y, 9, 12, 200, 40, 40, 255); break;
        case P_LIGHT: draw_glow(&A.disc, x, y, 36, 255, 240, 170, 255); draw_ellipse(&A.disc, x, y, 9, 9, 255, 255, 230, 255); break;
        case P_NOTE: draw_ellipse(&A.disc, x, y, 16, 13, 10, 8, 10, 255); draw_bar(x + 13, y, x + 13, y - 44, 5, 10, 8, 10, 255); draw_bar(x + 13, y - 44, x + 30, y - 32, 6, 10, 8, 10, 255);
            draw_ellipse(&A.disc, x - 5, y - 4, 4, 3, 255, 255, 255, 200); break;
        case P_TICKET: draw_tex_tint(&A.disc, x - 18, y - 9, 36, 18, 0, 0, 0, 0, 0, 0);
            fill_rect((int)x - 18, (int)y - 9, 36, 18, 220, 60, 50, 255); fill_rect((int)x - 14, (int)y - 5, 28, 10, 250, 220, 160, 255);
            fill_rect((int)x - 18, FLOOR_Y + (int)oy - 4, 36, 4, 0, 0, 0, 90); break;
        case P_CONFETTI: { Uint8 r = (Uint8)(120 + (i * 37) % 136), g = (Uint8)(80 + (i * 71) % 176), b = (Uint8)(80 + (i * 113) % 176);
            draw_ellipse(&A.disc, x, y, 12, 12, 30, 20, 20, 255); draw_ellipse(&A.disc, x, y, 9, 9, r, g, b, 255);
            draw_bar(x, y - 9, x + 6, y - 18, 3, 230, 210, 160, 255); if (frame_no % 4 < 2) draw_glow(&A.disc, x + 6, y - 18, 10, 255, 200, 80, 255); break; }
        case P_PIZZA: { float a = frame_no * 0.4f * (p->vx > 0 ? 1 : -1);
            draw_ellipse(&A.disc, x, y, 28, 28, 120, 60, 20, 255); draw_ellipse(&A.disc, x, y, 23, 23, 235, 180, 70, 255);
            for (int k = 0; k < 5; k++) { float b = a + k * 1.256f; draw_ellipse(&A.disc, x + cosf(b) * 13, y + sinf(b) * 13, 5, 5, 180, 30, 20, 255); }
            draw_bar(x + cosf(a) * 22, y + sinf(a) * 22, x - cosf(a) * 22, y - sinf(a) * 22, 2, 150, 90, 40, 200); break; }
        case P_DRIP: draw_ellipse(&A.disc, x, y, 7, 11, 10, 8, 10, 255); draw_ellipse(&A.disc, x - 2, y - 3, 2, 3, 200, 200, 200, 160); break;
        }
    }
    for (int i = 0; i < 8; i++) { Pickup *k = &pick[i]; if (!k->alive) continue;
        if (k->t > 11 && ((int)(k->t * 10)) % 2) continue;
        float x = sx(k->x) + ox, y = k->y + oy - 26 + sinf(k->t * 5) * 4;
        draw_glow(&A.disc, x, y, 40, 255, 220, 120, 160);
        fill_rect((int)x - 22, (int)y - 22, 44, 44, 30, 20, 14, 255); fill_rect((int)x - 19, (int)y - 19, 38, 38, 240, 220, 170, 255);
        if (k->kind == 10) { draw_ellipse(&A.disc, x, y + 2, 15, 12, 230, 180, 50, 255); fill_rect((int)x - 18, (int)y + 4, 36, 5, 200, 150, 40, 255); }
        else { int w = ((int)(k->t / 0.6f)) % NWEAP; const char *L[4] = {"N", "S", "T", "P"};
            draw_text_col(&A.font, L[w], x, y - 20, 0.8f, 1, 120, 40, 30, 255); }
    }
}

static void draw_splats(float ox, float oy) {
    for (int i = 0; i < MAXSPLAT; i++) { Splat *d = &splats[i]; if (d->a == 0) continue;
        float x = sx(d->x) + ox, y = d->y + oy; if (x < -120 || x > SCREEN_W + 120) continue;
        if (d->tex >= 0) draw_tex_srcx(&A.gore[d->tex], NULL, x - d->rx, y - d->ry, d->rx * 2, d->ry * 2, i & 1, d->ang, d->r > 60 ? 255 : 90, d->r > 60 ? 255 : 80, d->r > 60 ? 255 : 90, d->a);
        else draw_ellipse(&A.disc, x, y, d->rx, d->ry, d->r, d->g, d->b, d->a);
    }
}
static void draw_gibs(float ox, float oy) {
    for (int i = 0; i < MAXGIB; i++) { Gib *g = &gibs[i]; if (g->life <= 0) continue;
        Uint8 a = (Uint8)(255 * fminf(1, g->life));
        draw_tex_srcx(g->t, &g->src, sx(g->x) + ox - g->w / 2, g->y + oy - g->h / 2, g->w, g->h, g->flip, g->ang, 255, 255, 255, a);
    }
}
static void draw_parts(float ox, float oy) {
    for (int i = 0; i < 16; i++) { Ring *r = &rings[i]; if (r->life <= 0) continue;
        float k = r->life / 0.45f, x = sx(r->x) + ox, y = r->y + oy;
        for (int a = 0; a < 28; a++) { float t = a * 6.2832f / 28; draw_ellipse(&A.disc, x + cosf(t) * r->r, y + sinf(t) * r->r * 0.45f, 6 * k + 2, 6 * k + 2, r->cr, r->cg, r->cb, (Uint8)(220 * k)); }
    }
    for (int i = 0; i < MAXPART; i++) { Part *q = &parts[i]; if (q->life <= 0) continue;
        float k = q->life / q->max, s = q->size * (0.5f + 0.5f * k);
        if (q->kind == PK_SMOKE) { draw_ellipse(&A.disc, sx(q->x) + ox, q->y + oy, q->size, q->size, q->r, q->g, q->b, (Uint8)(110 * k)); continue; }
        if (q->kind == PK_CASING) { float a = q->ang * 0.01745f; draw_bar(sx(q->x) + ox - cosf(a) * 5, q->y + oy - sinf(a) * 5, sx(q->x) + ox + cosf(a) * 5, q->y + oy + sinf(a) * 5, 4, q->r, q->g, q->b, (Uint8)(255 * fminf(1, k * 3))); continue; }
        if (q->kind == PK_DEBRIS) { float a = q->ang * 0.01745f, L = q->size; draw_bar(sx(q->x) + ox - cosf(a) * L, q->y + oy - sinf(a) * L, sx(q->x) + ox + cosf(a) * L, q->y + oy + sinf(a) * L, L * 0.5f, q->r, q->g, q->b, (Uint8)(255 * fminf(1, k * 3))); continue; }
        if (q->add) draw_glow(&A.disc, sx(q->x) + ox, q->y + oy, s * 2.5f, q->r, q->g, q->b, (Uint8)(255 * k));
        else draw_ellipse(&A.disc, sx(q->x) + ox, q->y + oy, s, s, q->r, q->g, q->b, (Uint8)(255 * fminf(1, k * 2))); }
}

static void draw_fiend(float ox, float oy) {
    if (fiend.state < 2) return;
    float x = sx(fiend.x) + ox, y = fiend.y + oy;
    Tex *t = &A.blob[(frame_no / 10) & 1];
    float h = 300 + sinf(frame_no * 0.12f) * 14;
    draw_glow(&A.disc, x, y - h * 0.5f, 170, 90, 0, 0, 140);
    if (t->tex) { float w = t->w * 2.7f; draw_tex_tint(t, x - w / 2, y - h, w, h, fiend.x > (pl[0].x), 255, 255, 255, 255, sinf(frame_no * 0.08f) * 3); }
    for (int k = 0; k < 5; k++) draw_ellipse(&A.disc, x - 90 + k * 45, y - 6, 26, 10, 8, 6, 8, 255);
}

static void draw_boss_rat(float ox, float oy);
static void draw_boss(float ox, float oy) {
    if (!boss.active) return;
    if (level_no == 1) { draw_boss_rat(ox, oy); return; }
    float x = sx(boss.x) + ox, y = boss.y + oy + boss.bob;
    if (boss.state == BS_SLAM_AIM) {   /* ink shadow where the tentacle will land */
        float k = 1 - boss.timer / 0.85f; float tx = sx(boss.targetx) + ox;
        draw_ellipse(&A.disc, tx, FLOOR_Y + oy, 30 + 30 * k, 8 + 6 * k, 10, 6, 10, (Uint8)(120 + 120 * k));
        if ((frame_no / 4) % 2) draw_text_col(&A.small, "!", tx, FLOOR_Y + oy - 60, 0.8f, 1, 255, 60, 40, 255);
    }
    if (boss.state == BS_SLAM_HIT) {   /* giant gloved tentacle crashes down */
        float tx = sx(boss.targetx) + ox;
        draw_bar(tx, -20, tx, FLOOR_Y + oy - 30, 70, 120, 100, 120, 255);
        draw_bar(tx - 22, -20, tx - 22, FLOOR_Y + oy - 30, 10, 80, 64, 86, 255);
        for (int k = 0; k < 6; k++) draw_ellipse(&A.disc, tx + 18, 60 + k * 90 + oy, 10, 10, 180, 150, 170, 255);
        draw_ellipse(&A.disc, tx, FLOOR_Y + oy - 34, 50, 40, 250, 248, 236, 255);
        draw_ellipse(&A.disc, tx, FLOOR_Y + oy - 34, 50, 40, 20, 20, 20, 0);
    }
    if (boss.state == BS_SWEEP_WARN && (frame_no / 5) % 2) draw_glow(&A.disc, x - 140, y - 110, 70, 255, 240, 170, 255);
    if (boss.state == BS_SWEEP) {
        float bx = sx(boss.sweepx) + ox;
        draw_bar(x - 140, y - 110, bx, FLOOR_Y + oy - 26, 50, 255, 240, 170, 110);
        draw_ellipse(&A.disc, bx, FLOOR_Y + oy - 26, 50, 26, 255, 250, 210, 220);
        draw_glow(&A.disc, bx, FLOOR_Y + oy - 26, 90, 255, 230, 150, 255);
    }
    if (boss.pose < 0) return;
    Tex *t = &A.inky[boss.pose];
    double ang = boss.state == BS_DEAD ? sinf(frame_no * 0.5f) * 6 : sinf(boss.t * 1.3f) * 2;
    draw_sprite(t, x, y + 10, 1, 0, boss.flash ? 170 : 0, 255, ang, 1.0f + 0.02f * sinf(boss.t * 4));
}

static void draw_hud(void) {
    /* player panels */
    for (int i = 0; i < 2; i++) {
        Player *p = &pl[i]; if (!p->active && i == 1) {
            if (scene == SC_PLAY) draw_text_col(&A.small, "P2: PRESS + TO CLOCK IN", SCREEN_W - 24 - text_width(&A.small, "P2: PRESS + TO CLOCK IN", 0.5f), 22, 0.5f, 0, 230, 210, 170, 160);
            continue; }
        float x = i == 0 ? 20 : SCREEN_W - 300, y = 16;
        fill_rect((int)x, (int)y, 280, 74, 20, 12, 8, 190); fill_rect((int)x, (int)y, 280, 3, 200, 160, 90, 255);
        draw_text_col(&A.small, i == 0 ? "P1" : "P2", x + 12, y + 8, 0.55f, 0, 255, 220, 150, 255);
        const char *gl[3] = {"BOXERS", "OVERALLS", "FULL GEAR"};
        for (int g = 0; g < 3; g++) { int on = p->gear > g; float gx = x + 58 + g * 30;
            draw_ellipse(&A.disc, gx, y + 22, 11, 11, on ? (g == 2 ? 235 : g == 1 ? 80 : 245) : 60, on ? (g == 2 ? 185 : g == 1 ? 120 : 245) : 50, on ? (g == 2 ? 50 : g == 1 ? 190 : 245) : 46, 255); }
        draw_text_col(&A.small, gl[p->gear > 0 ? p->gear - 1 : 0], x + 150, y + 10, 0.45f, 0, 240, 230, 210, 255);
        char b[48]; snprintf(b, sizeof b, "x%d  %s", p->lives, WEAP_NAME[p->weapon]);
        draw_text_col(&A.small, b, x + 12, y + 42, 0.48f, 0, 255, 240, 200, 255);
    }
    /* building power */
    float px = SCREEN_W / 2 - 150, py = 18;
    fill_rect((int)px - 6, (int)py - 4, 312, 44, 20, 12, 8, 200);
    draw_text_col(&A.small, "POWER", px, py, 0.5f, 0, 255, 220, 150, 255);
    fill_rect((int)px + 108, (int)py + 6, 190, 16, 40, 30, 24, 255);
    Uint8 r = power < 25 ? 230 : 240, g = power < 25 ? 60 : 200;
    if (!(power < 25 && (frame_no / 15) % 2)) fill_rect((int)px + 108, (int)py + 6, (int)(power * 1.9f), 16, r, g, 70, 255);
    char b[32]; snprintf(b, sizeof b, "%d%%", (int)ceilf(power)); draw_text_col(&A.small, b, px + 203, py + 22, 0.4f, 1, 255, 240, 210, 255);
    snprintf(b, sizeof b, "SCORE %06d", score); draw_text_col(&A.small, b, SCREEN_W / 2, py + 44, 0.45f, 1, 240, 220, 180, 220);
    /* boss bar */
    if (boss.active && boss.state != BS_DEAD) {
        float bx = SCREEN_W / 2 - 300, by = SCREEN_H - 50;
        fill_rect((int)bx - 4, (int)by - 26, 608, 48, 20, 10, 8, 210);
        draw_text_col(&A.small, LEVELS[level_no].boss_name, bx, by - 24, 0.5f, 0, 255, 200, 150, 255);
        fill_rect((int)bx, (int)by + 2, 600, 14, 50, 20, 20, 255);
        fill_rect((int)bx, (int)by + 2, (int)(600.0f * boss.hp / boss.maxhp), 14, 200, 40, 40, 255);
    }
    if (boss.state == BS_INTRO && boss.active) {
        float k = clampf(boss.timer, 0, 1);
        fill_rect(0, 250, SCREEN_W, 150, 10, 6, 4, (Uint8)(200 * k));
        draw_text_col(&A.font, LEVELS[level_no].boss_name, SCREEN_W / 2, 268, 1.6f, 1, 255, 220, 150, (Uint8)(255 * k));
        draw_text_col(&A.small, LEVELS[level_no].boss_line, SCREEN_W / 2, 352, 0.7f, 1, 230, 200, 170, (Uint8)(255 * k));
    }
    if (msg_t > 0) {
        Uint8 a = (Uint8)(255 * clampf(msg_t * 2, 0, 1));
        draw_text_col(&A.font, msg, SCREEN_W / 2, 150, 0.8f, 1, 255, 236, 190, a);
    }
    if (warn_t > 0 || fiend.state == 2) {
        if ((frame_no / 8) % 2) draw_text_col(&A.font, fiend.state == 2 ? "IT'S HERE! HIDE IN A PROP CLOSET!" : "IT'S COMING... HIDE!", SCREEN_W / 2, 200, 0.85f, 1, 255, 80, 60, 255);
    }
}

static void draw_post(void) {
    /* sepia grade, grain, vignette: old film */
    SDL_SetRenderDrawBlendMode(R, SDL_BLENDMODE_MOD);
    SDL_SetRenderDrawColor(R, 255, 236, 205, 255); SDL_RenderFillRect(R, NULL);
    SDL_SetRenderDrawBlendMode(R, SDL_BLENDMODE_BLEND);
    Tex *g = &A.grain[(frame_no / 3) % 4];
    if (g->tex) { SDL_SetTextureBlendMode(g->tex, SDL_BLENDMODE_BLEND); draw_tex(g, 0, 0, SCREEN_W, SCREEN_H, 0, 60); }
    draw_tex(&A.vignette, 0, 0, SCREEN_W, SCREEN_H, 0, 255);
    if ((frame_no % 97) < 2) fill_rect((int)(frand() * SCREEN_W), 0, 2, SCREEN_H, 255, 240, 220, 50);   /* film scratch */
}

static void draw_dark(void) {
    if (!darkness_on) return;
    float x = 0; int n = 0;
    for (int i = 0; i < 2; i++) if (pl[i].active && pl[i].alive && !pl[i].hidden) { x += sx(pl[i].x); n++; }
    if (!n) { fill_rect(0, 0, SCREEN_W, SCREEN_H, 4, 2, 6, 245); return; }
    x /= n; float y = pl[0].y - 70, r = 330 + sinf(frame_no * 0.3f) * 6;
    draw_tex(&A.mask, x - r, y - r, r * 2, r * 2, 0, 255);
    fill_rect(0, 0, (int)(x - r) + 1, SCREEN_H, 4, 2, 6, 248); fill_rect((int)(x + r) - 1, 0, SCREEN_W, SCREEN_H, 4, 2, 6, 248);
    fill_rect((int)(x - r), 0, (int)(r * 2), (int)(y - r) + 1, 4, 2, 6, 248); fill_rect((int)(x - r), (int)(y + r) - 1, (int)(r * 2), SCREEN_H, 4, 2, 6, 248);
    /* enemy eyes glow in the dark */
    for (int i = 0; i < nen; i++) { Enemy *e = &en[i]; if (!e->alive || e->type >= E_CRATE) continue;
        float ex = sx(e->x), ey = e->y - (e->type == E_USHER ? 122 : e->type == E_DOG ? 78 : 36);
        if (ex < -50 || ex > SCREEN_W + 50) continue;
        draw_glow(&A.disc, ex - 7, ey, 12, 255, 170, 40, 255); draw_glow(&A.disc, ex + 7, ey, 12, 255, 170, 40, 255); }
}

static void draw_bg(float ox) {
    Tex *bgt = (level_no == 1 && A.bg2.tex) ? &A.bg2 : &A.bg;
    if (bgt->tex) {
        float par = camx * 0.35f; int w = bgt->w;
        float off = -fmodf(par, (float)w);
        for (float x = off; x < SCREEN_W; x += w) draw_tex(bgt, x + ox * 0.3f, 0, (float)w, SCREEN_H, 0, 255);
    } else fill_rect(0, 0, SCREEN_W, SCREEN_H, 60, 40, 30, 255);
    Uint8 dim = (Uint8)(110 - power * 0.6f);   /* the theatre dims as the power runs down */
    fill_rect(0, 0, SCREEN_W, SCREEN_H, 16, 8, 6, (Uint8)(70 + dim));
    int cz = zone_of(camx + SCREEN_W / 2);
    if (lit[cz] > 0) fill_rect(0, 0, SCREEN_W, SCREEN_H, 255, 230, 170, (Uint8)(30 * fminf(1, lit[cz])));
}

static void draw_title(void) {
    float t = scene_t;
    if (A.bg.tex) draw_tex(&A.bg, -fmodf(t * 20, A.bg.w - SCREEN_W), 0, (float)A.bg.w, SCREEN_H, 0, 255);
    fill_rect(0, 0, SCREEN_W, SCREEN_H, 14, 6, 4, 150);
    if (A.inky[0].tex) { float s = 1.05f; Tex *k = &A.inky[0];
        draw_tex_tint(k, SCREEN_W - k->w * s - 30, SCREEN_H - k->h * s + 40 + sinf(t * 1.5f) * 8, k->w * s, k->h * s, 0, 160, 140, 130, 255, sinf(t) * 2); }
    /* marquee */
    float mx = 120, my = 70, mw = 720, mh = 230;
    fill_rect((int)mx, (int)my, (int)mw, (int)mh, 40, 14, 12, 255);
    fill_rect((int)mx + 12, (int)my + 12, (int)mw - 24, (int)mh - 24, 120, 26, 22, 255);
    fill_rect((int)mx + 22, (int)my + 22, (int)mw - 44, (int)mh - 44, 32, 14, 10, 255);
    for (int i = 0; i < 26; i++) {   /* chasing bulbs */
        int on = ((i + (int)(t * 8)) % 3) != 0; float bx, by;
        if (i < 9) { bx = mx + 30 + i * 82; by = my + 6; } else if (i < 18) { bx = mx + 30 + (i - 9) * 82; by = my + mh - 6; }
        else if (i < 22) { bx = mx + 6; by = my + 52 + (i - 18) * 42; } else { bx = mx + mw - 6; by = my + 52 + (i - 22) * 42; }
        draw_ellipse(&A.disc, bx, by, 8, 8, on ? 255 : 90, on ? 230 : 70, on ? 150 : 50, 255);
        if (on) draw_glow(&A.disc, bx, by, 26, 255, 200, 110, 150);
    }
    draw_text_col(&A.font, "MIDNIGHT", mx + mw / 2, my + 34, 1.9f, 1, 255, 236, 180, 255);
    draw_text_col(&A.font, "MATINEE", mx + mw / 2, my + 112, 1.9f, 1, 255, 210, 120, 255);
    { char b[96]; snprintf(b, sizeof b, "<  SHOW %d: %s  >", level_no + 1, LEVELS[level_no].title);
      draw_text_col(&A.small, b, mx + mw / 2, my + mh + 16, 0.66f, 1, 255, 220, 150, 255); }
    const char *items[3] = {"SOLO  (GAMEPAD RUNS THE CAMERAS TOO)", "SECURITY OFFICE CO-OP", "POSSESSED PARTY MODE  (SOON)"};
    for (int i = 0; i < 3; i++) {
        float y = 420 + i * 62; int s = menu_sel == i;
        if (s) fill_rect(150, (int)y - 6, 660, 50, 120, 26, 22, 200);
        draw_text_col(&A.font, items[i], 480, y, 0.62f, 1, s ? 255 : 190, s ? 236 : 170, s ? 180 : 140, i == 2 ? 150 : 255);
    }
    draw_text_col(&A.small, "UP/DOWN: MODE   LEFT/RIGHT: SHOW   A / +: START", 480, 620, 0.5f, 1, 220, 200, 170, (Uint8)(150 + 100 * sinf(t * 4)));
    if (msg_t > 0) draw_text_col(&A.small, msg, SCREEN_W / 2, 668, 0.55f, 1, 255, 120, 90, 255);
}

void game_draw(void) {
    SDL_SetRenderDrawColor(R, 0, 0, 0, 255); SDL_RenderClear(R);
    if (scene == SC_TITLE) { draw_title(); draw_post(); return; }
    float ox = shake > 0 ? (frand() - 0.5f) * shake * 2 : 0, oy = shake > 0 ? (frand() - 0.5f) * shake * 2 : 0;
    draw_bg(ox);
    draw_boss(ox, oy);
    draw_tiles(ox, oy);
    draw_splats(ox, oy);
    for (int i = 0; i < nen; i++) if (en[i].alive) { float x = sx(en[i].x); if (x > -200 && x < SCREEN_W + 200) draw_enemy(&en[i], ox, oy); }
    for (int i = 0; i < 2; i++) draw_player(&pl[i], ox, oy);
    draw_gibs(ox, oy);
    draw_fiend(ox, oy);
    draw_proj(ox, oy);
    draw_parts(ox, oy);
    draw_dark();
    if (warn_t > 0 || fiend.state == 2) {   /* ink creeps in from the edges */
        float k = fiend.state == 2 ? 1 : 1 - warn_t / 1.8f;
        for (int i = 0; i < 14; i++) { float yy = i * 55.0f, len = 60 + 50 * k + sinf(frame_no * 0.1f + i) * 20;
            draw_ellipse(&A.disc, 0, yy, len, 40, 8, 4, 6, 230); draw_ellipse(&A.disc, SCREEN_W, yy, len, 40, 8, 4, 6, 230); }
    }
    draw_post();
    draw_hud();
    if (scene == SC_OVER || scene == SC_CLEAR) {
        fill_rect(0, 0, SCREEN_W, SCREEN_H, 10, 4, 2, (Uint8)fminf(200, scene_t * 200));
        if (scene == SC_OVER) {
            draw_text_col(&A.font, "THE SHOW IS OVER", SCREEN_W / 2, 260, 1.4f, 1, 255, 80, 60, 255);
            draw_text_col(&A.small, "UNCLE INKY KEEPS HIS THEATRE... FOR NOW", SCREEN_W / 2, 360, 0.7f, 1, 240, 210, 170, 255);
        } else {
            { char b[64]; snprintf(b, sizeof b, "%s CLEARED!", LEVELS[level_no].title); draw_text_col(&A.font, b, SCREEN_W / 2, 230, 1.2f, 1, 255, 230, 150, 255); }
            char b[48]; snprintf(b, sizeof b, "SCORE %06d", score);
            draw_text_col(&A.font, b, SCREEN_W / 2, 320, 0.9f, 1, 255, 240, 210, 255);
            if (level_no + 1 < NUM_LEVELS) { char b[64]; snprintf(b, sizeof b, "NEXT: SHOW %d - %s", level_no + 2, LEVELS[level_no + 1].title); draw_text_col(&A.small, b, SCREEN_W / 2, 400, 0.6f, 1, 240, 210, 170, 255); }
            else draw_text_col(&A.small, "MORE SHOWS COMING SOON", SCREEN_W / 2, 400, 0.6f, 1, 240, 210, 170, 255);
        }
        if (scene_t > 2) draw_text_col(&A.small, "PRESS A", SCREEN_W / 2, 480, 0.6f, 1, 255, 240, 200, (Uint8)(150 + 100 * sinf(scene_t * 4)));
    }
}

/* ------------------------------------------------------------ drawing: GamePad security office */
#define PR(x, y, w, h, r, g, b, a) fill_rect((int)(x), (int)(y), (int)(w), (int)(h), r, g, b, a)

static void draw_feed(int z, float fx, float fy, float fw, float fh, int big) {
    float x0 = ZONE_X0[z] * TILE, x1 = ZONE_X0[z + 1] * TILE, zw = x1 - x0;
    float s = fminf(fw / zw, fh / (LH * TILE)); float oy = fy + (fh - LH * TILE * s) / 2, ox = fx + (fw - zw * s) / 2;
    int dark = power <= 0 && lit[z] <= 0;
    PR(fx, fy, fw, fh, 14, 16, 12, 255);
    if (!dark) {
        for (int ty = 0; ty < LH; ty++) for (int tx = ZONE_X0[z]; tx < ZONE_X0[z + 1]; tx++) {
            char c = map[ty][tx]; float x = ox + (tx * TILE - x0) * s, y = oy + ty * TILE * s, w = TILE * s + 1;
            if (c == '#') PR(x, y, w, w, 96, 102, 84, 255);
            else if (c == '=') PR(x, y, w, w * 0.25f + 1, 140, 146, 120, 255);
            else if (c == '~') PR(x, y, w, w, 4, 4, 4, 255);
            else if (c == 'C') PR(x, y, w, w, 70, 60, 44, 255);
            else if (c == 'B') PR(x + w * 0.2f, y, w * 0.6f, w, 160, 160, 140, 255);
            else if (c == 'O') PR(x, y, w, w, 150, 90, 70, 255);
            else if (c == '>' || c == '<') PR(x, y, w, w * 0.5f, 120, 120, 110, 255);
        }
        if (lit[z] > 0) PR(fx, fy, fw, fh, 255, 240, 180, 40);
    }
    /* things on camera */
    for (int i = 0; i < nen; i++) { Enemy *e = &en[i]; if (!e->alive || zone_of(e->x) != z || e->type >= E_CRATE) continue;
        float ex = ox + (e->x - x0) * s, ey = oy + e->y * s; float w, h; enemy_box(e, &w, &h);
        if (!dark) PR(ex - w * s / 2, ey - h * s, w * s, h * s, 30, 34, 28, 255);
        float eyy = ey - h * s * 0.8f; float er = big ? 2.5f : 1.5f;
        draw_ellipse(&padDisc, ex - er * 1.6f, eyy, er, er, 255, 180, 60, 255); draw_ellipse(&padDisc, ex + er * 1.6f, eyy, er, er, 255, 180, 60, 255);
        if (e->marked > 0) { PR(ex - 6, ey - h * s - 8, 12, 3, 255, 60, 40, 255); }
    }
    for (int i = 0; i < 2; i++) { Player *p = &pl[i]; if (!p->active || !p->alive || zone_of(p->x) != z) continue;
        float px = ox + (p->x - x0) * s, py = oy + p->y * s;
        if (p->hidden) continue;
        PR(px - 20 * s, py - 120 * s, 40 * s, 120 * s, dark ? 40 : 200, dark ? 40 : 210, dark ? 40 : 190, 255);
        draw_ellipse(&padDisc, px, py - 124 * s, 26 * s, 14 * s, 230, 190, 50, 255);
    }
    if (fiend.state >= 2 && zone_of(fiend.x) == z) {
        float px = ox + (fiend.x - x0) * s, py = oy + fiend.y * s;
        draw_ellipse(&padDisc, px, py - 150 * s, 70 * s, 150 * s, 0, 0, 0, 255);
        draw_ellipse(&padDisc, px - 18 * s, py - 200 * s, 10 * s + 1, 14 * s + 1, 255, 255, 255, 255);
        draw_ellipse(&padDisc, px + 18 * s, py - 200 * s, 10 * s + 1, 14 * s + 1, 255, 255, 255, 255);
    }
    if (boss.active && z == 5 && boss.state != BS_DEAD) {
        float px = ox + (boss.x - x0) * s, py = oy + boss.y * s;
        if (!dark) draw_ellipse(&padDisc, px, py - 200 * s, 110 * s, 200 * s, 70, 60, 70, 255);
        draw_ellipse(&padDisc, px - 22 * s, py - 300 * s, 9 * s + 1, 9 * s + 1, boss.hp < boss.maxhp / 2 ? 255 : 255, boss.hp < boss.maxhp / 2 ? 40 : 190, 40, 255);
        draw_ellipse(&padDisc, px + 22 * s, py - 300 * s, 9 * s + 1, 9 * s + 1, boss.hp < boss.maxhp / 2 ? 255 : 255, boss.hp < boss.maxhp / 2 ? 40 : 190, 40, 255);
    }
    /* CCTV look: scanlines, noise, a rolling bar */
    for (float y = fy; y < fy + fh; y += 3) PR(fx, y, fw, 1, 0, 0, 0, 70);
    float roll = fy + fmodf(frame_no * 1.3f + z * 37, fh);
    PR(fx, roll, fw, 6, 200, 220, 190, 18);
    if (dark) for (int i = 0; i < (big ? 160 : 40); i++) PR(fx + frand() * fw, fy + frand() * fh, 2, 2, 200, 200, 200, (Uint8)(40 + frand() * 80));
    SDL_SetRenderDrawBlendMode(R, SDL_BLENDMODE_MOD); SDL_SetRenderDrawColor(R, 190, 230, 200, 255);
    SDL_Rect rc = {(int)fx, (int)fy, (int)fw, (int)fh}; SDL_RenderFillRect(R, &rc); SDL_SetRenderDrawBlendMode(R, SDL_BLENDMODE_BLEND);
}

void game_draw_pad(void) {
    PR(0, 0, PAD_W, PAD_H, 26, 18, 12, 255);
    if (scene != SC_PLAY) {
        draw_text_col(&padFont, "SECURITY OFFICE", PAD_W / 2, 150, 1.1f, 1, 255, 220, 150, 255);
        draw_text_col(&padSmall, scene == SC_TITLE ? "CAMERAS OFFLINE UNTIL THE SHOW STARTS" : "SHIFT OVER", PAD_W / 2, 230, 0.55f, 1, 230, 200, 160, 255);
        if (scene == SC_TITLE) {
            draw_text_col(&padSmall, "SOLO: THIS GAMEPAD RUNS AND WATCHES THE CAMERAS", PAD_W / 2, 300, 0.42f, 1, 220, 190, 150, 255);
            draw_text_col(&padSmall, "CO-OP: RUNNER USES A WII REMOTE / PRO CONTROLLER", PAD_W / 2, 330, 0.42f, 1, 220, 190, 150, 255);
        }
        for (float y = 0; y < PAD_H; y += 3) PR(0, y, PAD_W, 1, 0, 0, 0, 50);
        return;
    }
    /* header */
    PR(0, 0, PAD_W, 44, 14, 8, 6, 255); PR(0, 44, PAD_W, 2, 200, 150, 80, 255);
    draw_text_col(&padSmall, "SECURITY OFFICE", 14, 10, 0.55f, 0, 255, 220, 150, 255);
    int mins = (int)(scene_t / 4) % 60, hrs = 12 + (int)(scene_t / 240); if (hrs > 12) hrs -= 12;
    char clk[32]; snprintf(clk, sizeof clk, "%d:%02d AM", hrs, mins);
    draw_text_col(&padSmall, clk, PAD_W - 14 - text_width(&padSmall, clk, 0.55f), 10, 0.55f, 0, 255, 236, 200, 255);
    if (mode == 0) draw_text_col(&padSmall, "SOLO", PAD_W / 2, 12, 0.45f, 1, 200, 180, 150, 200);
    else draw_text_col(&padSmall, "CO-OP OPERATOR", PAD_W / 2, 12, 0.45f, 1, 200, 180, 150, 200);

    /* the big feed */
    float fx = 16, fy = 58, fw = 528, fh = 330;
    PR(fx - 4, fy - 4, fw + 8, fh + 8, 70, 60, 50, 255);
    draw_feed(cam_sel, fx, fy, fw, fh, 1);
    char lab[48]; snprintf(lab, sizeof lab, "CAM %d  %s", cam_sel + 1, ZONE_NAME[cam_sel]);
    draw_text_col(&padSmall, lab, fx + 10, fy + 8, 0.5f, 0, 230, 255, 220, 255);
    if ((frame_no / 30) % 2) { draw_ellipse(&padDisc, fx + fw - 50, fy + 18, 6, 6, 255, 40, 40, 255); draw_text_col(&padSmall, "REC", fx + fw - 40, fy + 9, 0.42f, 0, 255, 120, 120, 255); }
    if (power <= 0 && lit[cam_sel] <= 0) draw_text_col(&padFont, "NO POWER", fx + fw / 2, fy + fh / 2 - 20, 0.9f, 1, 255, 80, 60, (frame_no / 20) % 2 ? 255 : 120);
    if (cam_flash > 0) PR(fx, fy, fw, fh, 255, 255, 255, (Uint8)(cam_flash * 500 > 200 ? 200 : cam_flash * 500));

    /* camera buttons */
    for (int i = 0; i < 6; i++) {
        SDL_Rect r = cam_btn(i); int sel = i == cam_sel;
        int ne = 0; for (int k = 0; k < nen; k++) if (en[k].alive && en[k].type < E_CRATE && zone_of(en[k].x) == i) ne++;
        int alert = (fiend.state >= 2 && zone_of(fiend.x) == i) || (fiend.state == 1 && i == 3) || (boss.active && i == 5 && boss.state != BS_DEAD);
        int here = 0; for (int k = 0; k < 2; k++) if (pl[k].active && zone_of(pl[k].x) == i) here = 1;
        PR(r.x, r.y, r.w, r.h, sel ? 220 : 60, sel ? 170 : 50, sel ? 90 : 40, 255);
        draw_feed(i, r.x + 3, r.y + 3, r.w - 6, r.h - 6, 0);
        if (alert && (frame_no / 10) % 2) { PR(r.x, r.y, r.w, 4, 255, 40, 30, 255); PR(r.x, r.y + r.h - 4, r.w, 4, 255, 40, 30, 255); PR(r.x, r.y, 4, r.h, 255, 40, 30, 255); PR(r.x + r.w - 4, r.y, 4, r.h, 255, 40, 30, 255); }
        char t[24]; snprintf(t, sizeof t, "CAM %d", i + 1);
        draw_text_col(&padSmall, t, r.x + 8, r.y + 6, 0.38f, 0, 230, 255, 220, 255);
        if (ne) { snprintf(t, sizeof t, "%d", ne); draw_text_col(&padSmall, t, r.x + r.w - 18, r.y + 6, 0.38f, 0, 255, 170, 60, 255); }
        if (here) draw_ellipse(&padDisc, r.x + r.w - 12, r.y + r.h - 12, 6, 6, 240, 200, 60, 255);
        if (lit[i] > 0) draw_ellipse(&padDisc, r.x + 12, r.y + r.h - 12, 6, 6, 255, 250, 210, 255);
    }
    /* actions */
    int can_ping = ping_cool <= 0 && power >= 6, can_light = power >= 10 && lit[cam_sel] <= 0;
    PR(BTN_PING.x, BTN_PING.y, BTN_PING.w, BTN_PING.h, can_ping ? 150 : 60, can_ping ? 40 : 40, 34, 255);
    PR(BTN_PING.x + 4, BTN_PING.y + 4, BTN_PING.w - 8, BTN_PING.h - 8, can_ping ? 200 : 70, can_ping ? 56 : 50, 44, 255);
    draw_text_col(&padFont, "PING", BTN_PING.x + BTN_PING.w / 2, BTN_PING.y + 6, 0.7f, 1, 255, 240, 220, 255);
    draw_text_col(&padSmall, "(-)  6%", BTN_PING.x + BTN_PING.w / 2, BTN_PING.y + 40, 0.36f, 1, 255, 220, 200, 255);
    PR(BTN_LIGHTS.x, BTN_LIGHTS.y, BTN_LIGHTS.w, BTN_LIGHTS.h, can_light ? 170 : 60, can_light ? 140 : 50, 40, 255);
    PR(BTN_LIGHTS.x + 4, BTN_LIGHTS.y + 4, BTN_LIGHTS.w - 8, BTN_LIGHTS.h - 8, can_light ? 230 : 70, can_light ? 190 : 60, 60, 255);
    draw_text_col(&padFont, "LIGHTS", BTN_LIGHTS.x + BTN_LIGHTS.w / 2, BTN_LIGHTS.y + 6, 0.7f, 1, 40, 24, 10, 255);
    draw_text_col(&padSmall, "(X)  10%", BTN_LIGHTS.x + BTN_LIGHTS.w / 2, BTN_LIGHTS.y + 40, 0.36f, 1, 60, 36, 16, 255);
    /* power gauge */
    float gx = 382, gy = 408, gw = 162;
    draw_text_col(&padSmall, "POWER", gx, gy - 2, 0.42f, 0, 255, 220, 150, 255);
    char pw[16]; snprintf(pw, sizeof pw, "%d%%", (int)ceilf(power)); draw_text_col(&padSmall, pw, gx + gw - text_width(&padSmall, pw, 0.42f), gy - 2, 0.42f, 0, 255, 240, 200, 255);
    PR(gx, gy + 26, gw, 28, 40, 30, 24, 255);
    for (int i = 0; i < 10; i++) if (power > i * 10) PR(gx + 3 + i * 16, gy + 29, 13, 22, power < 25 ? 230 : 120 + i * 12, power < 25 ? 60 : 200, 70, 255);
    draw_text_col(&padSmall, "ZL / ZR OR TAP: SWITCH CAMERA", 560 + 138, 312, 0.34f, 1, 210, 190, 160, 255);
    draw_text_col(&padSmall, "LIGHTS SCARE THE INK FIEND", 560 + 138, 336, 0.34f, 1, 210, 190, 160, 255);
    draw_text_col(&padSmall, "PING = DOUBLE DAMAGE", 560 + 138, 360, 0.34f, 1, 210, 190, 160, 255);
    if ((fiend.state == 1 || fiend.state == 2) && (frame_no / 10) % 2) {
        PR(560, 390, 280, 76, 120, 10, 10, 255);
        draw_text_col(&padFont, "FIEND ALERT", 700, 400, 0.6f, 1, 255, 230, 220, 255);
        draw_text_col(&padSmall, "CAM 4: HIT LIGHTS!", 700, 436, 0.38f, 1, 255, 220, 200, 255);
    }
    for (float y = 0; y < PAD_H; y += 4) PR(0, y, PAD_W, 1, 0, 0, 0, 30);
}
