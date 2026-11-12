#include "save.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define SAVE_MAGIC 0x4D4D5356u   /* "MMSV" */
#define SAVE_VERSION 1

SaveData save;

/* Wii U: wut mounts the SD card at fs:/vol/external01 on start-up. PC: next to the game, or MM_SAVE (tests). */
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
static void path(char *out, size_t n, const char *suffix) {
    const char *custom = getenv("MM_SAVE");
    if (custom) snprintf(out, n, "%s%s", custom, suffix);
    else snprintf(out, n, "%s/save.dat%s", save_dir(), suffix);
}

static void defaults(void) {
    memset(&save, 0, sizeof save);
    save.magic = SAVE_MAGIC; save.version = SAVE_VERSION;
    save.music_vol = 8; save.sfx_vol = 8; save.rumble = 1; save.layout = 0; save.shake = 1; save.difficulty = 1;
}

void save_reset_progress(void) {
    SaveData keep = save; defaults();
    save.music_vol = keep.music_vol; save.sfx_vol = keep.sfx_vol; save.rumble = keep.rumble; save.layout = keep.layout;
    save.shake = keep.shake; save.difficulty = keep.difficulty; save.style = keep.style; save.seen_intro = keep.seen_intro; save.runner = keep.runner;
}

void save_load(void) {
    defaults();
    if (!enabled()) return;
    char p[256]; path(p, sizeof p, "");
    FILE *f = fopen(p, "rb");
    if (!f) { path(p, sizeof p, ".tmp"); f = fopen(p, "rb"); }   /* a write that got cut off between remove and rename */
    if (!f) return;
    SaveData d; size_t n = fread(&d, 1, sizeof d, f); fclose(f);
    if (n != sizeof d || d.magic != SAVE_MAGIC || d.version != SAVE_VERSION) return;
    save = d;
    /* never trust the file blindly */
    for (int r = 0; r < 2; r++) if (save.reached[r] >= SAVE_SHOWS) save.reached[r] = SAVE_SHOWS - 1;
    if (save.cont_level >= SAVE_SHOWS) save.cont_level = 0;
    if (save.cont_loop > 1) save.cont_loop = 0;
    for (int i = 0; i < SAVE_SHOWS; i++) { if (save.grade[i] > 4) save.grade[i] = 0; save.reels[i] &= 7; }
    if (save.music_vol > 10) save.music_vol = 8;
    if (save.sfx_vol > 10) save.sfx_vol = 8;
    if (save.difficulty > 2) save.difficulty = 1;
    if (save.layout > 1) save.layout = 0;
    if (save.outfit > 4) save.outfit = 0;
    if (save.style > 1) save.style = 0;
    if (save.runner > 1) save.runner = 0;
}

void save_write(void) {
    if (!enabled()) return;
    if (!getenv("MM_SAVE")) {
#ifdef __WIIU__
        mkdir("fs:/vol/external01/wiiu", 0777);
#endif
        mkdir(save_dir(), 0777);
    }
    char p[256], tmp[256]; path(p, sizeof p, ""); path(tmp, sizeof tmp, ".tmp");
    FILE *f = fopen(tmp, "wb"); if (!f) return;
    size_t n = fwrite(&save, 1, sizeof save, f);
    int ok = fclose(f) == 0 && n == sizeof save;
    if (!ok) { remove(tmp); return; }
    remove(p);   /* rename() over an existing file isn't reliable on the Wii U's FAT card */
    if (rename(tmp, p) != 0) { /* keep the .tmp: it is still a whole save */ }
}

int save_total_reels(void) {
    int n = 0; for (int i = 0; i < SAVE_SHOWS; i++) for (int b = 0; b < 3; b++) n += (save.reels[i] >> b) & 1; return n;
}
