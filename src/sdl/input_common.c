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
#include "input_common.h"
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

/*Set real joystick to use hat instead of axis*/
int set_real_js_use_hat(int joyIndex, const char* parm)
{
    real_js_configs[joyIndex].use_hat = Util_sscandec(parm) != 0 ? TRUE : FALSE;
    return TRUE;
}


/*Reset configurations of the real joysticks*/
void reset_real_js_configs(void)
{
    int i;
    for (i = 0; i < MAX_JOYSTICKS; i++) {
        real_js_configs[i].use_hat = FALSE;
    }
}

/*Write configurations of real joysticks*/
void write_real_js_configs(FILE* fp)
{
    int i;
    for (i = 0; i < MAX_JOYSTICKS; i++) {
        fprintf(fp, "SDL_JOY_%d_USE_HAT=%d\n", i, real_js_configs[i].use_hat);
    }
}

