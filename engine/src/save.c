#include "save.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define SAVE_MAGIC 0x4D4D5356u   /* "MMSV" */
#define SAVE_VERSION 3   /* 2: the secret ninth show; 3: Ticket Booth, Monster Book, new settings (older files are converted) */
#define V2_SIZE 156       /* a version 2 file: everything up to spare[7] */

SaveData save;
int save_slot;
static int write_failed;

/* Wii U: wut mounts the SD card at fs:/vol/external01 on start-up. PC: next to the game, or MM_SAVE (tests).
   Files: save1.dat .. save3.dat (one per slot) and settings.dat (shared). The very first build kept one save.dat:
   it becomes slot 1 the first time slot 1 is read. */
static const char *save_dir(void) {
#ifdef __WIIU__
    return "fs:/vol/external01/wiiu/midnight_matinee";
#else
    return ".";
#endif
}
static int enabled(void) {
#ifndef __WIIU__
    if (getenv("MM_HEADLESS") && !getenv("MM_SAVE")) return 0;   /* tests start from a clean slate unless they ask */
#endif
    return 1;
}
/* slot -1 = settings, -2 = the old single save */
static void path(char *out, size_t n, int slot, const char *suffix) {
    const char *custom = getenv("MM_SAVE");
    if (custom) {
        if (slot == -1) snprintf(out, n, "%s.settings%s", custom, suffix);
        else if (slot <= 0) snprintf(out, n, "%s%s", custom, suffix);   /* slot 1 (and the legacy file) keep the plain name */
        else snprintf(out, n, "%s.%d%s", custom, slot + 1, suffix);
        return;
    }
    if (slot == -1) snprintf(out, n, "%s/settings.dat%s", save_dir(), suffix);
    else if (slot == -2) snprintf(out, n, "%s/save.dat%s", save_dir(), suffix);
    else snprintf(out, n, "%s/save%d.dat%s", save_dir(), slot + 1, suffix);
}

static void defaults(SaveData *d) {
    memset(d, 0, sizeof *d);
    d->magic = SAVE_MAGIC; d->version = SAVE_VERSION;
    d->music_vol = 8; d->sfx_vol = 8; d->rumble = 1; d->layout = 0; d->shake = 1; d->difficulty = 1; d->subtitles = 1;
}
static void sanitize(SaveData *d) {   /* never trust the file blindly */
    for (int r = 0; r < 2; r++) if (d->reached[r] >= SAVE_SHOWS) d->reached[r] = SAVE_SHOWS - 1;
    if (d->cont_level >= SAVE_SHOWS) d->cont_level = 0;
    if (d->cont_loop > 1) d->cont_loop = 0;
    for (int i = 0; i < SAVE_SHOWS; i++) { if (d->grade[i] > 4) d->grade[i] = 0; d->reels[i] &= 7; }
    if (d->music_vol > 10) d->music_vol = 8;
    if (d->sfx_vol > 10) d->sfx_vol = 8;
    if (d->difficulty > 2) d->difficulty = 1;
    if (d->layout > 1) d->layout = 0;
    if (d->outfit > 4) d->outfit = 0;
    if (d->style > 1) d->style = 0;
    if (d->runner > 1) d->runner = 0;
    if (d->language > 1) d->language = 0;
    if (d->tickets < 0 || d->tickets > 999999) d->tickets = 0;
    if (d->up_gun > 3) d->up_gun = 3; if (d->up_armor > 2) d->up_armor = 2; if (d->up_power > 2) d->up_power = 2; if (d->up_super > 2) d->up_super = 2;
    if (d->screens > 1) d->screens = 0; if (d->subtitles > 1) d->subtitles = 1; if (d->assist > 1) d->assist = 0;
}
/* the layout before the ninth show existed */
typedef struct {
    unsigned magic, version;
    unsigned char reached[2], cleared[2][8], grade[8], reels[8];
    int best_score[8], best_time[8];
    unsigned char seen_ending, true_ending, seen_intro, cont_loop, cont_level;
    int rush_best;
    unsigned char music_vol, sfx_vol, rumble, layout, shake, difficulty, outfit, style, runner;
    unsigned char badge[4], language, breakers_flipped, closets_hidden, spare[8];
} SaveDataV1;
static int from_v1(const unsigned char *buf, SaveData *d) {
    SaveDataV1 o; memcpy(&o, buf, sizeof o);
    if (o.magic != SAVE_MAGIC || o.version != 1) return 0;
    defaults(d);
    memcpy(d->reached, o.reached, 2);
    for (int r = 0; r < 2; r++) memcpy(d->cleared[r], o.cleared[r], 8);
    memcpy(d->grade, o.grade, 8); memcpy(d->reels, o.reels, 8);
    memcpy(d->best_score, o.best_score, sizeof o.best_score); memcpy(d->best_time, o.best_time, sizeof o.best_time);
    d->seen_ending = o.seen_ending; d->true_ending = o.true_ending; d->seen_intro = o.seen_intro; d->cont_loop = o.cont_loop; d->cont_level = o.cont_level;
    d->rush_best = o.rush_best;
    d->music_vol = o.music_vol; d->sfx_vol = o.sfx_vol; d->rumble = o.rumble; d->layout = o.layout; d->shake = o.shake;
    d->difficulty = o.difficulty; d->outfit = o.outfit; d->style = o.style; d->runner = o.runner;
    memcpy(d->badge, o.badge, 4); d->language = o.language; d->breakers_flipped = o.breakers_flipped; d->closets_hidden = o.closets_hidden;
    return 1;
}
static int read_file(int slot, SaveData *out) {
    char p[256]; path(p, sizeof p, slot, "");
    FILE *f = fopen(p, "rb");
    if (!f) { path(p, sizeof p, slot, ".tmp"); f = fopen(p, "rb"); }   /* a write that got cut off between remove and rename */
    if (!f) return 0;
    unsigned char buf[sizeof(SaveData) + 64]; size_t n = fread(buf, 1, sizeof buf, f); fclose(f);
    SaveData d;
    if (n == sizeof d) { memcpy(&d, buf, sizeof d); if (d.magic != SAVE_MAGIC || d.version != SAVE_VERSION) return 0; }
    else if (n == sizeof(SaveDataV1)) { if (!from_v1(buf, &d)) return 0; }
    else if (n == V2_SIZE) {   /* version 3 only added fields at the end */
        unsigned m, v; memcpy(&m, buf, 4); memcpy(&v, buf + 4, 4); if (m != SAVE_MAGIC || v != 2) return 0;
        defaults(&d); memcpy(&d, buf, V2_SIZE); d.version = SAVE_VERSION; }
    else return 0;
    sanitize(&d); *out = d; return 1;
}
static void apply_settings(const SaveData *s) {
    save.music_vol = s->music_vol; save.sfx_vol = s->sfx_vol; save.rumble = s->rumble; save.layout = s->layout;
    save.shake = s->shake; save.difficulty = s->difficulty; save.style = s->style; save.language = s->language;
    save.screens = s->screens; save.subtitles = s->subtitles; save.assist = s->assist;
}
static int write_file(int slot, const SaveData *d) {
    char p[256], tmp[256]; path(p, sizeof p, slot, ""); path(tmp, sizeof tmp, slot, ".tmp");
    FILE *f = fopen(tmp, "wb"); if (!f) return 0;
    size_t n = fwrite(d, 1, sizeof *d, f);
    int ok = fclose(f) == 0 && n == sizeof *d;
    if (!ok) { remove(tmp); return 0; }
    remove(p);   /* rename() over an existing file isn't reliable on the Wii U's FAT card */
    rename(tmp, p);   /* if this fails the .tmp is still a whole save, and read_file finds it */
    return 1;
}

void save_load_settings(void) {
    defaults(&save);
    if (!enabled()) return;
    SaveData s; if (read_file(-1, &s)) apply_settings(&s);
}
int save_peek(int slot, SaveData *out) {
    if (!enabled()) return 0;
    if (read_file(slot, out)) return 1;
    if (slot == 0 && !getenv("MM_SAVE") && read_file(-2, out)) return 1;   /* the old single save becomes slot 1 */
    return 0;
}
void save_load(void) {
    SaveData keep = save;
    defaults(&save);
    if (!enabled()) { apply_settings(&keep); return; }
    SaveData d; if (save_peek(save_slot, &d)) save = d;
    apply_settings(&keep);   /* settings are the console's, not the slot's */
}
void save_use_slot(int slot) { save_slot = slot < 0 ? 0 : slot >= SAVE_SLOTS ? SAVE_SLOTS - 1 : slot; save_load(); }

void save_reset_progress(void) {
    SaveData keep = save; defaults(&save); apply_settings(&keep);
    save.seen_intro = keep.seen_intro; save.runner = keep.runner; save.outfit = 0;
}
void save_erase_slot(int slot) {
    if (!enabled()) return;
    char p[256]; path(p, sizeof p, slot, ""); remove(p); path(p, sizeof p, slot, ".tmp"); remove(p);
    if (slot == 0 && !getenv("MM_SAVE")) { path(p, sizeof p, -2, ""); remove(p); }
}

void save_write(void) {
    if (!enabled()) return;
    if (!getenv("MM_SAVE")) {
#ifdef __WIIU__
        mkdir("fs:/vol/external01/wiiu", 0777);
#endif
        mkdir(save_dir(), 0777);
    }
    save.magic = SAVE_MAGIC; save.version = SAVE_VERSION;
    int ok = write_file(save_slot, &save);
    ok = write_file(-1, &save) && ok;
    write_failed = !ok;
}
int save_write_failed(void) { return write_failed; }

int save_total_reels(void) {
    int n = 0; for (int i = 0; i < 8; i++) for (int b = 0; b < 3; b++) n += (save.reels[i] >> b) & 1; return n;
}
