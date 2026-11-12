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
typedef struct { Tex hero[3][HERO_FRAMES]; Tex usher[5], dog[5], blob[4], inky[3], gore[12], goat[5], meat[5], slice[5], rat[5], cleaver, gore2[12], bg2, bomb[5], ghost[5], glove[5], splicer[3], bg3, imp[5], worker[5], eel[5], press[3], bg4, suitcase[5], soldier[5], umbrella[5], collector[3], bg5, unbound[3], bg6; Tex bg, disc, vignette, mask, grain[4]; Font font, small; } Assets;
static Assets A;
static Font padFont, padSmall; static Tex padDisc;

/* ------------------------------------------------------------ level */
static char map[LH][LW + 1];
enum { E_USHER, E_DOG, E_BLOB, E_MINIBLOB, E_GOAT, E_MEATBALL, E_SLICE, E_BOMB, E_GHOST, E_GLOVE, E_IMP, E_WORKER, E_EEL, E_SUITCASE, E_SOLDIER, E_UMBRELLA, E_SHADOW, E_CRATE, E_HATCRATE };
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
    {"CARTOON REELS", "THE LOST CARTOON VAULT", "THE SPLICER", "SHE CUTS EVERY PICTURE SHORT", "mm_boss3",
     {"TITLE CARD", "BARNYARD SHORT", "SPOOK SHORT", "FILM VAULT", "CUTTING ROOM", "THE SPLICING TABLE"}, {0, 40, 85, 125, 170, 210, LW},
     {"mm_level3", "mm_level3", "mm_level3", "mm_vault", "mm_vault", "mm_level3"}},
    {"THE INK WORKS", "WHERE THE INK IS MADE", "THE BIG PRESS", "IT PRINTS NOTHING BUT TROUBLE", "mm_boss4",
     {"LOADING BAY", "PRESS FLOOR", "INK VATS", "PIPE WORKS", "BOILER FURNACE", "THE PRESS ROOM"}, {0, 40, 85, 125, 170, 210, LW},
     {"mm_level4", "mm_level4", "mm_level4", "mm_pipes", "mm_pipes", "mm_level4"}},
    {"LOST & FOUND", "EVERYTHING THE THEATRE EVER LOST", "THE COLLECTOR", "FINDERS KEEPERS", "mm_boss5",
     {"COAT CHECK", "LOST PROPERTY", "TOY CHEST", "LUGGAGE HALL", "DOLL HOSPITAL", "THE CLAIM DESK"}, {0, 40, 85, 125, 170, 210, LW},
     {"mm_level5", "mm_level5", "mm_level5", "mm_dolls", "mm_dolls", "mm_level5"}},
    {"ENCORE", "THE FINAL SHOW", "INKY UNBOUND", "THE SHOW MUST GO ON", "mm_boss6",
     {"BURNING LOBBY", "THE BALCONY", "ORCHESTRA PIT", "THE FLY LOFT", "CURTAIN CALL", "THE GRAND STAGE"}, {0, 40, 85, 125, 170, 210, LW},
     {"mm_level6", "mm_level6", "mm_level6", "mm_finale", "mm_finale", "mm_level6"}},
};
#define NUM_LEVELS 6
static int loop_no;   /* 0 = first run, 1 = the harder second run (beat it for the true ending) */
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
            type == E_GOAT ? 4 : type == E_MEATBALL ? 3 : type == E_SLICE ? 2 :
            type == E_BOMB ? 2 : type == E_GHOST ? 3 : type == E_GLOVE ? 3 :
            type == E_IMP ? 2 : type == E_WORKER ? 5 : type == E_EEL ? 3 :
            type == E_SUITCASE ? 4 : type == E_SOLDIER ? 3 : type == E_UMBRELLA ? 2 : type == E_SHADOW ? 4 : 1;
    if (loop_no && type < E_CRATE) e->hp += 1 + e->hp / 3;   /* second run: everything is tougher */
    e->timer = 1.0f + frand(); e->plat0 = 0; e->plat1 = LW * TILE;
    if (type == E_SLICE || type == E_UMBRELLA) { e->plat0 = e->y - 30; e->timer = frand() * 3; }   /* plat0 = home altitude */
    if (type == E_GHOST) e->land = frand() * 4;   /* phase offset for fading */
    if (type == E_EEL) { e->y = FLOOR_Y; e->timer = 0.5f + frand(); }
}
static void breaker(int tx) { if (nbreak < 8) breakers[nbreak++] = (Breaker){tx, 0}; put(tx, 11, 'B'); }
static void checkpoint(int tx) { if (ncheck < 12) checkpoints[ncheck++] = tx; put(tx, 12, 'K'); }
static void closet(int tx) { put(tx, 11, 'C'); put(tx, 12, 'C'); }
static void drip(int tx) { if (ndrip < 8) drips[ndrip++] = (Drip){tx, frand() * 2}; }

static void build_level1(void);
static void build_level2(void);
static void build_level3(void);
static void build_level4(void);
static void build_level5(void);
static void build_level6(void);
static void build_level(void) {
    ZONE_NAME = LEVELS[level_no].zone; ZONE_X0 = LEVELS[level_no].x0;
    if (level_no == 5) build_level6(); else if (level_no == 4) build_level5(); else if (level_no == 3) build_level4(); else if (level_no == 2) build_level3(); else if (level_no == 1) build_level2(); else build_level1();
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
/* SHOW 3: CARTOON REELS. 'F' film-frame platforms blink in and out, 'S' sprocket saws */
static void filmplat(int x0, int x1, int y) { for (int x = x0; x <= x1; x++) put(x, y, 'F'); }
static void saw(int tx, int ty) { put(tx, ty, 'S'); }
static void build_level3(void) {
    for (int y = 0; y < LH; y++) { memset(map[y], '.', LW); map[y][LW] = 0; }
    nen = nbreak = ndrip = ncheck = 0;
    block(0, LW - 1, 13, 14);
    block(0, 0, 0, 12); block(LW - 1, LW - 1, 0, 12);
    /* TITLE CARD: hopping bombs, first blinking frames */
    checkpoint(2);
    spawn(E_BOMB, 14, 13); plat(17, 22, 10); spawn(E_CRATE, 20, 10);
    spawn(E_BOMB, 26, 13); filmplat(29, 33, 9); spawn(E_GLOVE, 36, 13); breaker(31);
    /* BARNYARD SHORT: gloves scuttle over the fences, ink ponds */
    checkpoint(41);
    block(48, 50, 12, 12); spawn(E_GLOVE, 55, 13); pit(58, 61); filmplat(57, 62, 10);
    spawn(E_BOMB, 66, 13); spawn(E_BOMB, 69, 13); saw(72, 12); block(75, 77, 11, 12); spawn(E_GLOVE, 76, 11);
    plat(79, 83, 9); spawn(E_HATCRATE, 81, 9);
    /* SPOOK SHORT: film ghosts fade in and out */
    checkpoint(86);
    spawn(E_GHOST, 94, 9); filmplat(97, 101, 10); saw(99, 7); spawn(E_GHOST, 106, 8);
    pit(108, 111); filmplat(107, 112, 10); spawn(E_GLOVE, 115, 13); spawn(E_GHOST, 119, 8); breaker(121);
    /* FILM VAULT: towers of film cans, sprocket saws, frames that skip */
    checkpoint(126);
    block(131, 133, 11, 12); saw(135, 12); block(137, 139, 10, 12); spawn(E_BOMB, 138, 10);
    filmplat(141, 145, 8); saw(143, 5); spawn(E_GHOST, 147, 8); block(149, 151, 11, 12); saw(153, 12);
    spawn(E_GLOVE, 157, 13); filmplat(159, 163, 10); spawn(E_CRATE, 161, 10); spawn(E_BOMB, 165, 13); breaker(167);
    /* CUTTING ROOM: an ink trough crossed on skipping frames, saws swinging */
    checkpoint(171);
    pit(175, 197); filmplat(175, 178, 11); filmplat(181, 184, 10); saw(182, 6); filmplat(187, 190, 11);
    filmplat(193, 197, 10); saw(195, 6); spawn(E_GHOST, 185, 7); spawn(E_GHOST, 193, 6);
    spawn(E_GLOVE, 201, 13); spawn(E_BOMB, 205, 13); breaker(203);
    /* THE SPLICING TABLE (boss) */
    checkpoint(209);
    for (int i = 0; i < nen; i++) en[i].plat1 = LW * TILE;
}
/* SHOW 4: THE INK WORKS. 'X' ceiling crushers, 'V' steam vents that launch you, '~' ink vats with eels in them */
static void crusher(int tx) { put(tx, 0, 'X'); }
static void vent(int tx) { put(tx, 12, 'V'); }
static void build_level4(void) {
    for (int y = 0; y < LH; y++) { memset(map[y], '.', LW); map[y][LW] = 0; }
    nen = nbreak = ndrip = ncheck = 0;
    block(0, LW - 1, 13, 14);
    block(1, LW - 2, 0, 0);   /* factory ceiling for the crushers to hang from */
    block(0, 0, 0, 12); block(LW - 1, LW - 1, 0, 12);
    /* LOADING BAY: crates, imps, the first steam vent */
    checkpoint(2);
    spawn(E_IMP, 14, 13); block(17, 19, 12, 12); spawn(E_CRATE, 18, 12);
    vent(23); plat(22, 27, 6); spawn(E_IMP, 25, 6); spawn(E_WORKER, 33, 13); breaker(30);
    /* PRESS FLOOR: crushers stamping, wind-up workers charging */
    checkpoint(41);
    crusher(48); spawn(E_WORKER, 54, 13); crusher(58); plat(61, 66, 9); spawn(E_IMP, 64, 9);
    crusher(70); spawn(E_WORKER, 75, 13); vent(78); plat(77, 82, 5); spawn(E_HATCRATE, 80, 5);
    /* INK VATS: eels in the ink, catwalks over the top */
    checkpoint(86);
    pit(91, 96); spawn(E_EEL, 93, 13); plat(90, 97, 9); spawn(E_IMP, 95, 9);
    pit(101, 106); spawn(E_EEL, 103, 13); plat(100, 107, 9);
    spawn(E_WORKER, 112, 13); pit(115, 119); spawn(E_EEL, 117, 13); plat(114, 120, 9); breaker(122);
    /* PIPE WORKS: vents launch you up to the high pipes */
    checkpoint(126);
    block(131, 133, 10, 12); vent(135); plat(134, 140, 4); spawn(E_IMP, 138, 4); crusher(143);
    spawn(E_WORKER, 147, 13); vent(151); plat(150, 157, 5); spawn(E_IMP, 155, 5); pit(152, 156); spawn(E_EEL, 154, 13);
    spawn(E_CRATE, 161, 13); crusher(164); breaker(167);
    /* BOILER FURNACE: crushers over an ink trough, eels below */
    checkpoint(171);
    pit(175, 196); plat(175, 179, 10); crusher(177); plat(182, 186, 9); spawn(E_EEL, 181, 13); plat(189, 193, 10); crusher(191);
    spawn(E_EEL, 188, 13); spawn(E_EEL, 195, 13); plat(195, 198, 9);
    spawn(E_WORKER, 202, 13); spawn(E_IMP, 206, 13); breaker(204);
    /* THE PRESS ROOM (boss) */
    checkpoint(209);
    for (int i = 0; i < nen; i++) en[i].plat1 = LW * TILE;
}
/* SHOW 5: LOST & FOUND. 'U' toy-block platforms crumble after you land, 'M' giant mousetraps snap */
static float crumble_t[LH][LW], regrow_t[LH][LW], trap_t[LW];
static void blocks(int x0, int x1, int y) { for (int x = x0; x <= x1; x++) put(x, y, 'U'); }
static void mousetrap(int tx) { put(tx, 12, 'M'); }
static void build_level5(void) {
    for (int y = 0; y < LH; y++) { memset(map[y], '.', LW); map[y][LW] = 0; }
    memset(crumble_t, 0, sizeof crumble_t); memset(regrow_t, 0, sizeof regrow_t); memset(trap_t, 0, sizeof trap_t);
    nen = nbreak = ndrip = ncheck = 0;
    block(0, LW - 1, 13, 14);
    block(0, 0, 0, 12); block(LW - 1, LW - 1, 0, 12);
    /* COAT CHECK: suitcases hop, umbrellas swoop */
    checkpoint(2);
    spawn(E_SUITCASE, 14, 13); plat(17, 22, 10); spawn(E_CRATE, 20, 10); spawn(E_UMBRELLA, 24, 7);
    mousetrap(27); spawn(E_SOLDIER, 32, 13); breaker(30);
    /* LOST PROPERTY: shelves to climb, soldiers below */
    checkpoint(41);
    block(46, 48, 11, 12); spawn(E_SOLDIER, 52, 13); spawn(E_SOLDIER, 55, 13); mousetrap(58); plat(57, 63, 9); spawn(E_UMBRELLA, 60, 6);
    spawn(E_SUITCASE, 67, 13); block(70, 72, 12, 12); mousetrap(75); plat(78, 83, 9); spawn(E_HATCRATE, 81, 9);
    /* TOY CHEST: toy-block bridges that crumble under you */
    checkpoint(86);
    pit(91, 96); blocks(90, 97, 10); spawn(E_UMBRELLA, 94, 6); spawn(E_SUITCASE, 100, 13);
    pit(104, 110); blocks(103, 106, 10); blocks(108, 111, 9); spawn(E_UMBRELLA, 107, 5);
    spawn(E_SOLDIER, 115, 13); mousetrap(118); breaker(122);
    /* LUGGAGE HALL: towers of trunks, suitcases leaping off them */
    checkpoint(126);
    block(130, 132, 11, 12); spawn(E_SUITCASE, 131, 11); block(135, 138, 9, 12); spawn(E_SOLDIER, 136, 9); mousetrap(140);
    block(142, 144, 11, 12); spawn(E_SUITCASE, 143, 11); spawn(E_UMBRELLA, 148, 6); blocks(150, 155, 9); pit(150, 155);
    spawn(E_SOLDIER, 159, 13); spawn(E_CRATE, 162, 13); mousetrap(164); breaker(167);
    /* DOLL HOSPITAL: crumbling blocks over an ink drain, umbrellas everywhere */
    checkpoint(171);
    pit(175, 196); blocks(175, 178, 10); blocks(181, 184, 9); blocks(187, 190, 10); blocks(193, 197, 9);
    spawn(E_UMBRELLA, 180, 5); spawn(E_UMBRELLA, 189, 5); spawn(E_UMBRELLA, 196, 6);
    spawn(E_SUITCASE, 201, 13); spawn(E_SOLDIER, 206, 13); breaker(203);
    /* THE CLAIM DESK (boss) */
    checkpoint(209);
    for (int i = 0; i < nen; i++) en[i].plat1 = LW * TILE;
}
/* SHOW 6: ENCORE. Every hazard and enemy comes back for the final show, plus ink-shadow understudies of you.
   The orchestra pit floods with ink in waves; the fly loft drops sandbags ('X' drawn as sandbags here) */
static void build_level6(void) {
    for (int y = 0; y < LH; y++) { memset(map[y], '.', LW); map[y][LW] = 0; }
    nen = nbreak = ndrip = ncheck = 0;
    block(0, LW - 1, 13, 14);
    block(0, 0, 0, 12); block(LW - 1, LW - 1, 0, 12);
    /* BURNING LOBBY: old friends from the first show */
    checkpoint(2);
    spawn(E_USHER, 14, 13); spawn(E_BLOB, 19, 13); plat(21, 26, 10); spawn(E_CRATE, 23, 10); spawn(E_SLICE, 27, 7);
    spawn(E_SHADOW, 33, 13); breaker(30); drip(17); drip(29);
    /* THE BALCONY: tiers of seats to climb, goats and soldiers */
    checkpoint(41);
    block(46, 49, 12, 12); block(50, 53, 11, 12); spawn(E_GOAT, 52, 11); block(54, 57, 10, 12); spawn(E_SOLDIER, 56, 10);
    filmplat(59, 63, 8); spawn(E_GHOST, 64, 7); block(66, 69, 11, 12); spawn(E_DOG, 72, 13); spawn(E_SHADOW, 76, 13);
    plat(78, 83, 9); spawn(E_HATCRATE, 81, 9);
    /* ORCHESTRA PIT: the ink floods in waves; get up on the music stands */
    checkpoint(86);
    plat(89, 93, 10); plat(96, 100, 9); spawn(E_IMP, 98, 9); plat(103, 107, 10); spawn(E_UMBRELLA, 105, 6);
    plat(110, 114, 9); spawn(E_GLOVE, 112, 9); plat(116, 120, 10); spawn(E_BOMB, 118, 10); breaker(122);
    /* THE FLY LOFT: sandbags drop on ropes, steam vents up to the rigging */
    checkpoint(126);
    crusher(132); spawn(E_WORKER, 136, 13); vent(140); plat(139, 145, 5); spawn(E_SUITCASE, 143, 5); crusher(148);
    spawn(E_SHADOW, 152, 13); saw(156, 12); crusher(160); spawn(E_MEATBALL, 163, 13); breaker(167);
    /* CURTAIN CALL: one last gauntlet over the ink */
    checkpoint(171);
    pit(175, 196); filmplat(175, 178, 10); blocks(181, 184, 9); plat(187, 190, 10); filmplat(193, 197, 9);
    spawn(E_EEL, 180, 13); spawn(E_EEL, 189, 13); spawn(E_GHOST, 185, 6); spawn(E_UMBRELLA, 194, 5);
    spawn(E_SHADOW, 201, 13); spawn(E_SHADOW, 206, 13); breaker(203);
    /* THE GRAND STAGE (final boss) */
    checkpoint(209);
    for (int i = 0; i < nen; i++) en[i].plat1 = LW * TILE;
}
static float film_phase(int tx);
static int film_on(int tx) { return film_phase(tx) < 2.1f; }
static int solid_at(int tx, int ty) { if (tx < 0 || tx >= LW) return 1; if (ty < 0 || ty >= LH) return 0; char c = map[ty][tx]; return c == '#' || c == 'O' || c == '>' || c == '<'; }
static int oneway_at(int tx, int ty) { if (tx < 0 || tx >= LW || ty < 0 || ty >= LH) return 0; char c = map[ty][tx]; return c == '=' || c == 'U' || (c == 'F' && film_on(tx)); }

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
enum { P_NAIL, P_RIVET, P_FLAME, P_PLUNGER, P_LIGHT, P_NOTE, P_TICKET, P_DRIP, P_CONFETTI, P_PIZZA, P_CHEESE, P_FILM, P_REEL, P_INKBLOB, P_SHEET, P_CORK, P_SHOE };
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
enum { SC_TITLE, SC_PLAY, SC_OVER, SC_CLEAR, SC_END };
static float scene_t, ping_cool, op_idle;
static float film_phase(int tx) { return fmodf(scene_t + (tx / 5) * 1.1f, 3.2f); }
static float negative_t;   /* the Splicer's frame-skip flash */

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

static int god_mode(void) { static int g = -1; if (g < 0) g = getenv("MM_GOD") != NULL; return g; }   /* tests: invincible runner */
static void reset_player(Player *p, int full);
static void hurt_player(Player *p, float from_x) {
    if (!p->alive || p->inv > 0 || p->hidden) return;
    if (god_mode()) { p->inv = 1.6f; p->vx = (p->x < from_x ? -1 : 1) * 260; p->vy = -420; return; }
    p->gear--; p->inv = 1.6f; shake = 8;
    p->vx = (p->x < from_x ? -1 : 1) * 260; p->vy = -420;
    sfx("hurt");
    if (p->gear == 2) { for (int i = 0; i < 6; i++) part(p->x, p->y - 140, (frand() - 0.5f) * 300, -300 - frand() * 200, 0.9f, 9, 900, 230, 180, 50, 0); message("THERE GOES THE HARD HAT!", 1.4f); }
    if (p->gear == 1) { for (int i = 0; i < 10; i++) part(p->x, p->y - 70, (frand() - 0.5f) * 400, -300 - frand() * 200, 1.0f, 8, 900, 70, 100, 150, 0); message("DOWN TO YOUR BOXERS!", 1.4f); }
    if (p->gear <= 0) { p->alive = 0; p->dead_t = 0; p->vy = -700; sfx("boom"); ink_burst(p->x, p->y - 60, 20, 300); }
}
static void kill_player(Player *p) { if (!p->alive) return; if (god_mode()) { reset_player(p, 0); return; } p->gear = 1; p->inv = 0; p->hidden = 0; hurt_player(p, p->x); }

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
    A.bg3 = load_or_empty("bg/reels.jpg");
    for (int i = 0; i < 5; i++) {
        snprintf(rel, sizeof rel, "chars/bomb_%d.png", i); A.bomb[i] = load_or_empty(rel); if (!A.bomb[i].tex && i) A.bomb[i] = A.bomb[0];
        snprintf(rel, sizeof rel, "chars/ghost_%d.png", i); A.ghost[i] = load_or_empty(rel); if (!A.ghost[i].tex && i) A.ghost[i] = A.ghost[0];
        snprintf(rel, sizeof rel, "chars/glove_%d.png", i); A.glove[i] = load_or_empty(rel); if (!A.glove[i].tex && i) A.glove[i] = A.glove[0];
    }
    for (int i = 0; i < 3; i++) { snprintf(rel, sizeof rel, "chars/splicer_%d.png", i); A.splicer[i] = load_or_empty(rel); }
    A.bg4 = load_or_empty("bg/inkworks.jpg");
    for (int i = 0; i < 5; i++) {
        snprintf(rel, sizeof rel, "chars/imp_%d.png", i); A.imp[i] = load_or_empty(rel); if (!A.imp[i].tex && i) A.imp[i] = A.imp[0];
        snprintf(rel, sizeof rel, "chars/worker_%d.png", i); A.worker[i] = load_or_empty(rel); if (!A.worker[i].tex && i) A.worker[i] = A.worker[0];
        snprintf(rel, sizeof rel, "chars/eel_%d.png", i); A.eel[i] = load_or_empty(rel); if (!A.eel[i].tex && i) A.eel[i] = A.eel[0];
    }
    for (int i = 0; i < 3; i++) { snprintf(rel, sizeof rel, "chars/press_%d.png", i); A.press[i] = load_or_empty(rel); }
    A.bg5 = load_or_empty("bg/lostfound.jpg");
    for (int i = 0; i < 5; i++) {
        snprintf(rel, sizeof rel, "chars/suitcase_%d.png", i); A.suitcase[i] = load_or_empty(rel); if (!A.suitcase[i].tex && i) A.suitcase[i] = A.suitcase[0];
        snprintf(rel, sizeof rel, "chars/soldier_%d.png", i); A.soldier[i] = load_or_empty(rel); if (!A.soldier[i].tex && i) A.soldier[i] = A.soldier[0];
        snprintf(rel, sizeof rel, "chars/umbrella_%d.png", i); A.umbrella[i] = load_or_empty(rel); if (!A.umbrella[i].tex && i) A.umbrella[i] = A.umbrella[0];
    }
    for (int i = 0; i < 3; i++) { snprintf(rel, sizeof rel, "chars/collector_%d.png", i); A.collector[i] = load_or_empty(rel); }
    A.bg6 = load_or_empty("bg/encore.jpg");
    for (int i = 0; i < 3; i++) { snprintf(rel, sizeof rel, "chars/unbound_%d.png", i); A.unbound[i] = load_or_empty(rel); }
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
    if (!strcmp(name, "level6") || !strcmp(name, "boss6") || !strcmp(name, "pit") || !strcmp(name, "loft") || !strcmp(name, "curtain") || !strcmp(name, "ending") || !strcmp(name, "loop2")) {
        level_no = 5; if (!strcmp(name, "loop2")) loop_no = 1; start_level();
        if (!strcmp(name, "boss6") || !strcmp(name, "ending") || !strcmp(name, "loop2")) { cur_check = ncheck - 1; reset_player(&pl[0], 0); }
        if (!strcmp(name, "pit")) { cur_check = 2; reset_player(&pl[0], 0); }
        if (!strcmp(name, "loft")) { cur_check = 3; reset_player(&pl[0], 0); }
        if (!strcmp(name, "curtain")) { cur_check = 4; reset_player(&pl[0], 0); }
        if (!strcmp(name, "ending")) { scene = SC_END; scene_t = 0; music_play("mm_ending", 0); }
        return;
    }
    if (!strcmp(name, "level5") || !strcmp(name, "boss5") || !strcmp(name, "toys") || !strcmp(name, "luggage") || !strcmp(name, "dolls")) {
        level_no = 4; start_level();
        if (!strcmp(name, "boss5")) { cur_check = ncheck - 1; reset_player(&pl[0], 0); }
        if (!strcmp(name, "toys")) { cur_check = 2; reset_player(&pl[0], 0); }
        if (!strcmp(name, "luggage")) { cur_check = 3; reset_player(&pl[0], 0); }
        if (!strcmp(name, "dolls")) { cur_check = 4; reset_player(&pl[0], 0); }
        return;
    }
    if (!strcmp(name, "level4") || !strcmp(name, "boss4") || !strcmp(name, "vats") || !strcmp(name, "pipes") || !strcmp(name, "furnace")) {
        level_no = 3; start_level();
        if (!strcmp(name, "boss4")) { cur_check = ncheck - 1; reset_player(&pl[0], 0); }
        if (!strcmp(name, "vats")) { cur_check = 2; reset_player(&pl[0], 0); }
        if (!strcmp(name, "pipes")) { cur_check = 3; reset_player(&pl[0], 0); }
        if (!strcmp(name, "furnace")) { cur_check = 4; reset_player(&pl[0], 0); }
        return;
    }
    if (!strcmp(name, "level3") || !strcmp(name, "boss3") || !strcmp(name, "vault") || !strcmp(name, "cutting") || !strcmp(name, "spook")) {
        level_no = 2; start_level();
        if (!strcmp(name, "boss3")) { cur_check = ncheck - 1; reset_player(&pl[0], 0); }
        if (!strcmp(name, "spook")) { cur_check = 2; reset_player(&pl[0], 0); }
        if (!strcmp(name, "vault")) { cur_check = 3; reset_player(&pl[0], 0); }
        if (!strcmp(name, "cutting")) { cur_check = 4; reset_player(&pl[0], 0); }
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
    case E_BOMB: *w = 64; *h = 76; break;
    case E_GHOST: *w = 76; *h = 104; break;
    case E_GLOVE: *w = 90; *h = 66; break;
    case E_IMP: *w = 60; *h = 84; break;
    case E_SUITCASE: *w = 84; *h = 74; break;
    case E_SHADOW: *w = 44; *h = 140; break;
    case E_SOLDIER: *w = 56; *h = 140; break;
    case E_UMBRELLA: *w = 84; *h = 56; break;
    case E_WORKER: *w = 70; *h = 140; break;
    case E_EEL: *w = 60; *h = (e->state == 1 ? 80 : e->state == 2 ? 150 : 20); break;
    default: *w = 56; *h = 48; break;
    }
}
/* film ghosts fade out of the picture for a while: you can't hit them (or be hit) while they're faded */
static int ghost_faded(Enemy *e) { return e->type == E_GHOST && e->state == 0 && fmodf(e->t + e->land, 4.0f) > 2.6f; }
static void enemy_damage(Enemy *e, int dmg);
static void bomb_blast(float x, float y) {   /* cartoon bombs: a big black-and-white KA-BOOM that hurts everyone nearby */
    sfx("boom"); if (shake < 12) shake = 12; hitstop = 3;
    ring(x, y, 180, 255, 255, 255); spark_burst(x, y, 30, 255, 240, 200); smoke(x, y, 10, 40); embers(x, y, 8);
    for (int i = 0; i < 14; i++) part_k(PK_DEBRIS, x, y, (frand() - 0.5f) * 800, -200 - frand() * 600, 2.0f, 6 + frand() * 8, 1600, 24, 22, 24, 0);
    for (int i = 0; i < 18; i++) { float a = frand() * 6.283f, v = 300 + frand() * 400; part(x, y, cosf(a) * v, sinf(a) * v, 0.35f, 10, 0, 255, 255, 255, 1); }
    for (int k = 0; k < 2; k++) { Player *p = &pl[k]; if (p->active && p->alive && !p->hidden && fabsf(p->x - x) < 150 && fabsf((p->y - 60) - y) < 150) hurt_player(p, x); }
    for (int j = 0; j < nen; j++) { Enemy *o = &en[j]; if (o->alive && o->type < E_CRATE && fabsf(o->x - x) < 150 && fabsf(o->y - 40 - y) < 150) enemy_damage(o, 3); }
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
    case E_BOMB: return &A.bomb[e->state == 1 ? 4 : !e->onground ? 2 : e->land > 0 ? 3 : e->timer < 0.2f ? 1 : 0];
    case E_GHOST: return &A.ghost[e->state == 1 ? 4 : ghost_faded(e) ? 3 : ((int)(e->t * 3)) % 2];
    case E_GLOVE: return &A.glove[e->state == 0 ? ((int)(e->t * 8)) % 2 : e->state == 1 ? 2 : e->state == 2 ? 3 : 4];
    case E_SHADOW: return &A.hero[0][!e->onground ? 5 : fabsf(e->vx) > 20 ? 1 + ((int)(e->t * 8)) % 4 : 0];
    case E_SUITCASE: return &A.suitcase[e->flash > 3 ? 4 : !e->onground ? 2 : e->land > 0 ? 3 : e->timer < 0.25f ? 1 : 0];
    case E_SOLDIER: return &A.soldier[e->flash > 3 ? 4 : e->state == 1 ? (e->timer > 0.15f ? 2 : 3) : (fabsf(e->vx) > 5 ? ((int)(e->t * 6)) % 2 : 0)];
    case E_UMBRELLA: return &A.umbrella[e->flash > 3 ? 4 : e->state == 1 ? 3 : ((int)(e->t * 8)) % 3];
    case E_IMP: return &A.imp[e->flash ? 4 : e->state == 1 ? (e->timer > 0.2f ? 2 : 3) : (fabsf(e->vx) > 5 ? ((int)(e->t * 7)) % 2 : 0)];
    case E_WORKER: return &A.worker[e->flash > 3 ? 4 : e->state == 1 ? 2 : e->state == 2 ? 3 : (fabsf(e->vx) > 5 ? ((int)(e->t * 7)) % 2 : 0)];
    case E_EEL: return &A.eel[e->state == 1 ? (e->timer > 0.2f ? 0 : 1) : e->state == 2 ? (e->timer < 0.6f && e->timer > 0.25f ? 3 : 2) : 4];
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
    case E_BOMB:    /* blows up for real */
        if (t) shatter(t, e->x, e->y, e->face > 0, 3, 3, 520, 0);
        bomb_blast(cx, e->y - 40);
        break;
    case E_GHOST:   /* tears like old film: grey scraps and a puff of ink */
        sfx("poof"); if (t) shatter(t, e->x, e->y, e->face > 0, 3, 3, 380, 0);
        for (int i = 0; i < 24; i++) part(cx, cy, (frand() - 0.5f) * 600, -150 - frand() * 400, 1.0f, 5, 700, 200, 200, 196, 0);
        for (int i = 0; i < 20; i++) { float a = frand() * 6.283f, v = 200 + frand() * 400;
            part_k(PK_BLOOD, cx, cy, cosf(a) * v, sinf(a) * v - 200, 1.4f, 4 + frand() * 6, 1500, 12, 8, 12, 0); }
        break;
    case E_GLOVE:   /* the eye in the palm pops; red is the only colour left in this cartoon */
        sfx("splat"); if (t) shatter(t, e->x, e->y, e->face > 0, 3, 3, 420, 1);
        gore_part(11, cx, cy, 480, 1); gore_part(11, cx, cy, 440, 1); gore_part(3, cx, cy, 520, 1);
        blood_spurt(cx, cy, -e->face, 34, 600); blood_spurt(cx, cy, e->face, 18, 480);
        break;
    case E_SHADOW:  /* an ink understudy: bursts back into ink */
        sfx("splat"); sfx("fiendwarn");
        for (int i = 0; i < 44; i++) { float a = frand() * 6.283f, v = 200 + frand() * 500;
            part_k(PK_BLOOD, cx, cy - 20, cosf(a) * v, sinf(a) * v - 250, 1.4f, 4 + frand() * 9, 1500, 12, 8, 12, 0); }
        ring(cx, cy - 20, 110, 40, 30, 40); smoke(cx, cy - 20, 6, 20);
        break;
    case E_SUITCASE: /* bursts open: clothes, papers, a toothy lid */
        sfx("splat"); if (t) shatter(t, e->x, e->y, e->face > 0, 3, 3, 440, 1);
        for (int i = 0; i < 16; i++) part_k(PK_DEBRIS, cx, cy, (frand() - 0.5f) * 700, -200 - frand() * 500, 2.0f, 6 + frand() * 8, 1500, 200 + rand() % 50, 190 + rand() % 50, 160, 0);
        blood_spurt(cx, cy, -e->face, 30, 560); gore_part(3, cx, cy, 520, 1);
        break;
    case E_SOLDIER: /* splinters: wooden limbs, a bearskin hat, springs */
        sfx("clank"); if (t) shatter(t, e->x, e->y, e->face > 0, 3, 4, 460, 1);
        for (int i = 0; i < 12; i++) part_k(PK_DEBRIS, cx, cy - 20, (frand() - 0.5f) * 600, -200 - frand() * 500, 2.0f, 6 + frand() * 8, 1500, 150, 100, 55, 0);
        gore_part(5, cx, cy, 520, 0); blood_spurt(cx, cy - 40, -e->face, 22, 520);
        break;
    case E_UMBRELLA: /* shredded canopy and a snapped handle */
        sfx("splat"); if (t) shatter(t, e->x, e->y, e->face > 0, 3, 2, 380, 1);
        for (int i = 0; i < 14; i++) part(cx, cy, (frand() - 0.5f) * 600, -150 - frand() * 350, 1.2f, 6, 800, 20, 18, 22, 0);
        blood_spurt(cx, cy, -e->face, 16, 460);
        break;
    case E_IMP:     /* the inkwell shatters: glass and a big ink splash */
        sfx("shatter"); if (t) shatter(t, e->x, e->y, e->face > 0, 3, 3, 420, 0);
        for (int i = 0; i < 36; i++) { float a = frand() * 6.283f, v = 200 + frand() * 450;
            part_k(PK_BLOOD, cx, cy, cosf(a) * v, sinf(a) * v - 250, 1.4f, 4 + frand() * 8, 1500, 12, 8, 12, 0); }
        for (int i = 0; i < 10; i++) part_k(PK_DEBRIS, cx, cy, (frand() - 0.5f) * 600, -200 - frand() * 400, 1.5f, 4 + frand() * 5, 1500, 200, 220, 230, 0);
        break;
    case E_WORKER:  /* tin worker bursts: springs, rivets, sparks and oil */
        sfx("clank"); sfx("boom"); if (t) shatter(t, e->x, e->y, e->face > 0, 3, 4, 480, 1);
        gore_part(6, cx, cy, 600, 0); gore_part(5, cx, cy, 560, 0); gore_part(5, cx, cy - 40, 520, 0); gore_part(0, cx, cy, 500, 0); gore_part(1, cx, cy, 520, 0);
        spark_burst(cx, cy - 30, 26, 255, 210, 110); smoke(cx, cy - 30, 6, 80); blood_spurt(cx, cy - 40, -e->face, 24, 520);
        ring(cx, cy - 30, 100, 255, 210, 150);
        break;
    case E_EEL:     /* the eel bursts in a spray of ink and red */
        sfx("splat"); if (t) shatter(t, e->x, e->y, e->face > 0, 2, 4, 400, 1);
        blood_spurt(cx, e->y - 90, -e->face, 30, 600);
        for (int i = 0; i < 30; i++) { float a = frand() * 6.283f, v = 200 + frand() * 450;
            part_k(PK_BLOOD, cx, e->y - 80, cosf(a) * v, sinf(a) * v - 250, 1.4f, 4 + frand() * 7, 1500, 12, 8, 12, 0); }
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
    float speed_k = (darkness_on ? 1.35f : 1.0f) * (loop_no ? 1.25f : 1.0f);
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
        if (e->type == E_SLICE || e->type == E_UMBRELLA) {   /* flies: bob around home, then dive-bomb */
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
        if (e->type == E_EEL) {   /* hides in the ink; when you come close it rises and lunges */
            e->timer -= DT * speed_k;
            if (t) e->face = t->x < e->x ? -1 : 1;
            if (e->state == 0) { if (frame_no % 20 == 0) part_k(PK_BLOOD, e->x + (frand() - 0.5f) * 30, FLOOR_Y - 4, 0, -80, 0.4f, 4, 600, 20, 16, 20, 0);
                if (e->timer <= 0 && t && fabsf(t->x - e->x) < 300) { e->state = 1; e->timer = 0.45f; sfx("splash"); } }
            else if (e->state == 1) { if (e->timer <= 0) { e->state = 2; e->timer = 1.3f; sfx("spit"); } }
            else if (e->state == 2) {
                if (e->timer < 0.6f && e->timer > 0.25f) for (int k = 0; k < 2; k++) { Player *p = &pl[k];   /* the lunge reaches out */
                    if (p->active && p->alive && !p->hidden && overlap(p->x, p->y, 40, 110, e->x + e->face * 70, e->y - 90, 120, 110)) hurt_player(p, e->x); }
                if (e->timer <= 0) { e->state = 3; e->timer = 0.4f; } }
            else if (e->timer <= 0) { e->state = 0; e->timer = 1.6f + frand(); }
            if (e->state == 2) for (int k = 0; k < 2; k++) { Player *p = &pl[k]; if (p->active && p->alive && !p->hidden && overlap(p->x, p->y, 40, 110, e->x, e->y, 50, 150)) hurt_player(p, e->x); }
            continue;
        }
        if (e->type == E_GHOST) {   /* floats after you, fades in and out of the picture, lunges when solid */
            e->timer -= DT * speed_k;
            if (e->state == 0) {
                if (t) { e->face = t->x < e->x ? -1 : 1;
                    float hx = t->x - e->face * 200, hy = t->y - 110;
                    e->vx += ((hx - e->x) * 1.0f - e->vx) * 2 * DT; e->vy += ((hy - e->y) * 1.0f - e->vy) * 2 * DT;
                    e->vx = clampf(e->vx, -170, 170); e->vy = clampf(e->vy, -140, 140);
                    if (!ghost_faded(e) && dist < 460 && e->timer <= 0) { e->state = 1; e->timer = 0.7f;
                        float dx = t->x - e->x, dy = (t->y - 40) - e->y, l = sqrtf(dx * dx + dy * dy) + 1;
                        e->vx = dx / l * 460 * speed_k; e->vy = dy / l * 460 * speed_k; sfx("fiendwarn"); } }
                e->y += sinf(e->t * 2.5f) * 0.6f;
            } else if (e->timer <= 0) { e->state = 0; e->timer = 2.0f + frand(); e->vx *= 0.3f; e->vy *= 0.3f; }
            e->x += e->vx * DT; e->y += e->vy * DT;
            if (e->y > FLOOR_Y) { e->y = FLOOR_Y; e->vy = -100; } if (e->y < 140) e->y = 140;
            if (frame_no % 5 == 0 && !ghost_faded(e)) part(e->x - e->face * 30, e->y - 20, -e->face * 40, 20, 0.6f, 5, 0, 190, 190, 186, 0);
            if (!ghost_faded(e)) for (int k = 0; k < 2; k++) { Player *p = &pl[k]; if (p->active && p->alive && !p->hidden && overlap(p->x, p->y, 40, 110, e->x, e->y, 60, 90)) hurt_player(p, e->x); }
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
        case E_BOMB:   /* hops at you; up close the fuse burns down and it goes off */
            if (frame_no % 3 == 0) part(e->x + e->face * 10, e->y - 84, (frand() - 0.5f) * 80, -100 - frand() * 80, 0.25f, 4, 300, 255, 230, 150, 1);
            if (e->state == 0) {
                e->timer -= DT * speed_k;
                if (e->onground) { e->vx *= 0.85f;
                    if (t && fabsf(t->x - e->x) < 120 && fabsf(t->y - e->y) < 130) { e->state = 1; e->timer = 0.8f; sfx("tick"); }
                    else if (e->timer <= 0 && t && dist < 800) { e->face = t->x < e->x ? -1 : 1; e->vx = e->face * 210; e->vy = -500; e->timer = 0.8f + frand() * 0.4f; } }
            } else {
                e->vx *= 0.8f; e->timer -= DT;
                if (frame_no % 8 == 0) sfx("tick");
                if (e->timer <= 0) { e->alive = 0; score += 50; gore_enemy(e); continue; }
            }
            break;
        case E_SHADOW:   /* an ink copy of you: runs, jumps and fires nails back */
            if (t) {
                e->face = t->x < e->x ? -1 : 1; float d = fabsf(t->x - e->x);
                float want = d > 340 ? e->face * 210 : d < 200 ? -e->face * 160 : 0; e->vx += (want * speed_k - e->vx) * 6 * DT;
                e->timer -= DT * speed_k;
                if (e->onground && rand() % 140 == 0) e->vy = -820;
                if (e->timer <= 0 && dist < 760) { Proj *q = proj(P_NAIL, e->x + e->face * 40, e->y - 82, e->face * 620, 0, 1, 1); if (q) q->r = 7; sfx("throw"); e->timer = 0.9f + frand() * 0.6f; }
            }
            break;
        case E_SUITCASE:   /* big chomping hops toward you */
            e->timer -= DT * speed_k;
            if (e->onground) { e->vx *= 0.8f; if (e->timer <= 0 && t && dist < 820) {
                e->face = t->x < e->x ? -1 : 1; e->vx = e->face * 240; e->vy = -620; e->timer = 0.9f + frand() * 0.5f; sfx("boing"); } }
            break;
        case E_SOLDIER:   /* marches in, kneels and fires corks low along the floor: jump them */
            if (t && dist < 800) e->face = t->x < e->x ? -1 : 1;
            if (e->state == 0) {
                e->vx = (t && dist < 800 && fabsf(t->x - e->x) > 280) ? e->face * 80 * speed_k : 0;
                e->timer -= DT * speed_k;
                if (e->timer <= 0 && t && dist < 700 && fabsf(t->y - e->y) < 100) { e->state = 1; e->timer = 0.5f; e->vx = 0; }
            } else {
                e->timer -= DT * speed_k;
                if (e->timer <= 0) {
                    Proj *q = proj(P_CORK, e->x + e->face * 46, e->y - 34, e->face * 400, 0, 1, 1); if (q) q->r = 10;
                    smoke(e->x + e->face * 50, e->y - 36, 2, 220); sfx("throw"); e->state = 0; e->timer = 1.8f + frand();
                }
            }
            break;
        case E_IMP:   /* waddles near, then lobs a blob of ink */
            if (t && dist < 760) e->face = t->x < e->x ? -1 : 1;
            if (e->state == 0) {
                e->vx = (t && dist < 760 && fabsf(t->x - e->x) > 200) ? e->face * 95 * speed_k : 0;
                e->timer -= DT * speed_k;
                if (e->timer <= 0 && t && dist < 650) { e->state = 1; e->timer = 0.5f; e->vx = 0; }
            } else {
                e->timer -= DT * speed_k;
                if (e->timer <= 0 && t) {
                    float dx = t->x - e->x, tt = 0.8f;
                    Proj *q = proj(P_INKBLOB, e->x + e->face * 20, e->y - 80, clampf(dx / tt, -520, 520), -560, 1, 1); if (q) { q->grav = 1300; q->r = 13; }
                    sfx("spit"); e->state = 0; e->timer = 2.2f + frand();
                }
            }
            break;
        case E_WORKER:   /* clanks along; when it sees you it revs the jackhammer and charges */
            if (e->state == 0) {
                if (e->vx == 0) e->vx = e->face * 70;
                e->vx = signf(e->vx) * 70 * speed_k; e->face = e->vx < 0 ? -1 : 1;
                e->timer -= DT;
                if (t && fabsf(t->x - e->x) < 560 && fabsf(t->y - e->y) < 120 && e->timer <= 0) { e->state = 1; e->timer = 0.7f; e->face = t->x < e->x ? -1 : 1; e->vx = 0; sfx("charge"); }
            } else if (e->state == 1) {
                e->vx = 0; e->timer -= DT; if (frame_no % 4 == 0) smoke(e->x - e->face * 30, e->y - 110, 1, 200);
                if (e->timer <= 0) { e->state = 2; e->timer = 1.1f; sfx("clank"); }
            } else {
                e->vx = e->face * 430 * speed_k; e->timer -= DT;
                if (frame_no % 3 == 0) part(e->x + e->face * 50, e->y - 30, (frand() - 0.5f) * 200, -100 - frand() * 150, 0.3f, 3, 900, 255, 220, 140, 1);
                if (e->timer <= 0) { e->state = 0; e->timer = 1.4f; }
            }
            break;
        case E_GLOVE:   /* scuttles on its fingertips, crouches, then leaps in with a punch */
            if (t && dist < 900) {
                if (e->state == 0) { e->face = t->x < e->x ? -1 : 1; e->vx = e->face * 125 * speed_k; e->timer -= DT;
                    if (e->onground && fabsf(t->x - e->x) < 340 && e->timer <= 0) { e->state = 1; e->timer = 0.4f; e->vx = 0; } }
                else if (e->state == 1) { e->vx = 0; e->timer -= DT; if (e->timer <= 0) { e->state = 2; e->vy = -640; e->vx = e->face * 440 * speed_k; sfx("jump"); } }
                else if (e->state == 2) { if (e->onground && e->vy >= 0) { e->state = 3; e->timer = 0.45f; e->vx = 0; if (shake < 4) shake = 4; smoke(e->x, e->y, 3, 150); } }
                else { e->vx = 0; e->timer -= DT; if (e->timer <= 0) { e->state = 0; e->timer = 1.0f; } }
            } else e->vx = 0;
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
        if (!was_ground && e->onground && (e->type == E_BOMB || e->type == E_SUITCASE)) { e->land = 0.15f; if (e->type == E_SUITCASE) sfx("slam"); }
        if (!was_ground && e->onground && (e->type == E_BLOB || e->type == E_MINIBLOB)) { e->land = 0.18f; part_k(PK_BLOOD, e->x, e->y - 6, -80, -120, 0.6f, 4, 1200, 12, 8, 12, 0); part_k(PK_BLOOD, e->x, e->y - 6, 80, -120, 0.6f, 4, 1200, 12, 8, 12, 0); }
        if (e->land > 0) e->land -= DT;
        if ((r & 2) && (e->type == E_DOG || e->type == E_MEATBALL)) e->vx = -e->vx * 0.8f;
        if ((r & 2) && e->type == E_WORKER) { if (e->state == 2) { e->state = 0; e->timer = 1.4f; shake = 6; spark_burst(e->x + e->face * 40, e->y - 60, 12, 255, 220, 140); sfx("clank"); } e->vx = -e->face * 70; e->face = -e->face; }
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
/* sprocket saws: spinning film sprockets with teeth. The ones up in the air swing up and down */
static float saw_y(int tx, int ty) { return ty * TILE + 24 + (ty < 12 ? sinf(scene_t * 2.0f + tx) * 60 : 0); }
static void update_saws(void) {
    int t0 = (int)(camx / TILE) - 2, t1 = t0 + SCREEN_W / TILE + 4;
    for (int tx = t0; tx <= t1; tx++) { if (tx < 1 || tx >= LW) continue;
        for (int ty = 0; ty < LH; ty++) { if (map[ty][tx] != 'S') continue;
            float cx = tx * TILE + 24, cy = saw_y(tx, ty);
            if (frame_no % 7 == 0) part(cx + (frand() - 0.5f) * 40, cy + 26, (frand() - 0.5f) * 300, -150 - frand() * 150, 0.3f, 3, 900, 255, 250, 230, 1);
            for (int k = 0; k < 2; k++) { Player *p = &pl[k]; if (p->active && p->alive && !p->hidden && overlap(p->x, p->y, 40, 110, cx, cy + 30, 60, 60)) hurt_player(p, cx); }
        } }
}
/* ceiling crushers: a riveted piston slams to the floor on a timer (LIGHTS in the zone shuts them down) */
static float crush_phase(int tx) { return fmodf(scene_t + tx * 0.53f, 3.2f); }
static float crusher_bottom(int tx) {   /* y of the piston head's underside */
    float ph = crush_phase(tx), up = TILE * 1.0f + 20, down = FLOOR_Y;
    if (lit[zone_of(tx * TILE)] > 0) return up;
    if (ph < 2.0f) return up + (ph > 1.6f ? sinf(frame_no * 1.3f) * 3 : 0);
    if (ph < 2.14f) return up + (down - up) * ((ph - 2.0f) / 0.14f);
    if (ph < 2.7f) return down;
    return down - (down - up) * ((ph - 2.7f) / 0.5f);
}
/* steam vents: a blast of steam that throws you high into the air (that's how you reach the high catwalks) */
static int vent_on(int tx) { return fmodf(scene_t + tx * 0.7f, 2.6f) < 0.9f; }
static void update_crushers_vents(void) {
    int t0 = (int)(camx / TILE) - 2, t1 = t0 + SCREEN_W / TILE + 4;
    for (int tx = t0; tx <= t1; tx++) { if (tx < 1 || tx >= LW) continue;
        if (map[0][tx] == 'X') {
            float b = crusher_bottom(tx), ph = crush_phase(tx), cx = tx * TILE + 24;
            if (lit[zone_of(tx * TILE)] <= 0 && ph >= 2.14f && ph < 2.14f + DT * 1.5f) { sfx("slam"); if (cx > camx && cx < camx + SCREEN_W) { shake = fmaxf(shake, 7); smoke(cx, FLOOR_Y - 6, 4, 120); for (int k = 0; k < 6; k++) part_k(PK_DEBRIS, cx, FLOOR_Y - 6, (frand() - 0.5f) * 500, -150 - frand() * 300, 1.5f, 5, 1600, 60, 50, 44, 0); } }
            for (int k = 0; k < 2; k++) { Player *p = &pl[k]; if (p->active && p->alive && !p->hidden && fabsf(p->x - cx) < 56 && b > p->y - 120 && ph > 2.0f && ph < 2.7f) hurt_player(p, cx); }
            for (int j = 0; j < nen; j++) { Enemy *e = &en[j]; if (e->alive && e->type < E_CRATE && e->type != E_EEL && fabsf(e->x - cx) < 56 && b > e->y - 40 && ph > 2.0f && ph < 2.7f) enemy_damage(e, 99); }
        }
        if (map[12][tx] == 'V' && vent_on(tx)) {
            float cx = tx * TILE + 24;
            if (frame_no % 2 == 0) part_k(PK_SMOKE, cx + (frand() - 0.5f) * 30, FLOOR_Y - 20, (frand() - 0.5f) * 60, -500 - frand() * 200, 0.7f, 14, 0, 230, 230, 225, 0);
            if (fmodf(scene_t + tx * 0.7f, 2.6f) < DT * 1.5f && cx > camx && cx < camx + SCREEN_W) sfx("sizzle");
            for (int k = 0; k < 2; k++) { Player *p = &pl[k]; if (p->active && p->alive && !p->hidden && fabsf(p->x - cx) < 46 && p->y > FLOOR_Y - 380 && p->vy > -900) { p->vy = -1250; p->onground = 0; } }
        }
    }
}
/* toy blocks crumble a moment after you land on them, then stack themselves back up; mousetraps snap */
static void update_blocks_traps(void) {
    if (level_no != 4 && level_no != 5) return;
    for (int k = 0; k < 2; k++) { Player *p = &pl[k]; if (!p->active || !p->alive || !p->onground) continue;
        int ty = (int)((p->y + 2) / TILE);
        for (int dx = -1; dx <= 1; dx++) { int tx = (int)((p->x + dx * 16) / TILE);
            if (tx >= 0 && tx < LW && ty >= 0 && ty < LH && map[ty][tx] == 'U' && crumble_t[ty][tx] == 0) { crumble_t[ty][tx] = 0.01f; sfx("tick"); } } }
    int t0 = (int)(camx / TILE) - 4, t1 = t0 + SCREEN_W / TILE + 8;
    for (int ty = 0; ty < LH; ty++) for (int tx = t0; tx <= t1; tx++) { if (tx < 0 || tx >= LW) continue;
        if (map[ty][tx] == 'U' && crumble_t[ty][tx] > 0) { crumble_t[ty][tx] += DT;
            if (crumble_t[ty][tx] > 0.55f) { map[ty][tx] = 'u'; crumble_t[ty][tx] = 0; regrow_t[ty][tx] = 4.0f; sfx("poof");
                for (int i = 0; i < 4; i++) part_k(PK_DEBRIS, tx * TILE + 24, ty * TILE + 10, (frand() - 0.5f) * 300, -100 - frand() * 200, 2.0f, 10 + frand() * 6, 1600, 180 + rand() % 60, 60 + rand() % 120, 50 + rand() % 80, 0); } }
        else if (map[ty][tx] == 'u') { regrow_t[ty][tx] -= DT;
            int blocked = 0; for (int k = 0; k < 2; k++) if (pl[k].active && fabsf(pl[k].x - (tx * TILE + 24)) < 40 && fabsf(pl[k].y - ty * TILE) < 60) blocked = 1;
            if (regrow_t[ty][tx] <= 0 && !blocked) { map[ty][tx] = 'U'; sfx("pickup"); } }
    }
    for (int tx = t0; tx <= t1; tx++) { if (tx < 0 || tx >= LW || map[12][tx] != 'M') continue;
        float cx = tx * TILE + 24;
        if (trap_t[tx] == 0) { for (int k = 0; k < 2; k++) { Player *p = &pl[k]; if (p->active && p->alive && !p->hidden && fabsf(p->x - cx) < 46 && p->y > FLOOR_Y - 40) { trap_t[tx] = 0.01f; sfx("tick"); } } }
        else { float before = trap_t[tx]; trap_t[tx] += DT;
            if (before < 0.28f && trap_t[tx] >= 0.28f) { sfx("slam"); shake = fmaxf(shake, 5); spark_burst(cx, FLOOR_Y - 20, 8, 255, 230, 180);
                for (int k = 0; k < 2; k++) { Player *p = &pl[k]; if (p->active && p->alive && !p->hidden && fabsf(p->x - cx) < 60 && p->y > FLOOR_Y - 70) hurt_player(p, cx); }
                for (int j = 0; j < nen; j++) { Enemy *e = &en[j]; if (e->alive && e->type < E_CRATE && fabsf(e->x - cx) < 60 && e->y > FLOOR_Y - 70) enemy_damage(e, 3); } }
            if (trap_t[tx] > 2.2f) trap_t[tx] = 0; }
    }
}
/* the orchestra pit floods with ink in waves (Show 6) */
static float tide_surface(void) {   /* y of the ink's surface in the pit zone */
    float ph = fmodf(scene_t, 9.0f), low = FLOOR_Y + 30, high = FLOOR_Y - 120;
    if (ph < 4.0f) return low; if (ph < 5.0f) return low + (high - low) * (ph - 4.0f);
    if (ph < 8.0f) return high + sinf(scene_t * 3) * 4; return high + (low - high) * (ph - 8.0f);
}
static void update_tide(void) {
    if (level_no != 5) return;
    float x0 = ZONE_X0[2] * TILE, x1 = ZONE_X0[3] * TILE, surf = tide_surface(), ph = fmodf(scene_t, 9.0f);
    int in = 0; for (int k = 0; k < 2; k++) if (pl[k].active && pl[k].x > x0 && pl[k].x < x1) in = 1;
    if (in && ph >= 3.0f && ph < 3.0f + DT * 1.5f) { message("THE INK IS RISING! GET UP HIGH!", 1.6f); sfx("fiendwarn"); }
    if (in && ph >= 4.0f && ph < 4.0f + DT * 1.5f) sfx("splash");
    for (int k = 0; k < 2; k++) { Player *p = &pl[k]; if (p->active && p->alive && !p->hidden && p->x > x0 && p->x < x1 && p->y > surf + 12) hurt_player(p, p->x + 1); }
    if (in && surf < FLOOR_Y && frame_no % 3 == 0) part_k(PK_BLOOD, x0 + frand() * (x1 - x0), surf, (frand() - 0.5f) * 80, -100 - frand() * 120, 0.6f, 4, 900, 12, 8, 12, 0);
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
    boss.active = 1; boss.state = BS_INTRO; boss.timer = 2.6f; boss.maxhp = boss.hp = level_no == 5 ? 260 : level_no == 4 ? 200 : level_no == 3 ? 180 : level_no == 2 ? 160 : level_no == 1 ? 140 : 110; boss.face = -1;
    if (loop_no) boss.maxhp = boss.hp = boss.hp * 13 / 10;   /* second run: tougher bosses */
    if (getenv("MM_BOSS_HP")) boss.hp = atoi(getenv("MM_BOSS_HP"));   /* tests */
    boss.x = ARENA_X + SCREEN_W - 230; boss.y = FLOOR_Y;
    arena_locked = 1; music_play(LEVELS[level_no].boss_music, 1); sfx(level_no == 1 ? "angry" : "roar1"); shake = 10;
}
static void boss_hit_player_box(float x, float y, float w, float h) {
    for (int k = 0; k < 2; k++) { Player *p = &pl[k]; if (p->active && p->alive && overlap(p->x, p->y, 40, p->crouch ? 80 : 120, x, y, w, h)) hurt_player(p, x); }
}
static void update_boss_rat(void);
static void update_boss_splicer(void);
static void update_boss_press(void);
static void update_boss_collector(void);
static void update_boss_unbound(void);
static void update_boss(void) {
    if (!boss.active) {
        if (!arena_locked) for (int k = 0; k < 2; k++) if (pl[k].active && pl[k].alive && pl[k].x > ARENA_X + 150) boss_start();
        return;
    }
    if (level_no == 5) { update_boss_unbound(); return; }
    if (level_no == 4) { update_boss_collector(); return; }
    if (level_no == 3) { update_boss_press(); return; }
    if (level_no == 2) { update_boss_splicer(); return; }
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
enum { SS_SNIP_AIM = 40, SS_SNIP, SS_FILM, SS_REELS, SS_FLICK_OUT, SS_FLICK_IN };
enum { PS_STAMP_AIM = 60, PS_STAMP, PS_SHEETS, PS_ROLL_WARN, PS_ROLL };
enum { CS_STAB_AIM = 80, CS_STAB, CS_THROW, CS_SUCK };
enum { US_SLAM_AIM = 100, US_SLAM, US_RAIN, US_BEAM_WARN, US_BEAM };
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
/* ------------------------------------------------------------ boss: The Splicer (Show 3) */
static void update_boss_splicer(void) {
    float angry = boss.hp < boss.maxhp / 2 ? 0.7f : 1.0f;
    boss.t += DT; if (boss.flash > 0) boss.flash--;
    boss.timer -= DT; boss.bob = sinf(boss.t * 3) * 6;
    Player *t = nearest_player(boss.x, boss.y, NULL);
    float lo = ARENA_X + 250, hi = ARENA_X + SCREEN_W - 250;
    if (t && boss.state != BS_DEAD && boss.state != SS_FLICK_OUT) boss.face = t->x < boss.x ? -1 : 1;
    switch (boss.state) {
    case BS_INTRO: boss.pose = 0; if (boss.timer <= 0) { boss.state = BS_IDLE; boss.timer = 1.0f; } break;
    case BS_IDLE:
        boss.pose = angry < 1 ? 2 : 0;
        if (t) { float want = t->x - boss.face * 360; boss.x += clampf(want - boss.x, -1, 1) * 150 / angry * DT; }
        boss.x = clampf(boss.x, lo, hi);
        if (boss.timer <= 0) {
            int k = rand() % 4;
            if (k == 0) { boss.state = SS_SNIP_AIM; boss.slams = angry < 1 ? 3 : 2; boss.timer = 0.75f * angry; boss.targetx = t ? t->x : boss.x; sfx("charge"); }
            else if (k == 1) { boss.state = SS_FILM; boss.slams = angry < 1 ? 5 : 3; boss.timer = 0.4f; }
            else if (k == 2) { boss.state = SS_REELS; boss.slams = angry < 1 ? 4 : 3; boss.timer = 0.3f; sfx("summon"); }
            else { boss.state = SS_FLICK_OUT; boss.timer = 0.7f; negative_t = 0.7f; sfx("thunder"); }
            if (angry < 1 && !boss.summoned) { boss.summoned = 1; spawn(E_BOMB, (int)((ARENA_X + 160) / TILE), 12); spawn(E_BOMB, (int)((ARENA_X + SCREEN_W - 160) / TILE), 12); sfx("summon"); }
        }
        break;
    case SS_SNIP_AIM:   /* giant scissor blades open around you, then SNIP */
        boss.pose = 1;
        if (boss.timer > 0.25f && t) boss.targetx += (t->x - boss.targetx) * 0.1f;
        if (boss.timer <= 0) { boss.state = SS_SNIP; boss.timer = 0.25f; sfx("slam"); shake = 10; hitstop = 2;
            spark_burst(boss.targetx, FLOOR_Y - 80, 24, 255, 255, 255); ring(boss.targetx, FLOOR_Y - 80, 130, 255, 255, 255); }
        break;
    case SS_SNIP:
        boss.pose = 1; boss_hit_player_box(boss.targetx, FLOOR_Y, 150, 220);
        if (boss.timer <= 0) { if (--boss.slams > 0) { boss.state = SS_SNIP_AIM; boss.timer = 0.5f * angry; } else { boss.state = BS_IDLE; boss.timer = 1.2f * angry; } }
        break;
    case SS_FILM:       /* spits strips of film at you */
        boss.pose = 1;
        if (boss.timer <= 0 && t) {
            float x0 = boss.x + boss.face * 110, y0 = boss.y - 200, dx = t->x - x0, dy = (t->y - 60) - y0, l = sqrtf(dx * dx + dy * dy) + 1;
            Proj *q = proj(P_FILM, x0, y0, dx / l * 520, dy / l * 520, 1, 1); if (q) { q->r = 14; q->life = 3; }
            sfx("spit"); boss.timer = 0.35f * angry; if (--boss.slams <= 0) { boss.state = BS_IDLE; boss.timer = 1.1f * angry; } }
        break;
    case SS_REELS:      /* film reels rain from the rafters and roll along the floor */
        boss.pose = 0;
        if (boss.timer <= 0) { float x = t ? t->x + (frand() - 0.5f) * 320 : ARENA_X + frand() * SCREEN_W;
            Proj *q = proj(P_REEL, clampf(x, ARENA_X + 80, ARENA_X + SCREEN_W - 80), -40, (frand() < 0.5f ? -1 : 1) * (200 + frand() * 120), 0, 1, 1);
            if (q) { q->grav = 1500; q->r = 28; q->bounces = 4; q->life = 5; }
            sfx("clank"); boss.timer = 0.45f * angry; if (--boss.slams <= 0) { boss.state = BS_IDLE; boss.timer = 1.2f * angry; } }
        break;
    case SS_FLICK_OUT:  /* the picture skips: she flickers out and reappears on your other side */
        boss.pose = 0;
        if (boss.timer <= 0) { float side = t ? t->x : boss.x;
            boss.x = side < ARENA_X + SCREEN_W / 2 ? hi : lo; boss.state = SS_FLICK_IN; boss.timer = 0.45f; negative_t = 0.3f; sfx("thunder");
            if (t) boss.face = t->x < boss.x ? -1 : 1; }
        break;
    case SS_FLICK_IN:
        boss.pose = 1;
        if (boss.timer <= 0) { boss.state = SS_SNIP_AIM; boss.slams = 1; boss.timer = 0.55f * angry; boss.targetx = t ? t->x : boss.x; sfx("charge"); }
        break;
    case BS_DEAD:
        if (boss.pose >= 0) boss.pose = 2;
        if (frame_no % 4 == 0) { sfx("boom"); ink_burst(boss.x + (frand() - 0.5f) * 260, boss.y - frand() * 280, 10, 300);
            for (int k = 0; k < 5; k++) part(boss.x, boss.y - 180, (frand() - 0.5f) * 700, -300 - frand() * 400, 1.0f, 5, 800, 200, 196, 186, 0);
            if (frame_no % 8 == 0) gore_part(rand() % 12, boss.x + (frand() - 0.5f) * 200, boss.y - 120 - frand() * 200, 700, rand() & 1); }
        if (!boss.summoned && boss.timer < 1.0f) {
            boss.summoned = 1; shatter(&A.splicer[2], boss.x, boss.y + 10, boss.face > 0, 5, 4, 850, 1); boss.pose = -1;
            for (int k = 0; k < 6; k++) gore_part(11, boss.x, boss.y - 160, 800, 1);
            blood_spurt(boss.x, boss.y - 200, -1, 50, 900); blood_spurt(boss.x, boss.y - 200, 1, 50, 900);
            for (int i = 0; i < 40; i++) { float a = frand() * 6.283f, v = 300 + frand() * 500; part_k(PK_BLOOD, boss.x, boss.y - 180, cosf(a) * v, sinf(a) * v - 300, 1.5f, 5 + frand() * 8, 1500, 12, 8, 12, 0); }
            ring(boss.x, boss.y - 180, 320, 255, 255, 255); shake = 24; hitstop = 10; negative_t = 0.25f;
        }
        if (boss.timer <= 0 && scene == SC_PLAY) { scene = SC_CLEAR; scene_t = 0; music_play("mm_clear", 0); }
        break;
    }
    if (boss.state != BS_DEAD && boss.state != BS_INTRO && boss.state != SS_FLICK_OUT) boss_hit_player_box(boss.x, boss.y, 230, 220);
}
static void draw_boss_splicer(float ox, float oy) {
    float x = sx(boss.x) + ox, y = boss.y + oy + boss.bob;
    if (boss.state == SS_SNIP_AIM || boss.state == SS_SNIP) {   /* the scissor blades closing on you */
        float tx = sx(boss.targetx) + ox, k = boss.state == SS_SNIP ? 1 : clampf(1 - boss.timer / 0.75f, 0, 1);
        float open = boss.state == SS_SNIP ? 0.05f : 0.9f - 0.6f * k;
        for (int sd = -1; sd <= 1; sd += 2) {
            float a = -1.5708f + sd * open, bx = tx, by = FLOOR_Y + oy - 20, ex = bx + cosf(a) * 330, ey = by + sinf(a) * 330;
            draw_bar(bx, by, ex, ey, 26, 20, 18, 18, (Uint8)(150 + 105 * k));
            draw_bar(bx, by, ex, ey, 18, 200, 198, 192, (Uint8)(150 + 105 * k));
            draw_bar(bx + sd * 4, by, ex + sd * 4, ey, 4, 250, 250, 246, 255);
        }
        draw_ellipse(&A.disc, tx, FLOOR_Y + oy - 20, 14, 14, 20, 18, 18, 255);
        if (boss.state == SS_SNIP_AIM && (frame_no / 4) % 2) draw_text_col(&A.small, "!", tx, FLOOR_Y + oy - 380, 0.8f, 1, 255, 60, 40, 255);
    }
    if (boss.pose < 0) return;
    if (boss.state == SS_FLICK_OUT && (frame_no / 2) % 3 != 0) return;   /* skipping frames */
    Tex *t = &A.splicer[boss.pose];
    double ang = boss.state == BS_DEAD ? sinf(frame_no * 0.5f) * 6 : sinf(boss.t * 1.4f) * 2;
    Uint8 al = boss.state == SS_FLICK_OUT || boss.state == SS_FLICK_IN ? 170 : 255;
    draw_sprite(t, x, y + 10, 1, boss.face > 0, boss.flash ? 170 : 0, al, ang, 1.0f + 0.02f * sinf(boss.t * 5));
    if (boss.state == SS_FILM || boss.state == SS_FLICK_IN) draw_glow(&A.disc, x + boss.face * 30, y - 210, 50 + sinf(frame_no * 0.6f) * 10, 255, 250, 230, 220);   /* the projector eye */
}
/* ------------------------------------------------------------ boss: The Big Press (Show 4) */
static void update_boss_press(void) {
    float angry = boss.hp < boss.maxhp / 2 ? 0.7f : 1.0f;
    boss.t += DT; if (boss.flash > 0) boss.flash--;
    boss.timer -= DT; boss.bob = fabsf(sinf(boss.t * 4)) * -4;
    Player *t = nearest_player(boss.x, boss.y, NULL);
    float lo = ARENA_X + 260, hi = ARENA_X + SCREEN_W - 230;
    if (t && boss.state != BS_DEAD) boss.face = t->x < boss.x ? -1 : 1;
    if (boss.state != BS_DEAD && frame_no % 7 == 0) smoke(boss.x - boss.face * 120, boss.y - 300, 1, 160);
    switch (boss.state) {
    case BS_INTRO: boss.pose = 0; if (boss.timer <= 0) { boss.state = BS_IDLE; boss.timer = 1.0f; } break;
    case BS_IDLE:
        boss.pose = angry < 1 ? 2 : 0;
        if (t) { float want = t->x - boss.face * 400; boss.x += clampf(want - boss.x, -1, 1) * 70 / angry * DT; }
        boss.x = clampf(boss.x, lo, hi);
        if (frame_no % 30 == 0) { sfx("clank"); shake = fmaxf(shake, 2); }
        if (boss.timer <= 0) {
            int k = rand() % 3;
            if (k == 0) { boss.state = PS_STAMP_AIM; boss.slams = angry < 1 ? 3 : 2; boss.timer = 0.85f * angry; boss.targetx = t ? t->x : boss.x; sfx("charge"); }
            else if (k == 1) { boss.state = PS_SHEETS; boss.slams = angry < 1 ? 6 : 4; boss.timer = 0.35f; }
            else { boss.state = PS_ROLL_WARN; boss.timer = 0.8f * angry; sfx("charge"); }
            if (angry < 1 && !boss.summoned) { boss.summoned = 1; spawn(E_IMP, (int)((ARENA_X + 160) / TILE), 12); spawn(E_IMP, (int)((ARENA_X + 520) / TILE), 12); sfx("summon"); }
        }
        break;
    case PS_STAMP_AIM:  /* a giant printing plate hovers over you, then STAMPS */
        boss.pose = 1;
        if (boss.timer > 0.3f && t) boss.targetx += (t->x - boss.targetx) * 0.09f;
        if (boss.timer <= 0) { boss.state = PS_STAMP; boss.timer = 0.35f; shake = 16; hitstop = 3; sfx("slam"); sfx("boom");
            ring(boss.targetx, FLOOR_Y - 10, 170, 255, 220, 160); smoke(boss.targetx, FLOOR_Y - 10, 6, 60);
            for (int k = 0; k < 20; k++) part_k(PK_BLOOD, boss.targetx + (frand() - 0.5f) * 140, FLOOR_Y - 6, (frand() - 0.5f) * 600, -200 - frand() * 400, 1.2f, 5 + frand() * 5, 1500, 12, 8, 12, 0);
            splat(boss.targetx, FLOOR_Y + 2, 60, 1, 10, 8, 10); }
        break;
    case PS_STAMP:
        boss.pose = 1; boss_hit_player_box(boss.targetx, FLOOR_Y, 160, FLOOR_Y);
        if (boss.timer <= 0) { if (--boss.slams > 0) { boss.state = PS_STAMP_AIM; boss.timer = 0.6f * angry; } else { boss.state = BS_IDLE; boss.timer = 1.3f * angry; } }
        break;
    case PS_SHEETS:     /* spits razor-edged printed sheets that flutter at you */
        boss.pose = 1;
        if (boss.timer <= 0 && t) { float x0 = boss.x + boss.face * 120, y0 = boss.y - 210 + (frand() - 0.5f) * 60;
            Proj *q = proj(P_SHEET, x0, y0, boss.face * (380 + frand() * 140), 0, 1, 1); if (q) { q->r = 16; q->life = 3.5f; }
            sfx("throw"); boss.timer = 0.3f * angry; if (--boss.slams <= 0) { boss.state = BS_IDLE; boss.timer = 1.1f * angry; } }
        break;
    case PS_ROLL_WARN: boss.pose = 1; if ((frame_no / 5) % 2 == 0) smoke(boss.x + boss.face * 120, FLOOR_Y - 20, 1, 30);
        if (boss.timer <= 0) { boss.state = PS_ROLL; boss.sweepx = boss.x + boss.face * 140; sfx("splash"); shake = 8; } break;
    case PS_ROLL:       /* the ink roller rolls out across the floor: jump it */
        boss.pose = 0; boss.sweepx += boss.face * 640 / angry * DT;
        boss_hit_player_box(boss.sweepx, FLOOR_Y, 90, 70);
        if (frame_no % 2 == 0) part_k(PK_BLOOD, boss.sweepx, FLOOR_Y - 30, (frand() - 0.5f) * 200, -200 - frand() * 200, 0.8f, 6, 1200, 12, 8, 12, 0);
        if (frame_no % 4 == 0) splat(boss.sweepx, FLOOR_Y + 2, 26, 1, 10, 8, 10);
        if (boss.sweepx < ARENA_X - 60 || boss.sweepx > ARENA_X + SCREEN_W + 60) { boss.state = BS_IDLE; boss.timer = 1.1f * angry; }
        break;
    case BS_DEAD:
        if (boss.pose >= 0) boss.pose = 2;
        if (frame_no % 4 == 0) { sfx("boom"); spark_burst(boss.x + (frand() - 0.5f) * 260, boss.y - frand() * 300, 14, 255, 200, 110); smoke(boss.x, boss.y - 200, 2, 70);
            ink_burst(boss.x + (frand() - 0.5f) * 220, boss.y - frand() * 260, 10, 300);
            if (frame_no % 8 == 0) gore_part(rand() % 8, boss.x + (frand() - 0.5f) * 200, boss.y - 120 - frand() * 200, 700, 0); }
        if (!boss.summoned && boss.timer < 1.0f) {
            boss.summoned = 1; shatter(&A.press[2], boss.x, boss.y + 10, boss.face > 0, 5, 4, 850, 1); boss.pose = -1;
            for (int k = 0; k < 14; k++) part_k(PK_DEBRIS, boss.x, boss.y - 180, (frand() - 0.5f) * 1000, -300 - frand() * 600, 2.5f, 8 + frand() * 10, 1600, 60, 52, 46, 0);
            for (int i = 0; i < 50; i++) { float a = frand() * 6.283f, v = 300 + frand() * 600; part_k(PK_BLOOD, boss.x, boss.y - 180, cosf(a) * v, sinf(a) * v - 300, 1.5f, 5 + frand() * 8, 1500, 12, 8, 12, 0); }
            for (int i = 0; i < 12; i++) { Proj *q = proj(P_SHEET, boss.x, boss.y - 200, (frand() - 0.5f) * 700, 0, 0, 0); if (q) { q->life = 1.5f; q->enemy = 0; q->dmg = 0; } }
            ring(boss.x, boss.y - 180, 320, 255, 220, 160); shake = 24; hitstop = 10;
        }
        if (boss.timer <= 0 && scene == SC_PLAY) { scene = SC_CLEAR; scene_t = 0; music_play("mm_clear", 0); }
        break;
    }
    if (boss.state != BS_DEAD && boss.state != BS_INTRO) boss_hit_player_box(boss.x, boss.y, 240, 220);
}
static void draw_boss_press(float ox, float oy) {
    float x = sx(boss.x) + ox, y = boss.y + oy + boss.bob;
    if (boss.state == PS_STAMP_AIM || boss.state == PS_STAMP) {   /* the printing plate: a slab of iron type */
        float tx = sx(boss.targetx) + ox, k = boss.state == PS_STAMP ? 1 : clampf(1 - boss.timer / 0.85f, 0, 1);
        float bot = boss.state == PS_STAMP ? FLOOR_Y + oy : 70 + oy + sinf(frame_no * 0.8f) * 3;
        if (boss.state == PS_STAMP_AIM) { draw_ellipse(&A.disc, tx, FLOOR_Y + oy, 70 + 30 * k, 10 + 5 * k, 0, 0, 0, (Uint8)(110 + 130 * k));
            if ((frame_no / 4) % 2) draw_text_col(&A.small, "!", tx, FLOOR_Y + oy - 80, 0.8f, 1, 255, 60, 40, 255); }
        draw_bar(tx, 0, tx, bot - 60, 16, 50, 44, 40, 255);
        fill_rect((int)tx - 84, (int)bot - 64, 168, 64, 24, 20, 18, 255); fill_rect((int)tx - 80, (int)bot - 60, 160, 56, 90, 78, 64, 255);
        fill_rect((int)tx - 80, (int)bot - 60, 160, 6, 190, 150, 70, 255);
        for (int r = 0; r < 2; r++) for (int c = 0; c < 6; c++) fill_rect((int)tx - 72 + c * 25, (int)bot - 44 + r * 18, 18, 12, 30, 26, 22, 255);
    }
    if (boss.state == PS_ROLL) { float bx = sx(boss.sweepx) + ox;   /* the ink roller */
        draw_ellipse(&A.disc, bx, FLOOR_Y + oy - 36, 40, 40, 10, 8, 10, 255); draw_ellipse(&A.disc, bx - 10, FLOOR_Y + oy - 48, 10, 6, 120, 120, 130, 160);
        draw_bar(bx, FLOOR_Y + oy - 36, bx - boss.face * 60, FLOOR_Y + oy - 6, 18, 10, 8, 10, 255); }
    if (boss.pose < 0) return;
    Tex *t = &A.press[boss.pose];
    double ang = boss.state == BS_DEAD ? sinf(frame_no * 0.5f) * 5 : 0;
    draw_sprite(t, x, y + 10, 1, boss.face > 0, boss.flash ? 170 : 0, 255, ang, 1.0f + 0.015f * sinf(boss.t * 8));
}
/* ------------------------------------------------------------ boss: The Collector (Show 5) */
static void update_boss_collector(void) {
    float angry = boss.hp < boss.maxhp / 2 ? 0.7f : 1.0f;
    boss.t += DT; if (boss.flash > 0) boss.flash--;
    boss.timer -= DT; boss.bob = sinf(boss.t * 2.4f) * 8;
    Player *t = nearest_player(boss.x, boss.y, NULL);
    float lo = ARENA_X + 250, hi = ARENA_X + SCREEN_W - 220;
    if (t && boss.state != BS_DEAD) boss.face = t->x < boss.x ? -1 : 1;
    switch (boss.state) {
    case BS_INTRO: boss.pose = 0; if (boss.timer <= 0) { boss.state = BS_IDLE; boss.timer = 1.0f; } break;
    case BS_IDLE:
        boss.pose = angry < 1 ? 2 : 0;
        if (t) { float want = t->x - boss.face * 380; boss.x += clampf(want - boss.x, -1, 1) * 120 / angry * DT; }
        boss.x = clampf(boss.x, lo, hi);
        if (boss.timer <= 0) {
            int k = rand() % 3;
            if (k == 0) { boss.state = CS_STAB_AIM; boss.slams = angry < 1 ? 3 : 2; boss.timer = 0.75f * angry; boss.targetx = t ? t->x : boss.x; sfx("charge"); }
            else if (k == 1) { boss.state = CS_THROW; boss.slams = angry < 1 ? 6 : 4; boss.timer = 0.3f; }
            else { boss.state = CS_SUCK; boss.timer = 2.4f; sfx("roar1"); message("HE'S TRYING TO BAG YOU! RUN!", 1.4f); }
            if (angry < 1 && !boss.summoned) { boss.summoned = 1; spawn(E_SOLDIER, (int)((ARENA_X + 140) / TILE), 12); spawn(E_SOLDIER, (int)((ARENA_X + 480) / TILE), 12); sfx("summon"); }
        }
        break;
    case CS_STAB_AIM:   /* a giant umbrella spike hovers over you, then plunges */
        boss.pose = 1;
        if (boss.timer > 0.25f && t) boss.targetx += (t->x - boss.targetx) * 0.1f;
        if (boss.timer <= 0) { boss.state = CS_STAB; boss.timer = 0.3f; shake = 12; hitstop = 2; sfx("slam");
            ring(boss.targetx, FLOOR_Y - 10, 120, 255, 240, 200); spark_burst(boss.targetx, FLOOR_Y - 8, 16, 255, 240, 200);
            for (int k = 0; k < 6; k++) part_k(PK_DEBRIS, boss.targetx, FLOOR_Y - 6, (frand() - 0.5f) * 500, -200 - frand() * 300, 1.5f, 6, 1600, 110, 80, 50, 0); }
        break;
    case CS_STAB:
        boss.pose = 1; boss_hit_player_box(boss.targetx, FLOOR_Y, 90, FLOOR_Y);
        if (boss.timer <= 0) { if (--boss.slams > 0) { boss.state = CS_STAB_AIM; boss.timer = 0.5f * angry; } else { boss.state = BS_IDLE; boss.timer = 1.2f * angry; } }
        break;
    case CS_THROW:      /* pulls lost shoes out of the sack and hurls them */
        boss.pose = 1;
        if (boss.timer <= 0 && t) { float x0 = boss.x - boss.face * 40, y0 = boss.y - 330, dx = t->x - x0;
            Proj *q = proj(P_SHOE, x0, y0, clampf(dx / 1.0f, -560, 560) + (frand() - 0.5f) * 120, -620, 1, 1); if (q) { q->grav = 1300; q->r = 16; q->bounces = 2; q->life = 4; }
            sfx("throw"); boss.timer = 0.32f * angry; if (--boss.slams <= 0) { boss.state = BS_IDLE; boss.timer = 1.1f * angry; } }
        break;
    case CS_SUCK: {     /* opens the sack: everything gets dragged toward it. Run the other way */
        boss.pose = angry < 1 ? 2 : 0;
        float mx = boss.x - boss.face * 30, my = boss.y - 200;
        for (int i = 0; i < 2; i++) { float a = frand() * 6.283f, r = 300 + frand() * 200; part(mx + cosf(a) * r, my + sinf(a) * r * 0.6f, -cosf(a) * r * 2.2f, -sinf(a) * r * 1.3f, 0.45f, 4, 0, 220, 210, 190, 0); }
        if (frame_no % 10 == 0) sfx("summon");
        for (int k = 0; k < 2; k++) { Player *p = &pl[k]; if (!p->active || !p->alive || p->hidden) continue;
            float dir = boss.x < p->x ? -1 : 1, pull = 260 / angry * DT, vy0 = 0;
            move_box(&p->x, &p->y, &(float){dir * pull / DT}, &vy0, 40, 120, 0); }
        if (boss.timer <= 0) { boss.state = BS_IDLE; boss.timer = 1.0f * angry; }
        break; }
    case BS_DEAD:
        if (boss.pose >= 0) boss.pose = 2;
        if (frame_no % 4 == 0) { sfx("boom"); blood_spurt(boss.x + (frand() - 0.5f) * 160, boss.y - frand() * 300, frand() < 0.5f ? -1 : 1, 10, 700);
            for (int k = 0; k < 3; k++) { Proj *q = proj(P_SHOE, boss.x, boss.y - 300, (frand() - 0.5f) * 700, -500 - frand() * 300, 0, 0); if (q) { q->grav = 1300; q->bounces = 2; q->life = 2; } }
            if (frame_no % 8 == 0) gore_part(rand() % 12, boss.x + (frand() - 0.5f) * 200, boss.y - 120 - frand() * 200, 700, 1); }
        if (!boss.summoned && boss.timer < 1.0f) {
            boss.summoned = 1; shatter(&A.collector[2], boss.x, boss.y + 10, boss.face > 0, 4, 5, 850, 1); boss.pose = -1;
            for (int k = 0; k < 6; k++) gore_part(11, boss.x, boss.y - 200, 800, 1);
            gore_part(3, boss.x - 20, boss.y - 330, 900, 1); gore_part(3, boss.x + 20, boss.y - 330, 900, 1);
            blood_spurt(boss.x, boss.y - 250, -1, 60, 900); blood_spurt(boss.x, boss.y - 250, 1, 50, 800);
            for (int i = 0; i < 20; i++) part_k(PK_DEBRIS, boss.x, boss.y - 250, (frand() - 0.5f) * 1000, -300 - frand() * 600, 2.5f, 6 + frand() * 10, 1600, 120 + rand() % 120, 80 + rand() % 100, 50 + rand() % 80, 0);
            ring(boss.x, boss.y - 200, 300, 255, 220, 160); shake = 24; hitstop = 10;
        }
        if (boss.timer <= 0 && scene == SC_PLAY) { scene = SC_CLEAR; scene_t = 0; music_play("mm_clear", 0); }
        break;
    }
    if (boss.state != BS_DEAD && boss.state != BS_INTRO) boss_hit_player_box(boss.x, boss.y, 160, 300);
}
static void draw_boss_collector(float ox, float oy) {
    float x = sx(boss.x) + ox, y = boss.y + oy + boss.bob;
    if (boss.state == CS_STAB_AIM || boss.state == CS_STAB) {   /* a giant black umbrella spike */
        float tx = sx(boss.targetx) + ox, k = boss.state == CS_STAB ? 1 : clampf(1 - boss.timer / 0.75f, 0, 1);
        float tip = boss.state == CS_STAB ? FLOOR_Y + oy : 120 + oy + sinf(frame_no * 0.9f) * 4;
        if (boss.state == CS_STAB_AIM) { draw_ellipse(&A.disc, tx, FLOOR_Y + oy, 30 + 24 * k, 7 + 4 * k, 0, 0, 0, (Uint8)(110 + 130 * k));
            if ((frame_no / 4) % 2) draw_text_col(&A.small, "!", tx, FLOOR_Y + oy - 80, 0.8f, 1, 255, 60, 40, 255); }
        draw_bar(tx, 0, tx, tip - 140, 10, 90, 60, 30, 255);   /* handle */
        for (int i = 0; i < 6; i++) { float w = 70 - i * 11; fill_rect((int)(tx - w), (int)(tip - 140 + i * 20), (int)(w * 2), 22, 14, 12, 16, 255); }   /* folded canopy */
        draw_bar(tx, tip - 30, tx, tip, 6, 200, 200, 210, 255);   /* metal ferrule tip */
    }
    if (boss.pose < 0) return;
    Tex *t = &A.collector[boss.pose];
    double ang = boss.state == BS_DEAD ? sinf(frame_no * 0.5f) * 6 : sinf(boss.t * 1.2f) * 3;
    draw_sprite(t, x, y + 10, 1, boss.face > 0, boss.flash ? 170 : 0, 255, ang, 1.0f);
    if (boss.state == CS_SUCK) draw_glow(&A.disc, x - boss.face * 40, y - 220, 90 + sinf(frame_no * 0.5f) * 15, 40, 30, 50, 180);
}
/* ------------------------------------------------------------ final boss: Inky Unbound (Show 6) */
static void update_boss_unbound(void) {
    float angry = boss.hp < boss.maxhp / 2 ? 0.7f : 1.0f;
    boss.t += DT; if (boss.flash > 0) boss.flash--;
    boss.timer -= DT; boss.bob = sinf(boss.t * 1.6f) * 12;
    Player *t = nearest_player(boss.x, boss.y, NULL);
    if (t && boss.state != BS_DEAD) boss.face = t->x < boss.x ? -1 : 1;
    if (boss.state != BS_DEAD && frame_no % 3 == 0) part_k(PK_BLOOD, boss.x + (frand() - 0.5f) * 260, boss.y - 100 - frand() * 300, (frand() - 0.5f) * 60, 60, 1.2f, 4 + frand() * 5, 900, 12, 8, 12, 0);
    switch (boss.state) {
    case BS_INTRO: boss.pose = 0; if (boss.timer <= 0) { boss.state = BS_IDLE; boss.timer = 1.0f; } break;
    case BS_IDLE:
        boss.pose = angry < 1 ? 2 : 0;
        if (boss.timer <= 0) {
            int k = rand() % 3;
            if (k == 0) { boss.state = US_SLAM_AIM; boss.slams = angry < 1 ? 4 : 3; boss.timer = 0.8f * angry; boss.targetx = t ? t->x : boss.x; sfx("charge"); }
            else if (k == 1) { boss.state = US_RAIN; boss.timer = 2.6f; boss.slams = 0; sfx("thunder"); message("INK RAIN!", 1.0f); }
            else { boss.state = US_BEAM_WARN; boss.timer = 0.9f * angry; sfx("charge"); }
            if (angry < 1 && !boss.summoned) { boss.summoned = 1; spawn(E_SHADOW, (int)((ARENA_X + 120) / TILE), 12); spawn(E_SHADOW, (int)((ARENA_X + 420) / TILE), 12); sfx("summon"); message("HIS UNDERSTUDIES TAKE THE STAGE!", 1.5f); }
        }
        break;
    case US_SLAM_AIM:   /* an ink tentacle the size of a building hovers over you, then slams */
        boss.pose = 1;
        if (boss.timer > 0.28f && t) boss.targetx += (t->x - boss.targetx) * 0.1f;
        if (boss.timer <= 0) { boss.state = US_SLAM; boss.timer = 0.35f; shake = 18; hitstop = 3; sfx("slam"); sfx("boom");
            ring(boss.targetx, FLOOR_Y - 10, 180, 60, 40, 60); splat(boss.targetx, FLOOR_Y + 2, 70, 1, 10, 8, 10);
            for (int k = 0; k < 24; k++) part_k(PK_BLOOD, boss.targetx + (frand() - 0.5f) * 120, FLOOR_Y - 6, (frand() - 0.5f) * 700, -200 - frand() * 500, 1.3f, 5 + frand() * 6, 1500, 12, 8, 12, 0); }
        break;
    case US_SLAM:
        boss.pose = 1; boss_hit_player_box(boss.targetx, FLOOR_Y, 140, FLOOR_Y);
        if (boss.timer <= 0) { if (--boss.slams > 0) { boss.state = US_SLAM_AIM; boss.timer = 0.55f * angry; } else { boss.state = BS_IDLE; boss.timer = 1.2f * angry; } }
        break;
    case US_RAIN:       /* ink pours from the rafters around you */
        boss.pose = 0;
        if (frame_no % (angry < 1 ? 5 : 8) == 0) { float x = (t ? t->x : boss.x) + (frand() - 0.5f) * 700;
            Proj *q = proj(P_DRIP, clampf(x, ARENA_X + 40, ARENA_X + SCREEN_W - 40), 10, 0, 80, 1, 1); if (q) { q->grav = 1500; q->r = 12; } }
        if (boss.timer <= 0) { boss.state = BS_IDLE; boss.timer = 1.0f * angry; }
        break;
    case US_BEAM_WARN: boss.pose = 1; if (boss.timer <= 0) { boss.state = US_BEAM; boss.sweepx = boss.x + boss.face * 80; sfx("beam"); } break;
    case US_BEAM:       /* his eyes burn a line across the stage floor: jump it */
        boss.pose = 1; boss.sweepx += boss.face * 760 / angry * DT;
        boss_hit_player_box(boss.sweepx, FLOOR_Y, 70, 60);
        if (frame_no % 2 == 0) { part(boss.sweepx, FLOOR_Y - 8, (frand() - 0.5f) * 300, -200 - frand() * 200, 0.4f, 6, 600, 255, 180, 60, 1); if (frame_no % 6 == 0) splat(boss.sweepx, FLOOR_Y + 2, 14, 1, 30, 14, 6); }
        if (boss.sweepx < ARENA_X - 60 || boss.sweepx > ARENA_X + SCREEN_W + 60) { boss.state = BS_IDLE; boss.timer = 1.0f * angry; }
        break;
    case BS_DEAD:
        if (boss.pose >= 0) boss.pose = 2;
        if (frame_no % 3 == 0) { sfx("boom"); ink_burst(boss.x + (frand() - 0.5f) * 300, boss.y - frand() * 420, 14, 380);
            spark_burst(boss.x + (frand() - 0.5f) * 260, boss.y - frand() * 400, 10, 255, 170, 60);
            if (frame_no % 9 == 0) gore_part(rand() % 12, boss.x + (frand() - 0.5f) * 240, boss.y - 160 - frand() * 260, 800, 1); }
        if (!boss.summoned && boss.timer < 1.2f) {
            boss.summoned = 1; shatter(&A.unbound[2], boss.x, boss.y + 10, boss.face > 0, 6, 6, 950, 1); boss.pose = -1;
            for (int i = 0; i < 80; i++) { float a = frand() * 6.283f, v = 300 + frand() * 800; part_k(PK_BLOOD, boss.x, boss.y - 260, cosf(a) * v, sinf(a) * v - 300, 1.8f, 6 + frand() * 10, 1500, 12, 8, 12, 0); }
            for (int k = 0; k < 4; k++) ring(boss.x, boss.y - 260, 300 + k * 120, 255, 200, 120);
            blood_spurt(boss.x, boss.y - 300, -1, 60, 900); blood_spurt(boss.x, boss.y - 300, 1, 60, 900);
            shake = 30; hitstop = 14; negative_t = 0.4f;
        }
        if (boss.timer <= 0 && scene == SC_PLAY) { scene = SC_CLEAR; scene_t = 0; music_play("mm_clear", 0); }
        break;
    }
    if (boss.state != BS_DEAD && boss.state != BS_INTRO) boss_hit_player_box(boss.x, boss.y, 260, 300);
}
static void draw_boss_unbound(float ox, float oy) {
    float x = sx(boss.x) + ox, y = boss.y + oy + boss.bob;
    if (boss.state == US_SLAM_AIM || boss.state == US_SLAM) {   /* a colossal ink tentacle */
        float tx = sx(boss.targetx) + ox, k = boss.state == US_SLAM ? 1 : clampf(1 - boss.timer / 0.8f, 0, 1);
        float tip = boss.state == US_SLAM ? FLOOR_Y + oy : 150 + oy + sinf(frame_no * 0.7f) * 8;
        if (boss.state == US_SLAM_AIM) { draw_ellipse(&A.disc, tx, FLOOR_Y + oy, 60 + 30 * k, 10 + 5 * k, 0, 0, 0, (Uint8)(110 + 130 * k));
            if ((frame_no / 4) % 2) draw_text_col(&A.small, "!", tx, FLOOR_Y + oy - 80, 0.8f, 1, 255, 60, 40, 255); }
        for (int i = 0; i < 14; i++) { float yy = tip - i * 48, ww = 34 + i * 3, wob = sinf(frame_no * 0.2f + i * 0.6f) * 8;
            draw_ellipse(&A.disc, tx + wob, yy, ww, 34, 10, 8, 12, 255); }
        for (int i = 1; i < 12; i += 2) draw_ellipse(&A.disc, tx + sinf(frame_no * 0.2f + i * 0.6f) * 8 - boss.face * 18, tip - i * 48, 9, 7, 70, 60, 80, 255);   /* suckers */
        draw_ellipse(&A.disc, tx, tip - 10, 26, 18, 230, 225, 210, 255);   /* one of his old white gloves */
    }
    if (boss.state == US_BEAM) { float bx = sx(boss.sweepx) + ox, ex = x + boss.face * 40, ey = y - 330;
        draw_bar(ex, ey, bx, FLOOR_Y + oy - 6, 22, 255, 140, 30, 150); draw_bar(ex, ey, bx, FLOOR_Y + oy - 6, 8, 255, 230, 150, 255);
        draw_glow(&A.disc, bx, FLOOR_Y + oy - 10, 50, 255, 160, 60, 255); }
    if (boss.state == US_BEAM_WARN) draw_glow(&A.disc, x + boss.face * 40, y - 330, 40 + (0.9f - boss.timer) * 50, 255, 150, 40, 255);
    if (boss.pose < 0) return;
    Tex *t = &A.unbound[boss.pose];
    double ang = boss.state == BS_DEAD ? sinf(frame_no * 0.6f) * 6 : sinf(boss.t * 1.1f) * 2;
    draw_sprite(t, x, y + 30, 1, boss.face > 0, boss.flash ? 170 : 0, 255, ang, 1.0f + 0.02f * sinf(boss.t * 3));
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
            if (p->type == P_REEL && p->vy >= 0) { p->y = ty * TILE - 1;
                if (p->vy > 260 && p->bounces > 0) { p->bounces--; p->vy = -p->vy * 0.45f; sfx("clank"); spark_burst(p->x, p->y, 6, 255, 255, 240); }
                else { p->vy = 0; p->grav = 0; }
                continue; }
            if (p->type == P_PIZZA && p->vy >= 0 && p->bounces > 0) { p->bounces--; p->vy = -380; p->y = ty * TILE - 1; splat(p->x, ty * TILE, 10, 1, 170, 30, 20); continue; }
            if (p->type == P_CONFETTI) { for (int i = 0; i < 12; i++) part(p->x, p->y, (frand() - 0.5f) * 400, -100 - frand() * 300, 0.8f, 4, 900, 120 + rand() % 136, 80 + rand() % 176, 80 + rand() % 176, 0); spark_burst(p->x, p->y, 6, 255, 220, 120); }
            if (p->type == P_SHOE && p->vy >= 0 && p->bounces > 0) { p->bounces--; p->vy = -360; p->y = ty * TILE - 1; sfx("boing"); continue; }
            if (p->type == P_CORK) { spark_burst(p->x, p->y, 5, 230, 200, 150); }
            if (p->type == P_INKBLOB) { sfx("splat"); for (int i = 0; i < 10; i++) part_k(PK_BLOOD, p->x, p->y - 4, (frand() - 0.5f) * 400, -100 - frand() * 250, 1.0f, 4 + frand() * 4, 1400, 12, 8, 12, 0); splat(p->x, ty * TILE + 2, 22, 1, 10, 8, 10); }
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
        if (p->type == P_INKBLOB && frame_no % 3 == 0) part_k(PK_BLOOD, p->x, p->y, 0, 0, 0.8f, 3, 900, 12, 8, 12, 0);
        if (p->type == P_SHEET) { p->vy = sinf(p->life * 7 + p->x * 0.01f) * 220; }
        if (p->type == P_FILM && frame_no % 3 == 0) part(p->x, p->y, 0, 0, 0.3f, 6, 0, 200, 196, 186, 0);
        if (p->enemy) {
            for (int k = 0; k < 2; k++) { Player *q = &pl[k];
                if (q->active && q->alive && !q->hidden && overlap(q->x, q->y, 40, q->crouch ? 76 : 118, p->x, p->y + p->r, p->r * 2, p->r * 2)) {
                    hurt_player(q, p->x); if (p->type != P_NOTE) p->alive = 0; } }
        } else {
            for (int j = 0; j < nen && p->alive; j++) { Enemy *e = &en[j]; if (!e->alive || j == p->last_hit || ghost_faded(e) || (e->type == E_EEL && (e->state == 0 || e->state == 3))) continue;
                float w, h; enemy_box(e, &w, &h);
                if (overlap(e->x, e->y, w, h, p->x, p->y + p->r, p->r * 2, p->r * 2)) {
                    if (p->type == P_FLAME) e->burn = 2.2f;                      /* set them on fire */
                    if (p->type == P_RIVET) { ring(p->x, p->y, 50, 255, 170, 80); smoke(p->x, p->y, 2, 90); }
                    if (p->type == P_PLUNGER) { e->stun = 0.6f; sfx("boing"); message("THWONK!", 0.5f); }
                    enemy_damage(e, p->dmg); spark_burst(p->x, p->y, 7, 255, 240, 200);
                    if (p->type == P_FLAME) p->last_hit = j; else p->alive = 0; } }
            if (p->alive && boss.active && boss.state != BS_DEAD && boss.state != BS_INTRO && boss.state != SS_FLICK_OUT &&
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
                if (boss.hp <= 0) { boss.summoned = 0; boss.state = BS_DEAD; boss.timer = 3.5f; score += 10000; message(level_no == 5 ? "THE CURTAIN FALLS ON UNCLE INKY!" : level_no == 4 ? "CLAIM TICKET: VOID!" : level_no == 3 ? "STOP THE PRESSES!" : level_no == 2 ? "THE SPLICER GETS CUT!" : level_no == 1 ? "BIG CHEESE IS TOAST!" : "UNCLE INKY TAKES A BOW!", 3.0f); shake = 16; sfx("roar2"); hitstop = 18; }
                else if (boss.hp < boss.maxhp / 2 && boss.hp + p->dmg >= boss.maxhp / 2) { sfx("angry"); message(level_no == 5 ? "HIS HEART IS SHOWING! AIM FOR IT!" : level_no == 4 ? "HE WANTS IT ALL BACK!" : level_no == 3 ? "IT'S OVERHEATING!" : level_no == 2 ? "SHE'S UNSPOOLING!" : "HE'S STEAMED!", 1.5f); shake = 10; }
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
    if (scene == SC_END) {   /* ending: the first run rolls into the harder second run; beating that is the true ending */
        if (scene_t > 6.0f && (anyp & (B_START | B_JUMP | B_SHOOT))) {
            if (!loop_no) { Player keep[2]; memcpy(keep, pl, sizeof keep); int sc = score;
                loop_no = 1; level_no = 0; start_level(); score = sc; message("THE SECOND SHOW: EVERYTHING IS FASTER AND MEANER", 3.0f);
                for (int i = 0; i < 2; i++) if (keep[i].active) { pl[i].active = 1; pl[i].pad = keep[i].pad; pl[i].lives = keep[i].lives + 2; pl[i].weapon = keep[i].weapon; reset_player(&pl[i], 0); } }
            else { loop_no = 0; scene = SC_TITLE; music_play("mm_title", 1); }
        }
        return;
    }
    if (scene == SC_OVER || scene == SC_CLEAR) {
        if (scene_t > 2.0f && (anyp & (B_START | B_JUMP | B_SHOOT))) {
            if (scene == SC_CLEAR && level_no + 1 >= NUM_LEVELS) { scene = SC_END; scene_t = 0; music_play("mm_ending", 0); return; }   /* the finale */
            if (scene == SC_CLEAR && level_no + 1 < NUM_LEVELS) {   /* on to the next show, keeping your crew */
                Player keep[2]; memcpy(keep, pl, sizeof keep); int sc = score;
                level_no++; start_level(); score = sc;
                for (int i = 0; i < 2; i++) if (keep[i].active) { pl[i].active = 1; pl[i].pad = keep[i].pad; pl[i].lives = keep[i].lives; pl[i].weapon = keep[i].weapon; reset_player(&pl[i], 0); }
            } else { loop_no = 0; scene = SC_TITLE; music_play("mm_title", 1); }
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
        static int avail[NUM_LEVELS][6] = {{-1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1}};
        int z = zone_of(lead->x);
        if (avail[level_no][z] < 0) avail[level_no][z] = music_available(want);
        music_play(avail[level_no][z] ? want : LEVELS[level_no].music[0], 1);
    }
    for (int i = 0; i < 2; i++) update_player(&pl[i], &pads[pl[i].pad]);
    op_input(&pads[op_pad]); auto_camera();
    if (negative_t > 0) negative_t -= DT;
    update_enemies(); update_fiend(); update_ovens(); update_saws(); update_crushers_vents(); update_blocks_traps(); update_tide(); update_boss(); update_proj();

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
        if (c == '#' && level_no == 3) {   /* ink works: riveted iron */
            int top = ty == 0 || map[ty - 1][tx] != '#';
            fill_rect((int)x, (int)y, TILE, TILE, 54, 46, 40, 255);
            fill_rect((int)x, (int)y, 2, TILE, 30, 26, 22, 255); fill_rect((int)x, (int)y + TILE - 2, TILE, 2, 30, 26, 22, 255);
            draw_ellipse(&A.disc, x + 8, y + 8, 3, 3, 130, 110, 80, 255); draw_ellipse(&A.disc, x + 40, y + 8, 3, 3, 130, 110, 80, 255);
            if (top && ty > 0) { fill_rect((int)x, (int)y, TILE, 6, 120, 100, 70, 255); if ((tx * 13) % 5 == 0) draw_ellipse(&A.disc, x + 24, y + 9, 6, 10 + (tx % 3) * 4, 10, 8, 10, 255); }
        } else if (c == '#' && level_no == 2) {   /* old cartoon film: grey */
            int top = ty == 0 || map[ty - 1][tx] != '#';
            fill_rect((int)x, (int)y, TILE, TILE, 70, 68, 66, 255);
            fill_rect((int)x, (int)y + 23, TILE, 2, 36, 34, 34, 255);
            fill_rect((int)x + ((tx * 7 + ty * 3) % 3) * 16, (int)y, 2, TILE, 40, 38, 38, 255);
            if (top) { fill_rect((int)x, (int)y, TILE, 6, 170, 166, 160, 255); fill_rect((int)x, (int)y + 6, TILE, 3, 16, 14, 14, 255); }
        } else if (c == 'F') {   /* a frame of film you can stand on... while it's in the picture */
            float ph = film_phase(tx); int on = film_on(tx), warn = on && ph > 1.6f && (frame_no / 3) % 2;
            if (on && !warn) {
                fill_rect((int)x, (int)y, TILE, 16, 22, 20, 20, 255);
                fill_rect((int)x + 2, (int)y + 4, TILE - 4, 8, 180, 176, 168, 255);
                for (int k = 0; k < 3; k++) { fill_rect((int)x + 4 + k * 16, (int)y + 1, 7, 3, 240, 236, 226, 255); fill_rect((int)x + 4 + k * 16, (int)y + 12, 7, 3, 240, 236, 226, 255); }
                fill_rect((int)x + TILE - 2, (int)y, 2, 16, 10, 10, 10, 255);
            } else {
                for (int k = 0; k < 4; k++) fill_rect((int)x + k * 12, (int)y + 6, 6, 3, 200, 196, 186, on ? 160 : 55);
                if (!on && ph > 2.9f && (frame_no / 3) % 2) fill_rect((int)x, (int)y + 4, TILE, 8, 220, 216, 206, 90);
            }
        } else if (c == 'U') {   /* wobbly toy block */
            float ct = crumble_t[ty][tx], wob = ct > 0 ? sinf(frame_no * 1.7f) * 3 * (ct / 0.55f + 0.3f) : 0;
            Uint8 br = (Uint8)(150 + (tx * 37) % 100), bg_ = (Uint8)(60 + (tx * 53) % 140), bb = (Uint8)(50 + (tx * 71) % 120);
            fill_rect((int)(x + wob), (int)y, TILE, TILE - 4, 30, 20, 14, 255); fill_rect((int)(x + wob) + 3, (int)y + 3, TILE - 6, TILE - 10, br, bg_, bb, 255);
            fill_rect((int)(x + wob) + 9, (int)y + 9, TILE - 18, TILE - 22, (Uint8)(br * 0.8f), (Uint8)(bg_ * 0.8f), (Uint8)(bb * 0.8f), 255);
            draw_text_col(&A.small, (const char[]){(char)('A' + (tx * 7 + ty) % 26), 0}, x + wob + 24, y + 10, 0.5f, 1, 250, 240, 210, 255);
        } else if (c == 'u') {   /* a crumbled block, stacking itself back up */
            if (regrow_t[ty][tx] < 0.8f && (frame_no / 3) % 2) fill_rect((int)x + 3, (int)y + 3, TILE - 6, TILE - 10, 220, 200, 160, 70);
        } else if (c == 'M') {   /* giant mousetrap */
            float tt = trap_t[tx]; int snapped = tt >= 0.28f && tt < 2.0f;
            fill_rect((int)x - 10, (int)y + 34, TILE + 20, 14, 40, 26, 14, 255); fill_rect((int)x - 8, (int)y + 36, TILE + 16, 10, 170, 120, 70, 255);
            draw_ellipse(&A.disc, x + 24, y + 32, 9, 7, 255, 210, 80, 255);   /* cheese */
            if (snapped) { draw_bar(x - 6, y + 34, x + 54, y + 34, 5, 200, 200, 210, 255); draw_bar(x - 6, y + 34, x - 6, y + 24, 5, 200, 200, 210, 255); }
            else { float a = tt > 0 ? 1.2f + tt * 3 : 1.5708f;   /* the bar, cocked back */
                draw_bar(x - 6, y + 40, x - 6 + cosf(a) * 60, y + 40 - sinf(a) * 60, 5, 200, 200, 210, 255);
                if (tt > 0 && (frame_no / 2) % 2) draw_text_col(&A.small, "!", x + 24, y - 10, 0.6f, 1, 255, 80, 40, 255); }
        } else if (c == 'X' && level_no == 5) {   /* fly loft: a sandbag on a rope */
            float b = crusher_bottom(tx) + oy, cx = x + 24, ph = crush_phase(tx);
            draw_bar(cx, y, cx, b - 70, 3, 150, 120, 80, 255);
            draw_ellipse(&A.disc, cx, b - 36, 46, 38, 40, 28, 16, 255); draw_ellipse(&A.disc, cx, b - 38, 42, 34, 160, 128, 80, 255);
            draw_bar(cx - 14, b - 72, cx + 14, b - 72, 6, 120, 90, 50, 255);
            draw_text_col(&A.small, "50", cx, b - 50, 0.5f, 1, 90, 60, 30, 255);
            if (lit[zone_of(tx * TILE)] <= 0 && ph > 1.6f && ph < 2.0f) { float k = (ph - 1.6f) / 0.4f; draw_ellipse(&A.disc, cx, FLOOR_Y + oy, 46 * (0.6f + 0.4f * k), 9, 0, 0, 0, (Uint8)(80 + 140 * k));
                if ((frame_no / 4) % 2) draw_text_col(&A.small, "!", cx, FLOOR_Y + oy - 70, 0.7f, 1, 255, 80, 40, 255); }
        } else if (c == 'X') {   /* ceiling crusher: shaft + riveted iron head */
            float b = crusher_bottom(tx) + oy, cx = x + 24, ph = crush_phase(tx);
            fill_rect((int)cx - 10, (int)y, 20, (int)(b - 40 - y), 70, 60, 50, 255); fill_rect((int)cx - 4, (int)y, 4, (int)(b - 40 - y), 150, 130, 100, 255);
            fill_rect((int)cx - 50, (int)b - 44, 100, 44, 30, 24, 20, 255); fill_rect((int)cx - 46, (int)b - 40, 92, 36, 96, 84, 70, 255);
            fill_rect((int)cx - 46, (int)b - 12, 92, 8, 60, 50, 40, 255);
            for (int k = 0; k < 4; k++) draw_ellipse(&A.disc, cx - 34 + k * 23, b - 30, 4, 4, 200, 170, 110, 255);
            if (lit[zone_of(tx * TILE)] <= 0 && ph > 1.6f && ph < 2.0f) {   /* warning shadow on the floor */
                float k = (ph - 1.6f) / 0.4f; draw_ellipse(&A.disc, cx, FLOOR_Y + oy, 50 * (0.6f + 0.4f * k), 9, 0, 0, 0, (Uint8)(80 + 140 * k));
                if ((frame_no / 4) % 2) draw_text_col(&A.small, "!", cx, FLOOR_Y + oy - 70, 0.7f, 1, 255, 80, 40, 255); }
            if (lit[zone_of(tx * TILE)] > 0) draw_text_col(&A.small, "OFF", cx, b + 6, 0.4f, 1, 160, 220, 255, 255);
        } else if (c == 'V') {   /* steam vent grate */
            fill_rect((int)x + 2, (int)y + 34, TILE - 4, 14, 30, 26, 22, 255); fill_rect((int)x + 4, (int)y + 36, TILE - 8, 10, 120, 104, 80, 255);
            for (int k = 0; k < 5; k++) fill_rect((int)x + 7 + k * 8, (int)y + 37, 3, 8, 30, 26, 22, 255);
            if (vent_on(tx)) draw_glow(&A.disc, x + 24, y + 20, 40, 255, 255, 250, 140);
            else if (fmodf(scene_t + tx * 0.7f, 2.6f) > 2.3f && (frame_no / 4) % 2) draw_text_col(&A.small, "^", x + 24, y + 4, 0.6f, 1, 255, 255, 255, 200);
        } else if (c == 'S') {   /* sprocket saw */
            float cy = saw_y(tx, ty) + oy, cx = x + 24, a = frame_no * 0.35f;
            if (ty < 12) draw_bar(cx, ty * TILE + oy - 60, cx, cy, 3, 40, 38, 38, 255);
            for (int k = 0; k < 10; k++) { float b = a + k * 0.628f; draw_ellipse(&A.disc, cx + cosf(b) * 32, cy + sinf(b) * 32, 7, 7, 30, 28, 28, 255); }
            draw_ellipse(&A.disc, cx, cy, 33, 33, 30, 28, 28, 255); draw_ellipse(&A.disc, cx, cy, 28, 28, 190, 186, 178, 255);
            for (int k = 0; k < 10; k++) { float b = a + k * 0.628f; draw_ellipse(&A.disc, cx + cosf(b) * 32, cy + sinf(b) * 32, 4, 4, 225, 222, 214, 255); }
            for (int k = 0; k < 4; k++) { float b = -a * 0.5f + k * 1.571f; draw_ellipse(&A.disc, cx + cosf(b) * 15, cy + sinf(b) * 15, 6, 6, 30, 28, 28, 255); }
            draw_ellipse(&A.disc, cx, cy, 6, 6, 20, 18, 18, 255);
        } else if (c == '#') {
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
    case E_GHOST: {
        int faded = ghost_faded(e); Uint8 al = faded ? (Uint8)(40 + 30 * ((frame_no / 3) % 2)) : 235;
        draw_sprite(et, x, y + 2, 1, e->face > 0, fl, al, sinf(e->t * 3) * 4, 1);
        break; }
    case E_BOMB: {
        float sw = e->state == 1 ? 1.0f + (0.8f - e->timer) * 0.35f : 1.0f;
        if (e->state == 1 && (frame_no / 3) % 2) fl = 170;
        draw_sprite(et, x, y + 2, sw, e->face > 0, fl, 255, 0, e->land > 0 ? 0.85f : 1.0f);
        break; }
    case E_EEL: {
        if (e->state == 0) { draw_ellipse(&A.disc, x, y - 2, 26, 6, 30, 26, 30, 200); break; }
        draw_sprite(et, x, y + 14, 1, e->face > 0, fl, 255, 0, 1);
        draw_ellipse(&A.disc, x, y - 2, 40, 9, 10, 8, 10, 255);   /* the splash ring it rises out of */
        break; }
    case E_SHADOW: {   /* the hero's own sprite, drawn as living ink with white eyes */
        if (!et || !et->tex) break;
        float w = et->w, h = et->h; draw_tex_tint(et, x - w / 2, y + 2 - h, w, h, e->face < 0, 18, 12, 22, 245, 0);
        if (e->flash) draw_tex_tint(et, x - w / 2, y + 2 - h, w, h, e->face < 0, 255, 255, 255, 120, 0);
        draw_ellipse(&A.disc, x + e->face * 6 - 6, y - 118, 4, 5, 255, 255, 255, 255); draw_ellipse(&A.disc, x + e->face * 6 + 6, y - 118, 4, 5, 255, 255, 255, 255);
        if (frame_no % 4 == 0) part_k(PK_BLOOD, e->x + (frand() - 0.5f) * 30, e->y - frand() * 120, 0, 40, 0.8f, 3, 900, 12, 8, 12, 0);
        break; }
    default:   /* goats, meatballs, slices, gloves, imps, workers */
        draw_sprite(et, x, y + 2, 1, e->face > 0, fl, 255, 0, 1);
        break;
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
        case P_FILM: { float a = atan2f(p->vy, p->vx), ca = cosf(a), sa = sinf(a), wv = sinf(frame_no * 0.5f) * 6;
            draw_bar(x - ca * 34 - sa * wv, y - sa * 34 + ca * wv, x + ca * 10, y + sa * 10, 16, 20, 18, 18, 255);
            draw_bar(x - ca * 32 - sa * wv, y - sa * 32 + ca * wv, x + ca * 8, y + sa * 8, 9, 190, 186, 176, 255);
            for (int k = 0; k < 3; k++) draw_ellipse(&A.disc, x - ca * (k * 13) - sa * 7, y - sa * (k * 13) + ca * 7, 2, 2, 250, 246, 236, 255);
            break; }
        case P_REEL: { float a = p->x * 0.04f; float cy = y - 30 + 1;
            draw_ellipse(&A.disc, x, cy, 32, 32, 20, 18, 18, 255); draw_ellipse(&A.disc, x, cy, 27, 27, 150, 146, 140, 255);
            draw_ellipse(&A.disc, x, cy, 18, 18, 60, 58, 56, 255);
            for (int k = 0; k < 5; k++) { float b = a + k * 1.2566f; draw_ellipse(&A.disc, x + cosf(b) * 16, cy + sinf(b) * 16, 6, 6, 20, 18, 18, 255); }
            draw_ellipse(&A.disc, x, cy, 5, 5, 220, 216, 206, 255);
            fill_rect((int)x - 26, FLOOR_Y + (int)oy - 4, 52, 4, 0, 0, 0, 90); break; }
        case P_CORK: draw_ellipse(&A.disc, x, y, 11, 8, 60, 36, 18, 255); draw_ellipse(&A.disc, x, y, 9, 6, 200, 150, 90, 255);
            draw_ellipse(&A.disc, x - p->vx * 0.01f, y, 3, 5, 150, 100, 60, 255); break;
        case P_SHOE: { float a = frame_no * 0.3f * (p->vx > 0 ? 1 : -1), ca = cosf(a), sa = sinf(a);
            draw_bar(x - ca * 18, y - sa * 18, x + ca * 18, y + sa * 18, 22, 30, 18, 10, 255); draw_bar(x - ca * 16, y - sa * 16, x + ca * 14, y + sa * 14, 16, 120, 70, 40, 255);
            draw_ellipse(&A.disc, x - ca * 10 - sa * 8, y - sa * 10 + ca * 8, 8, 8, 30, 18, 10, 255); break; }
        case P_INKBLOB: draw_ellipse(&A.disc, x, y, 14, 12, 10, 8, 10, 255); draw_ellipse(&A.disc, x - 4, y - 4, 4, 3, 160, 160, 170, 200);
            draw_ellipse(&A.disc, x - p->vx * 0.02f, y - p->vy * 0.02f, 8, 7, 10, 8, 10, 255); break;
        case P_SHEET: { float a = frame_no * 0.5f + i, ca = cosf(a) * 26, sa = sinf(a) * 26, cb = -sinf(a) * 18, sb = cosf(a) * 18;
            draw_bar(x - ca, y - sa, x + ca, y + sa, 36, 30, 26, 22, 255); draw_bar(x - ca, y - sa, x + ca, y + sa, 30, 238, 228, 205, 255);
            for (int k = -1; k <= 1; k++) draw_bar(x - ca * 0.7f + cb * k * 0.4f, y - sa * 0.7f + sb * k * 0.4f, x + ca * 0.7f + cb * k * 0.4f, y + sa * 0.7f + sb * k * 0.4f, 2, 40, 34, 30, 255);
            break; }
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
static void draw_boss_splicer(float ox, float oy);
static void draw_boss_press(float ox, float oy);
static void draw_boss_collector(float ox, float oy);
static void draw_boss_unbound(float ox, float oy);
static void draw_boss(float ox, float oy) {
    if (!boss.active) return;
    if (level_no == 5) { draw_boss_unbound(ox, oy); return; }
    if (level_no == 4) { draw_boss_collector(ox, oy); return; }
    if (level_no == 3) { draw_boss_press(ox, oy); return; }
    if (level_no == 2) { draw_boss_splicer(ox, oy); return; }
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
    if (loop_no) draw_text_col(&A.small, "2ND SHOW", SCREEN_W / 2, py + 62, 0.4f, 1, 255, 90, 70, 230);
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
    Tex *bgt = (level_no == 5 && A.bg6.tex) ? &A.bg6 : (level_no == 4 && A.bg5.tex) ? &A.bg5 : (level_no == 3 && A.bg4.tex) ? &A.bg4 : (level_no == 2 && A.bg3.tex) ? &A.bg3 : (level_no == 1 && A.bg2.tex) ? &A.bg2 : &A.bg;
    if (bgt->tex) {
        float k = (bgt == &A.bg3 || bgt == &A.bg4 || bgt == &A.bg5) ? 1.12f : bgt == &A.bg6 ? 1.03f : 1.0f;   /* the cartoon reel's painted floor sits higher: scale it down to ours */
        float par = camx * 0.35f; float w = bgt->w * k;
        float off = -fmodf(par, w);
        for (float x = off; x < SCREEN_W; x += w) draw_tex(bgt, x + ox * 0.3f, 0, w, SCREEN_H * k, 0, 255);
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

static void draw_ending(void) {
    float t = scene_t;
    fill_rect(0, 0, SCREEN_W, SCREEN_H, 20, 6, 6, 255);
    for (int i = 0; i < 16; i++) {   /* the red curtains close in from both sides */
        float k = fminf(1, t / 2.5f), w = SCREEN_W * 0.5f * k, fx = i * (w / 8);
        Uint8 sh = (Uint8)(110 + 40 * sinf(i * 1.3f));
        if (i < 8) fill_rect((int)fx, 0, (int)(w / 8) + 1, SCREEN_H, sh, 18, 20, 255);
        else fill_rect((int)(SCREEN_W - w + (i - 8) * (w / 8)), 0, (int)(w / 8) + 1, SCREEN_H, sh, 18, 20, 255);
    }
    fill_rect(0, 0, SCREEN_W, 60, 90, 60, 20, 255); fill_rect(0, 56, SCREEN_W, 6, 200, 160, 60, 255);
    if (t < 2.5f) return;
    Uint8 a = (Uint8)fminf(255, (t - 2.5f) * 200);
    if (!loop_no) {
        draw_text_col(&A.font, "THE CURTAIN FALLS", SCREEN_W / 2, 120, 1.3f, 1, 255, 230, 160, a);
        draw_text_col(&A.small, "UNCLE INKY IS FINALLY AT REST...", SCREEN_W / 2, 200, 0.7f, 1, 240, 210, 170, a);
        if (t > 4.0f) draw_text_col(&A.small, "...BUT THE PROJECTOR IS STILL RUNNING.", SCREEN_W / 2, 236, 0.7f, 1, 255, 120, 90, (Uint8)fminf(255, (t - 4.0f) * 200));
    } else {
        draw_text_col(&A.font, "TRUE ENDING", SCREEN_W / 2, 120, 1.3f, 1, 255, 240, 180, a);
        draw_text_col(&A.small, "THE LIGHTS COME UP. THE INK DRAINS AWAY.", SCREEN_W / 2, 200, 0.7f, 1, 240, 220, 180, a);
        draw_text_col(&A.small, "THE NIGHT CREW CLOCKS OUT. THE SHOW IS OVER FOR GOOD.", SCREEN_W / 2, 236, 0.7f, 1, 240, 220, 180, a);
    }
    { char b[48]; snprintf(b, sizeof b, "FINAL SCORE %06d", score); draw_text_col(&A.font, b, SCREEN_W / 2, 300, 0.8f, 1, 255, 240, 210, a); }
    static const char *credits[] = {"MIDNIGHT MATINEE", "A GAME BY JUNIOR GOMEZ", "SIX SHOWS - SIX BOSSES",
        "ART, MUSIC AND SOUND MADE WITH RUNWAY", "BUILT WITH CLAUDE", "THANKS FOR WATCHING THE SHOW"};
    for (int i = 0; i < 6; i++) { float ct = t - 4.5f - i * 0.6f; if (ct <= 0) continue;
        draw_text_col(&A.small, credits[i], SCREEN_W / 2, 380 + i * 34, i == 0 ? 0.75f : 0.55f, 1, 255, 220, 160, (Uint8)fminf(255, ct * 300)); }
    if (A.inky[0].tex && !loop_no && t > 5) { Tex *k = &A.inky[0]; float s = 0.35f;
        draw_tex_tint(k, SCREEN_W - k->w * s - 40, SCREEN_H - k->h * s - 30, k->w * s, k->h * s, 0, 60, 50, 50, (Uint8)(120 + 60 * sinf(t * 3)), 0); }
    if (t > 6.0f) draw_text_col(&A.small, loop_no ? "PRESS A TO RETURN TO THE TITLE" : "PRESS A FOR THE SECOND SHOW (HARDER - THE TRUE ENDING AWAITS)", SCREEN_W / 2, 650, 0.55f, 1, 255, 240, 200, (Uint8)(150 + 100 * sinf(t * 4)));
}

void game_draw(void) {
    SDL_SetRenderDrawColor(R, 0, 0, 0, 255); SDL_RenderClear(R);
    if (scene == SC_TITLE) { draw_title(); draw_post(); return; }
    if (scene == SC_END) { draw_ending(); draw_post(); return; }
    float ox = shake > 0 ? (frand() - 0.5f) * shake * 2 : 0, oy = shake > 0 ? (frand() - 0.5f) * shake * 2 : 0;
    draw_bg(ox);
    draw_boss(ox, oy);
    draw_tiles(ox, oy);
    draw_splats(ox, oy);
    if (level_no == 5) {   /* the flooded orchestra pit */
        float x0 = sx(ZONE_X0[2] * TILE) + ox, x1 = sx(ZONE_X0[3] * TILE) + ox, surf = tide_surface() + oy;
        if (x1 > 0 && x0 < SCREEN_W && surf < FLOOR_Y + 20) {
            fill_rect((int)x0, (int)surf, (int)(x1 - x0), (int)(SCREEN_H - surf), 8, 6, 10, 235);
            for (float wx = x0; wx < x1; wx += 46) draw_ellipse(&A.disc, wx + 23, surf + sinf(frame_no * 0.08f + wx * 0.05f) * 4, 30, 9, 14, 10, 16, 255);
            for (float wx = x0; wx < x1; wx += 92) draw_ellipse(&A.disc, wx + 30, surf - 2, 8, 3, 120, 110, 130, 140);
        }
    }
    for (int i = 0; i < nen; i++) if (en[i].alive) { float x = sx(en[i].x); if (x > -200 && x < SCREEN_W + 200) draw_enemy(&en[i], ox, oy); }
    for (int i = 0; i < 2; i++) draw_player(&pl[i], ox, oy);
    draw_gibs(ox, oy);
    draw_fiend(ox, oy);
    draw_proj(ox, oy);
    draw_parts(ox, oy);
    if (negative_t > 0) {   /* the film skips: flashes of overexposed white and black frames */
        int f = (frame_no / 2) % 3;
        if (f == 0) fill_rect(0, 0, SCREEN_W, SCREEN_H, 250, 248, 240, 150); else if (f == 1) fill_rect(0, 0, SCREEN_W, SCREEN_H, 0, 0, 0, 120);
        for (int k = 0; k < 4; k++) fill_rect((int)(frand() * SCREEN_W), 0, 2 + rand() % 3, SCREEN_H, 255, 255, 255, 120);
    }
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
            else draw_text_col(&A.small, "THE FINAL CURTAIN...", SCREEN_W / 2, 400, 0.6f, 1, 240, 210, 170, 255);
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
            else if (c == 'U') PR(x, y, w, w * 0.8f, 220, 150, 90, 255);
            else if (c == 'M') PR(x, y + w * 0.6f, w, w * 0.4f, 230, 200, 120, 255);
            else if (c == 'X') PR(x - w * 0.5f, oy + (crusher_bottom(tx) - 44) * s, w * 2, 44 * s, 200, 120, 80, 255);
            else if (c == 'V') PR(x, y + w * 0.6f, w, w * 0.4f, vent_on(tx) ? 240 : 140, vent_on(tx) ? 240 : 140, vent_on(tx) ? 240 : 140, 255);
            else if (c == 'F') { if (film_on(tx)) PR(x, y, w, w * 0.3f + 1, 200, 200, 190, 255); }
            else if (c == 'S') PR(x + w * 0.15f, oy + (saw_y(tx, ty) - 24) * s, w * 0.7f, w * 0.7f, 230, 80, 60, 255);
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

/* ---- test watchdog (MM_CHECK): reports impossible states during scripted runs ---- */
void game_check(long frame) {
    static int stuck[2], fallen[2], reported = 0;
    if (reported > 40) return;
    #define REPORT(...) do { printf("CHECK f%ld scene%d show%d loop%d: ", frame, scene, level_no + 1, loop_no); printf(__VA_ARGS__); printf("\n"); reported++; } while (0)
    if (scene != SC_PLAY) { stuck[0] = stuck[1] = fallen[0] = fallen[1] = 0; return; }
    for (int i = 0; i < 2; i++) {
        Player *p = &pl[i]; if (!p->active) continue;
        if (!isfinite(p->x) || !isfinite(p->y) || !isfinite(p->vx) || !isfinite(p->vy)) REPORT("P%d non-finite pos/vel", i + 1);
        if (!p->alive) { stuck[i] = fallen[i] = 0; continue; }
        if (p->x < 0 || p->x > LW * TILE) REPORT("P%d outside level x=%.0f", i + 1, p->x);
        if (p->y > LH * TILE + 96) { if (++fallen[i] == 90) REPORT("P%d fell below the world for 1.5s without dying y=%.0f x=%.0f", i + 1, p->y, p->x); } else fallen[i] = 0;
        int tx = (int)(p->x / TILE), ty = (int)((p->y - 30) / TILE);
        if (solid_at(tx, ty) && ty >= 0) { if (++stuck[i] == 60) REPORT("P%d inside a wall for 1s at tile %d,%d (%c)", i + 1, tx, ty, map[ty][tx]); } else stuck[i] = 0;
        if (p->lives < 0 || p->lives > 99) REPORT("P%d lives=%d", i + 1, p->lives);
    }
    int alive = 0;
    for (int i = 0; i < nen; i++) { Enemy *e = &en[i]; if (!e->alive) continue; alive++;
        if (!isfinite(e->x) || !isfinite(e->y)) REPORT("enemy %d type %d non-finite", i, e->type);
        else if (e->y > LH * TILE + 600) REPORT("enemy %d type %d fell out of the world y=%.0f (never cleaned up)", i, e->type, e->y); }
    if (nen >= MAXE) { static long last; if (frame - last > 600) { REPORT("enemy table full (%d slots, %d alive)", nen, alive); last = frame; } }
    if (!isfinite(boss.hp + 0.0f)) REPORT("boss hp non-finite");
    #undef REPORT
}
