/* Midnight Matinee - platform layer: TV + GamePad windows, input, fixed-step loop.
 * Built on The Tolling's engine; the two-screen setup follows Hall Pass.
 *
 * Test mode (no window needed):
 *   MM_HEADLESS=1 MM_FRAMES=900 MM_SHOTS=out MM_SHOT_EVERY=30 MM_SCRIPT=script.txt MM_SCENE=level ./midnight_matinee
 * script lines: "<from_frame> <to_frame> <keys>"  keys: L R U D J(ump) A(shoot) K(lock) W(eapon) [ ] (cams) P(ing) G(lights) T(start)
 * MM_SCRIPT2 drives controller 1 (a second player); MM_PADS=n pretends n controllers are connected.
 */
#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gfx.h"
#include "audio.h"
#include "game.h"

typedef struct { int a, b; unsigned keys; } ScriptLine;
static ScriptLine script[2][512]; static int nscript[2];

static unsigned keys_from(const char *s) {
    unsigned k = 0;
    for (; *s; s++) switch (*s) {
        case 'L': k |= B_LEFT; break;  case 'R': k |= B_RIGHT; break; case 'U': k |= B_UP; break;
        case 'D': k |= B_DOWN; break;  case 'J': k |= B_JUMP; break;  case 'A': k |= B_SHOOT; break;
        case 'K': k |= B_LOCK; break;  case 'W': k |= B_WEAPON; break;
        case '[': k |= B_CAMPREV; break; case ']': k |= B_CAMNEXT; break;
        case 'P': k |= B_PING; break;  case 'G': k |= B_LIGHTS; break; case 'T': k |= B_START; break;
    }
    return k;
}
static void load_script(const char *path, int w) {
    FILE *f = fopen(path, "r"); if (!f) return;
    char line[128];
    while (fgets(line, sizeof line, f) && nscript[w] < 512) {
        char ks[32] = ""; int a, b;
        if (line[0] == '#') continue;
        if (sscanf(line, "%d %d %31s", &a, &b, ks) >= 2) script[w][nscript[w]++] = (ScriptLine){a, b, keys_from(ks)};
    }
    fclose(f);
}

static unsigned read_pad(SDL_GameController *gc) {
    unsigned k = 0; if (!gc) return 0;
    if (SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_DPAD_LEFT)) k |= B_LEFT;
    if (SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_DPAD_RIGHT)) k |= B_RIGHT;
    if (SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_DPAD_UP)) k |= B_UP;
    if (SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_DPAD_DOWN)) k |= B_DOWN;
    int ax = SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_LEFTX), ay = SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_LEFTY);
    if (ax < -12000) k |= B_LEFT; if (ax > 12000) k |= B_RIGHT;
    if (ay < -14000) k |= B_UP;   if (ay > 14000) k |= B_DOWN;
    /* Nintendo layout (SDL names are swapped): A (right) jumps... same as The Tolling: B jumps, A/Y shoot */
    if (SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_B)) k |= B_JUMP;
    if (SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_A)) k |= B_SHOOT;
    if (SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_Y)) k |= B_SHOOT;
    if (SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_X)) k |= B_LIGHTS;
    if (SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)) k |= B_LOCK;
    if (SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_LEFTSHOULDER)) k |= B_WEAPON;
    if (SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_TRIGGERLEFT) > 16000) k |= B_CAMPREV;
    if (SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) > 16000) k |= B_CAMNEXT;
    if (SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_BACK)) k |= B_PING;
    if (SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_START)) k |= B_START;
    return k;
}

static unsigned read_keys(void) {
    const Uint8 *ks = SDL_GetKeyboardState(NULL); unsigned k = 0;
    if (ks[SDL_SCANCODE_LEFT]) k |= B_LEFT;  if (ks[SDL_SCANCODE_RIGHT]) k |= B_RIGHT;
    if (ks[SDL_SCANCODE_UP]) k |= B_UP;      if (ks[SDL_SCANCODE_DOWN]) k |= B_DOWN;
    if (ks[SDL_SCANCODE_Z] || ks[SDL_SCANCODE_SPACE]) k |= B_JUMP;
    if (ks[SDL_SCANCODE_X]) k |= B_SHOOT;
    if (ks[SDL_SCANCODE_LSHIFT] || ks[SDL_SCANCODE_RSHIFT]) k |= B_LOCK;
    if (ks[SDL_SCANCODE_A]) k |= B_WEAPON;
    if (ks[SDL_SCANCODE_Q]) k |= B_CAMPREV;  if (ks[SDL_SCANCODE_E]) k |= B_CAMNEXT;
    if (ks[SDL_SCANCODE_W]) k |= B_PING;     if (ks[SDL_SCANCODE_R]) k |= B_LIGHTS;
    if (ks[SDL_SCANCODE_RETURN]) k |= B_START;
    return k;
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    int headless = getenv("MM_HEADLESS") != NULL;
    if (headless) { SDL_setenv("SDL_VIDEODRIVER", "dummy", 1); SDL_setenv("MM_MUTE", "1", 1); }
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK) < 0) {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) { printf("SDL: %s\n", SDL_GetError()); return 1; }
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    SDL_Window *win = NULL, *padWin = NULL; SDL_Renderer *ren = NULL, *padRen = NULL;
    SDL_Surface *canvas = NULL, *padCanvas = NULL;
    if (headless) {
        canvas = SDL_CreateRGBSurfaceWithFormat(0, SCREEN_W, SCREEN_H, 32, SDL_PIXELFORMAT_RGBA32);
        ren = SDL_CreateSoftwareRenderer(canvas);
        padCanvas = SDL_CreateRGBSurfaceWithFormat(0, PAD_W, PAD_H, 32, SDL_PIXELFORMAT_RGBA32);
        padRen = SDL_CreateSoftwareRenderer(padCanvas);
    } else {
#if defined(__WIIU__) && defined(SDL_WINDOW_WIIU_TV_ONLY)
        /* two windows: the TV, and the GamePad with its own picture (the security office) */
        win = SDL_CreateWindow("Midnight Matinee", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_W, SCREEN_H,
                               SDL_WINDOW_SHOWN | SDL_WINDOW_WIIU_TV_ONLY | SDL_WINDOW_WIIU_PREVENT_SWAP);
        ren = win ? SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED) : NULL;
        if (ren) {
            padWin = SDL_CreateWindow("Midnight Matinee GamePad", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720,
                                      SDL_WINDOW_SHOWN | SDL_WINDOW_WIIU_GAMEPAD_ONLY);
            if (padWin) padRen = SDL_CreateRenderer(padWin, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        }
        if (!padRen) {   /* fall back to one picture on both screens */
            if (padWin) SDL_DestroyWindow(padWin); padWin = NULL;
            if (ren) SDL_DestroyRenderer(ren); if (win) SDL_DestroyWindow(win);
            win = SDL_CreateWindow("Midnight Matinee", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_W, SCREEN_H, SDL_WINDOW_SHOWN);
            ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        }
#else
        win = SDL_CreateWindow("Midnight Matinee (TV)", 40, 60, SCREEN_W, SCREEN_H, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
        ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!ren) ren = SDL_CreateRenderer(win, -1, 0);
        if (!getenv("MM_ONE_WINDOW")) {   /* on PC the GamePad screen gets its own window */
            padWin = SDL_CreateWindow("Midnight Matinee (GamePad)", 60 + SCREEN_W / 2, 120, PAD_W, PAD_H, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
            if (padWin) padRen = SDL_CreateRenderer(padWin, -1, 0);
        }
#endif
        SDL_RenderSetLogicalSize(ren, SCREEN_W, SCREEN_H);
    }
    if (padRen) SDL_RenderSetLogicalSize(padRen, PAD_W, PAD_H);

    gfx_init(ren);
    audio_init();
    game_init();
    if (padRen) { gfx_set_renderer(padRen); game_init_pad(); gfx_set_renderer(ren); }
    const char *sc = getenv("MM_SCENE"); if (sc) game_goto(sc);
    const char *sp = getenv("MM_SCRIPT"); if (sp) load_script(sp, 0);
    const char *sp2 = getenv("MM_SCRIPT2"); if (sp2) load_script(sp2, 1);
    const char *shots = getenv("MM_SHOTS");
    int every = getenv("MM_SHOT_EVERY") ? atoi(getenv("MM_SHOT_EVERY")) : 0;
    long maxf = getenv("MM_FRAMES") ? atol(getenv("MM_FRAMES")) : -1;
    int fake_pads = getenv("MM_PADS") ? atoi(getenv("MM_PADS")) : 1;

    SDL_GameController *gc[MAX_PADS] = {0};
    unsigned prev[MAX_PADS] = {0}; long frame = 0; int running = 1;
    Uint64 freq = SDL_GetPerformanceFrequency(), last = SDL_GetPerformanceCounter(); double acc = 0;
    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) running = 0;
            if (ev.type == SDL_KEYDOWN && ev.key.keysym.scancode == SDL_SCANCODE_ESCAPE) running = 0;
            if (ev.type == SDL_CONTROLLERDEVICEADDED) {
                SDL_GameController *c = SDL_GameControllerOpen(ev.cdevice.which);
                int dup = 0; for (int i = 0; i < MAX_PADS; i++) if (gc[i] == c) dup = 1;
                if (c && !dup) for (int i = 0; i < MAX_PADS; i++) if (!gc[i]) { gc[i] = c; break; }
            }
            if (ev.type == SDL_CONTROLLERDEVICEREMOVED) {
                SDL_GameController *c = SDL_GameControllerFromInstanceID(ev.cdevice.which);
                for (int i = 0; i < MAX_PADS; i++) if (c && gc[i] == c) { SDL_GameControllerClose(c); gc[i] = NULL; }
            }
            /* GamePad touch screen (and the mouse on the PC GamePad window) */
            if (ev.type == SDL_FINGERDOWN) game_touch(ev.tfinger.x, ev.tfinger.y);
            if (ev.type == SDL_MOUSEBUTTONDOWN && padWin && ev.button.windowID == SDL_GetWindowID(padWin)) {
                int w, h; SDL_GetWindowSize(padWin, &w, &h);
                if (w > 0 && h > 0) game_touch(ev.button.x / (float)w, ev.button.y / (float)h);
            }
        }
        int steps = 1;
        if (!headless) {
            Uint64 now = SDL_GetPerformanceCounter();
            acc += (double)(now - last) / freq; last = now;
            if (acc > 0.25) acc = 0.25;
            steps = 0; while (acc >= 1.0 / 60) { acc -= 1.0 / 60; steps++; }
        }
        for (int s = 0; s < steps; s++) {
            Input in[MAX_PADS]; int con[MAX_PADS];
            for (int i = 0; i < MAX_PADS; i++) {
                unsigned held = headless ? 0 : read_pad(gc[i]);
                if (i == 0 && !headless) held |= read_keys();
                if (i < 2) for (int k = 0; k < nscript[i]; k++) if (frame >= script[i][k].a && frame <= script[i][k].b) held |= script[i][k].keys;
                in[i] = (Input){held, held & ~prev[i]}; prev[i] = held;
                con[i] = headless ? (i < fake_pads) : (i == 0 || gc[i] != NULL);
            }
            game_step(in, con); frame++;
            if (maxf > 0 && frame >= maxf) running = 0;
            if (game_wants_quit()) running = 0;
        }
        if (steps == 0 && !headless) { SDL_Delay(1); continue; }
        int shoot_now = headless && shots && every > 0 && frame % every == 0;
        if (headless && !shoot_now) continue;
        gfx_set_renderer(ren); game_draw();
        if (padRen) { gfx_set_renderer(padRen); SDL_SetRenderDrawColor(padRen, 0, 0, 0, 255); SDL_RenderClear(padRen); game_draw_pad(); gfx_set_renderer(ren); }
        if (shoot_now) {
            char path[512];
            snprintf(path, sizeof path, "%s/f%05ld.png", shots, frame); IMG_SavePNG(canvas, path);
            snprintf(path, sizeof path, "%s/p%05ld.png", shots, frame); IMG_SavePNG(padCanvas, path);
        }
        if (!headless) {
            SDL_RenderPresent(ren);                 /* TV (on Wii U: no buffer swap yet) */
            if (padRen) SDL_RenderPresent(padRen);  /* GamePad, then both screens swap together */
        }
    }
    audio_quit();
    SDL_Quit();
    return 0;
}
