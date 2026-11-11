#ifndef AUDIO_H
#define AUDIO_H
void audio_init(void);
void music_play(const char *name, int loop);   /* name without extension, e.g. "level1_graveyard" */
void music_stop(void);
void music_preload(const char *name);         /* read a track into memory now so a later music_play of it is instant */
void sfx(const char *name);
void audio_quit(void);
void audio_set_volume(int music, int sfx);   /* 0..10 each */
void audio_duck(int on);                     /* quieter music while paused */
#endif
