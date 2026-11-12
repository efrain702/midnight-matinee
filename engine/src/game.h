#ifndef GAME_H
#define GAME_H
#include "input.h"
#define PAD_W 854
#define PAD_H 480

void game_init(void);                 /* TV renderer is current */
void game_init_pad(void);             /* GamePad renderer is current: load its own copies of fonts/shapes */
/* advance one fixed 1/60 s step. pads[0] is the Wii U GamePad (keyboard on PC), pads[1..3] other controllers;
   connected[i] says which are plugged in */
void game_step(const Input pads[MAX_PADS], const int connected[MAX_PADS]);
void game_check(long frame);
void game_set_checkpoint(int c);   /* tests: MM_CP */   /* tests */
void game_draw(void);                 /* the TV picture */
void game_draw_pad(void);             /* the GamePad picture (security office) */
void game_touch(float x, float y);    /* a tap on the GamePad screen, 0..1 */
int game_wants_quit(void);
int game_layout(void);                /* 0: B jumps, A shoots   1: A jumps, B shoots (Options) */
int game_take_rumble(int pad, float *strength, int *ms);   /* a buzz the game wants on this controller (Options can turn it off) */
void game_focus_lost(void);           /* HOME button / app switched away: pause */
void game_goto(const char *scene);    /* tests: "title", "level", "boss", "fiend" */
#endif
