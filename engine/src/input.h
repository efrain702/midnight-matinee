#ifndef INPUT_H
#define INPUT_H
enum { B_LEFT = 1, B_RIGHT = 2, B_UP = 4, B_DOWN = 8, B_JUMP = 16, B_SHOOT = 32,
       B_LOCK = 64,      /* hold: stand still and aim (R) */
       B_WEAPON = 128,   /* swap weapon (L) */
       B_CAMPREV = 256, B_CAMNEXT = 512,   /* security cameras (ZL / ZR) */
       B_PING = 1024, B_LIGHTS = 2048,     /* security office actions (- / X) */
       B_START = 4096,
       B_SUPER = 8192 };   /* super move (Y) */
typedef struct { unsigned held, pressed; } Input;
#define MAX_PADS 4
#endif
