#include "audio.h"
#include "gfx.h"
#include <SDL_mixer.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static int ok = 0; static int vol_music = 8, vol_sfx = 8, ducked = 0;
static void apply_volume(void);
static Mix_Music *cur = NULL;
static char cur_name[64];
static const char *SFX[] = {"jump", "throw", "rivet", "sizzle", "boing", "hit", "hurt", "boom", "slam", "pickup", "checkpoint",
    "splash", "menu", "poof", "shatter", "clank", "charge", "power", "beam", "summon", "angry", "tick", "spit", "roar1", "roar2",
    "coin", "thunder", "casing", "lights", "closet", "fiendwarn", "splat",
    "land_wood", "land_metal", "land_tile", "reel", "iris", "boss_sting", "confirm", "dizzy",
    "vo_joe_hurt", "vo_joe_lights", "vo_joe_reel", "vo_joe_go", "vo_joe_win", "vo_joe_ko",
    "vo_rosie_hurt", "vo_rosie_lights", "vo_rosie_reel", "vo_rosie_go", "vo_rosie_win", "vo_rosie_ko",
    "ann_logo", "ann_title", "ann_clear", "ann_over",
    "super_joe", "super_rosie", "super_ready", "shop_buy"};
#define NSFX (int)(sizeof SFX / sizeof *SFX)
static Mix_Chunk *chunks[NSFX];

void audio_init(void) {
    if (getenv("MM_MUTE")) return;
    if (Mix_OpenAudio(48000, AUDIO_S16SYS, 2, 2048) < 0) { SDL_Log("audio: %s", Mix_GetError()); return; }
    Mix_Init(MIX_INIT_OGG);
    Mix_AllocateChannels(32);
    Mix_ReserveChannels(1);   /* channel 0: voices (narration, boss taunts) never get cut off by effects */
    char rel[64];
    for (int i = 0; i < NSFX; i++) {
        snprintf(rel, sizeof rel, "sfx/%s.wav", SFX[i]);
        chunks[i] = Mix_LoadWAV(content_path(rel));
    }
    ok = 1; apply_volume();
}

/* Tracks are read whole into memory before they play, so the audio thread never waits on the SD card
   mid-game (a slow read there starves the mixer: a stutter and a loud buzz on the Wii U). */
typedef struct { Mix_Music *mus; void *buf; char name[64]; } Track;
static Track next;                                   /* preloaded, waiting to play */
static void *cur_buf = NULL;

static void track_free(Track *t) { if (t->mus) Mix_FreeMusic(t->mus); SDL_free(t->buf); memset(t, 0, sizeof *t); }

static int track_load(Track *t, const char *name) {
    static const char *FALLBACK[][2] = {{"mm_none", "mm_level1"}};
    const char *file = name; char rel[96];
    for (int tries = 0; tries < 2; tries++) {
        snprintf(rel, sizeof rel, "music/%s.ogg", file);
        size_t n = 0; void *buf = SDL_LoadFile(content_path(rel), &n);
        if (buf) {
            Mix_Music *m = Mix_LoadMUS_RW(SDL_RWFromConstMem(buf, (int)n), 1);
            if (m) { t->mus = m; t->buf = buf; snprintf(t->name, sizeof t->name, "%s", name); return 1; }
            SDL_free(buf);
        }
        file = NULL;   /* a track that hasn't been made yet: borrow a stand-in */
        for (unsigned i = 0; i < sizeof FALLBACK / sizeof *FALLBACK; i++) if (!strcmp(name, FALLBACK[i][0])) file = FALLBACK[i][1];
        if (!file) break;
    }
    SDL_Log("music %s: %s", name, Mix_GetError()); return 0;
}

void music_preload(const char *name) {
    if (!ok || (next.mus && !strcmp(next.name, name))) return;
    track_free(&next); track_load(&next, name);
}

void music_play(const char *name, int loop) {
    if (!ok) return;
    if (cur && !strcmp(cur_name, name) && Mix_PlayingMusic()) return;
    if (cur) { Mix_HaltMusic(); Mix_FreeMusic(cur); cur = NULL; }
    SDL_free(cur_buf); cur_buf = NULL;
    Track t = {0};
    if (next.mus && !strcmp(next.name, name)) { t = next; memset(&next, 0, sizeof next); }   /* already in memory: instant */
    else if (!track_load(&t, name)) return;
    cur = t.mus; cur_buf = t.buf;
    snprintf(cur_name, sizeof cur_name, "%s", name);
    /* the .ogg files carry LOOPSTART/LOOPLENGTH tags, so -1 loops seamlessly at the right point */
    Mix_PlayMusic(cur, loop ? -1 : 1);
}

void music_stop(void) { if (ok) Mix_HaltMusic(); cur_name[0] = 0; }

void sfx(const char *name) {
    if (!ok) return;
    for (int i = 0; i < NSFX; i++) if (!strcmp(SFX[i], name)) { if (chunks[i]) Mix_PlayChannel(-1, chunks[i], 0); return; }
}

static void apply_volume(void) {
    if (!ok) return;
    Mix_VolumeMusic(MIX_MAX_VOLUME * vol_music * (ducked ? 3 : 8) / 80);
    Mix_Volume(-1, MIX_MAX_VOLUME * vol_sfx / 10);
}
void audio_set_volume(int music, int sfxv) { vol_music = music; vol_sfx = sfxv; apply_volume(); }
void audio_duck(int on) { ducked = on; apply_volume(); }

void audio_quit(void) { if (ok) Mix_CloseAudio(); }

int music_available(const char *name) {   /* is music/<name>.ogg on the SD card? */
    char rel[96]; snprintf(rel, sizeof rel, "music/%s.ogg", name);
    SDL_RWops *f = SDL_RWFromFile(content_path(rel), "rb");
    if (!f) return 0; SDL_RWclose(f); return 1;
}

/* ---- voices: read from the card when needed (they're long), one at a time on the reserved channel ---- */
static Mix_Chunk *voice_chunk;
void voice_stop(void) { if (!ok) return; Mix_HaltChannel(0); if (voice_chunk) { Mix_FreeChunk(voice_chunk); voice_chunk = NULL; } }
int voice_play(const char *name) {
    if (!ok) return 0;
    voice_stop();
    char rel[96]; snprintf(rel, sizeof rel, "vo/%s.wav", name);
    voice_chunk = Mix_LoadWAV(content_path(rel));
    if (!voice_chunk) return 0;
    Mix_VolumeChunk(voice_chunk, MIX_MAX_VOLUME);
    Mix_PlayChannel(0, voice_chunk, 0);
    return 1;
}
int voice_playing(void) { return ok && Mix_Playing(0); }
