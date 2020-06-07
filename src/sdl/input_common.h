#ifndef INPUT_COMMOH_H
#define INPUT_COMMOH_H
#ifdef __linux__
#define LPTJOY	1
#endif

#ifdef LPTJOY
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/lp.h>
#endif /* LPTJOY */

#include <SDL.h>

#include "config.h"
#include "sdl/input.h"
#include "akey.h"
#include "atari.h"
#include "binload.h"
#include "colours.h"
#include "filter_ntsc.h"
#include "../input.h"
#include "log.h"
#include "platform.h"
#include "pokey.h"
#include "sdl/video.h"
#include "ui.h"
#include "util.h"
#include "videomode.h"
#include "screen.h"
#ifdef USE_UI_BASIC_ONSCREEN_KEYBOARD
#include "ui_basic.h"
#endif

void reset_real_js_configs(void);
void write_real_js_configs(FILE* fp);
int set_real_js_use_hat(int joyIndex, const char* parm);

#define MAX_JOYSTICKS	4
static SDL_Joystick *joystick[MAX_JOYSTICKS] = { NULL, NULL, NULL, NULL };
static int joystick_nbuttons[MAX_JOYSTICKS];
static SDL_INPUT_RealJSConfig_t real_js_configs[MAX_JOYSTICKS];
static int joysticks_found = 0;
static struct js_state {
	unsigned int port;
	unsigned int trig;
} sdl_js_state[MAX_JOYSTICKS];

static int KBD_TRIG_0 = SDLK_RCTRL;
#ifdef SDL
static int KBD_STICK_0_LEFT = SDLK_KP4;
static int KBD_STICK_0_RIGHT = SDLK_KP6;
static int KBD_STICK_0_DOWN = SDLK_KP5;
static int KBD_STICK_0_UP = SDLK_KP8;
#endif

#ifdef SDL2
static int KBD_STICK_0_LEFT = SDLK_KP_4;
static int KBD_STICK_0_RIGHT = SDLK_KP_6;
static int KBD_STICK_0_DOWN = SDLK_KP_5;
static int KBD_STICK_0_UP = SDLK_KP_8;
#endif

static int KBD_TRIG_1 = SDLK_LCTRL;
static int KBD_STICK_1_LEFT = SDLK_a;
static int KBD_STICK_1_RIGHT = SDLK_d;
static int KBD_STICK_1_DOWN = SDLK_s;
static int KBD_STICK_1_UP = SDLK_w;

#endif