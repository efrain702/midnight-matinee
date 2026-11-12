#ifndef SAVE_H
#define SAVE_H
/* Everything that survives turning the Wii U off: progress, records, unlocks and settings.
   Stored on the SD card (sd:/wiiu/midnight_matinee/save.dat) or next to the game on PC. */
#define SAVE_SHOWS 8
typedef struct {
    unsigned magic, version;
    unsigned char reached[2];             /* furthest show reached on the first run / the second run (0-based) */
    unsigned char cleared[2][SAVE_SHOWS]; /* show beaten, per run */
    unsigned char grade[SAVE_SHOWS];      /* best rank: 0 none, 1 C, 2 B, 3 A, 4 S */
    unsigned char reels[SAVE_SHOWS];      /* bit per hidden film reel found */
    int best_score[SAVE_SHOWS];
    int best_time[SAVE_SHOWS];            /* tenths of a second, 0 = none */
    unsigned char seen_ending, true_ending, seen_intro, cont_loop, cont_level;
    int rush_best;                        /* boss rush, tenths of a second, 0 = none */
    /* settings */
    unsigned char music_vol, sfx_vol, rumble, layout, shake, difficulty, outfit, style, runner;
    unsigned char spare[15];
} SaveData;

extern SaveData save;
void save_load(void);   /* fills `save` (defaults when there's no file or it's from an older build) */
void save_write(void);  /* best effort: a missing or locked SD card just means nothing is kept */
void save_reset_progress(void);
int save_total_reels(void);
#endif
