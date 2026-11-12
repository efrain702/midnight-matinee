#ifndef SAVE_H
#define SAVE_H
/* Everything that survives turning the Wii U off: progress, records, unlocks and settings.
   Stored on the SD card (sd:/wiiu/midnight_matinee/save.dat) or next to the game on PC. */
#define SAVE_SHOWS 9   /* the eight shows plus the secret ninth */
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
    unsigned char badge[4];        /* Stagehand Badges earned (bits) */
    unsigned char language, breakers_flipped, closets_hidden;
    unsigned char secret_seen;            /* the "a secret show has opened" note was shown */
    unsigned char spare[7];
    /* ---- version 3 ---- */
    int tickets;                          /* Ticket Booth currency (earned from score) */
    unsigned char up_gun, up_armor, up_power, up_super;   /* Ticket Booth upgrades bought */
    unsigned char book[8];                /* Monster Book: bit per enemy type (0-31) and boss (32-40) defeated */
    unsigned char screens;                /* 0 TV + GamePad office, 1 same picture on both (off-TV play); next launch */
    unsigned char subtitles, assist;      /* settings */
    unsigned char rehearsed;              /* finished the Rehearsal */
    unsigned short remix_cleared;         /* Midnight Remix: bit per show beaten */
    unsigned char spare3[10];
} SaveData;

#define SAVE_SLOTS 3
extern SaveData save;
extern int save_slot;         /* 0..2: whose progress `save` holds */
void save_load_settings(void);   /* at boot, before a slot is picked: settings only (they're shared by every slot) */
void save_load(void);   /* fills `save` from the current slot (defaults when there's no file or it's from an older build) */
void save_use_slot(int slot);    /* pick a slot and load it */
int save_peek(int slot, SaveData *out);   /* 1 if that slot has a save; *out gets it */
void save_erase_slot(int slot);
int save_write_failed(void);  /* the last write didn't reach the SD card (locked or missing) */
void save_write(void);  /* best effort: a missing or locked SD card just means nothing is kept */
void save_reset_progress(void);
int save_total_reels(void);
#endif
