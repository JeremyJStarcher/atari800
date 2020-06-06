#define _POSIX_C_SOURCE 200112L /* for snprintf */
#include <SDL.h>

#include "sdl2_joystick.h"
#include "SDL_joystick.h"
#include "input.h"
#include "ui.h"
#include "platform.h"
#include "log.h"
#include "util.h"

#include <stdio.h>

#define MAX_JOYSTICKS 4
static SDL_Joystick *joystick[MAX_JOYSTICKS] = {NULL, NULL, NULL, NULL};
static int joystick_nbuttons[MAX_JOYSTICKS];
static SDL_INPUT_RealJSConfig_t real_js_configs[MAX_JOYSTICKS];
static int joysticks_found = 0;
static int swap_joysticks = FALSE;
static const Uint8 *kbhits;

/* real joysticks */

static int fd_joystick0 = -1;
static int fd_joystick1 = -1;

static struct js_state
{
	unsigned int port;
	unsigned int trig;
} sdl_js_state[MAX_JOYSTICKS];

#define minjoy 10000 /* real joystick tolerancy */

static int KBD_TRIG_0 = SDLK_RCTRL;
static int KBD_STICK_0_LEFT = SDLK_KP_4;
static int KBD_STICK_0_RIGHT = SDLK_KP_6;
static int KBD_STICK_0_DOWN = SDLK_KP_5;
static int KBD_STICK_0_UP = SDLK_KP_8;

static int KBD_TRIG_1 = SDLK_LCTRL;
static int KBD_STICK_1_LEFT = SDLK_a;
static int KBD_STICK_1_RIGHT = SDLK_d;
static int KBD_STICK_1_DOWN = SDLK_s;
static int KBD_STICK_1_UP = SDLK_w;

/* For better handling of the PLATFORM_Configure-recognition...
   Takes a keySym as integer-string and fills the value
   into the retval referentially.
   Authors: B.Schreiber, A.Martinez
   fixed and cleaned up by joy */
static int SDLKeyBind(int *retval, char *sdlKeySymIntStr)
{
	int ksym;

	if (retval == NULL || sdlKeySymIntStr == NULL)
	{
		return FALSE;
	}

	/* make an int out of the keySymIntStr... */
	ksym = Util_sscandec(sdlKeySymIntStr);
	*retval = ksym;
	return TRUE;

	/* JJS: FIXME
	if (ksym > SDLK_FIRST && ksym < SDLK_LAST) {
		*retval = ksym;
		return TRUE;
	}
	else {
		return FALSE;
	}
	*/
}

void PLATFORM_GetJoystickKeyName(int joystick, int direction, char *buffer, int bufsize)
{
	const char *key = "";
	switch (direction)
	{
	case 0:
		key = SDL_GetKeyName((SDL_Keycode)(joystick == 0 ? KBD_STICK_0_LEFT : KBD_STICK_1_LEFT));
		break;
	case 1:
		key = SDL_GetKeyName((SDL_Keycode)(joystick == 0 ? KBD_STICK_0_UP : KBD_STICK_1_UP));
		break;
	case 2:
		key = SDL_GetKeyName((SDL_Keycode)(joystick == 0 ? KBD_STICK_0_RIGHT : KBD_STICK_1_RIGHT));
		break;
	case 3:
		key = SDL_GetKeyName((SDL_Keycode)(joystick == 0 ? KBD_STICK_0_DOWN : KBD_STICK_1_DOWN));
		break;
	case 4:
		key = SDL_GetKeyName((SDL_Keycode)(joystick == 0 ? KBD_TRIG_0 : KBD_TRIG_1));
		break;
	}
	snprintf(buffer, bufsize, "%11s", key);
}

/*Get pointer to a real joystick configuration*/
SDL_INPUT_RealJSConfig_t *SDL_INPUT_GetRealJSConfig(int joyIndex)
{
	return &real_js_configs[joyIndex];
}

void PLATFORM_SetJoystickKey(int joystick, int direction, int value)
{
	if (joystick == 0)
	{
		switch (direction)
		{
		case 0:
			KBD_STICK_0_LEFT = value;
			break;
		case 1:
			KBD_STICK_0_UP = value;
			break;
		case 2:
			KBD_STICK_0_RIGHT = value;
			break;
		case 3:
			KBD_STICK_0_DOWN = value;
			break;
		case 4:
			KBD_TRIG_0 = value;
			break;
		}
	}
	else
	{
		switch (direction)
		{
		case 0:
			KBD_STICK_1_LEFT = value;
			break;
		case 1:
			KBD_STICK_1_UP = value;
			break;
		case 2:
			KBD_STICK_1_RIGHT = value;
			break;
		case 3:
			KBD_STICK_1_DOWN = value;
			break;
		case 4:
			KBD_TRIG_1 = value;
			break;
		}
	}
}

int Init_SDL2_Joysticks(int *argc, char *argv[])
{
	int i;
	joysticks_found = 0;
	for (i = 0; i < SDL_NumJoysticks() && i < MAX_JOYSTICKS; i++)
	{
		joystick[joysticks_found] = SDL_JoystickOpen(i);
		if (joystick[joysticks_found] == NULL)
			Log_print("Joystick %i not found", i);
		else
		{
			Log_print("Joystick %i found", i);
			joystick_nbuttons[joysticks_found] = SDL_JoystickNumButtons(joystick[i]);
#ifdef USE_UI_BASIC_ONSCREEN_KEYBOARD
			if (joystick_nbuttons[joysticks_found] > OSK_MAX_BUTTONS)
				joystick_nbuttons[joysticks_found] = OSK_MAX_BUTTONS;
#endif
			joysticks_found++;
		}
	}

	kbhits = SDL_GetKeyboardState(NULL);

	if (kbhits == NULL)
	{
		Log_print("SDL_GetKeyState() failed");
		Log_flushlog();
		return FALSE;
	}
	return TRUE;
}

/*Reset configurations of the real joysticks*/
static void reset_real_js_configs(void)
{
	int i;
	for (i = 0; i < MAX_JOYSTICKS; i++)
	{
		real_js_configs[i].use_hat = FALSE;
	}
}

/*Set real joystick to use hat instead of axis*/
static int set_real_js_use_hat(int joyIndex, const char *parm)
{
	real_js_configs[joyIndex].use_hat = Util_sscandec(parm) != 0 ? TRUE : FALSE;
	return TRUE;
}

/* For getting sdl key map out of the config...
   Authors: B.Schreiber, A.Martinez
   cleaned up by joy */
int SDL2_INPUT_ReadConfig(char *option, char *parameters)
{
	static int was_config_initialized = FALSE;

	if (was_config_initialized == FALSE)
	{
		reset_real_js_configs();
		was_config_initialized = TRUE;
	}

	if (strcmp(option, "SDL_JOY_0_ENABLED") == 0)
	{
		PLATFORM_kbd_joy_0_enabled = (parameters != NULL && parameters[0] != '0');
		return TRUE;
	}
	else if (strcmp(option, "SDL_JOY_1_ENABLED") == 0)
	{
		PLATFORM_kbd_joy_1_enabled = (parameters != NULL && parameters[0] != '0');
		return TRUE;
	}
	else if (strcmp(option, "SDL_JOY_0_LEFT") == 0)
		return SDLKeyBind(&KBD_STICK_0_LEFT, parameters);
	else if (strcmp(option, "SDL_JOY_0_RIGHT") == 0)
		return SDLKeyBind(&KBD_STICK_0_RIGHT, parameters);
	else if (strcmp(option, "SDL_JOY_0_DOWN") == 0)
		return SDLKeyBind(&KBD_STICK_0_DOWN, parameters);
	else if (strcmp(option, "SDL_JOY_0_UP") == 0)
		return SDLKeyBind(&KBD_STICK_0_UP, parameters);
	else if (strcmp(option, "SDL_JOY_0_TRIGGER") == 0)
		return SDLKeyBind(&KBD_TRIG_0, parameters);
	else if (strcmp(option, "SDL_JOY_1_LEFT") == 0)
		return SDLKeyBind(&KBD_STICK_1_LEFT, parameters);
	else if (strcmp(option, "SDL_JOY_1_RIGHT") == 0)
		return SDLKeyBind(&KBD_STICK_1_RIGHT, parameters);
	else if (strcmp(option, "SDL_JOY_1_DOWN") == 0)
		return SDLKeyBind(&KBD_STICK_1_DOWN, parameters);
	else if (strcmp(option, "SDL_JOY_1_UP") == 0)
		return SDLKeyBind(&KBD_STICK_1_UP, parameters);
	else if (strcmp(option, "SDL_JOY_1_TRIGGER") == 0)
		return SDLKeyBind(&KBD_TRIG_1, parameters);
	else if (strcmp(option, "SDL_JOY_0_USE_HAT") == 0)
		return set_real_js_use_hat(0, parameters);
	else if (strcmp(option, "SDL_JOY_1_USE_HAT") == 0)
		return set_real_js_use_hat(1, parameters);
	else if (strcmp(option, "SDL_JOY_2_USE_HAT") == 0)
		return set_real_js_use_hat(2, parameters);
	else if (strcmp(option, "SDL_JOY_3_USE_HAT") == 0)
		return set_real_js_use_hat(3, parameters);
	else
		return FALSE;
}

/*Write configurations of real joysticks*/
static void write_real_js_configs(FILE *fp)
{
	int i;
	for (i = 0; i < MAX_JOYSTICKS; i++)
	{
		fprintf(fp, "SDL_JOY_%d_USE_HAT=%d\n", i, real_js_configs[i].use_hat);
	}
}

/* Save the keybindings and the keybindapp options to the config file...
   Authors: B.Schreiber, A.Martinez
   cleaned up by joy */
void SDL2_INPUT_WriteConfig(FILE *fp)
{
	fprintf(fp, "SDL_JOY_0_ENABLED=%d\n", PLATFORM_kbd_joy_0_enabled);
	fprintf(fp, "SDL_JOY_0_LEFT=%d\n", KBD_STICK_0_LEFT);
	fprintf(fp, "SDL_JOY_0_RIGHT=%d\n", KBD_STICK_0_RIGHT);
	fprintf(fp, "SDL_JOY_0_UP=%d\n", KBD_STICK_0_UP);
	fprintf(fp, "SDL_JOY_0_DOWN=%d\n", KBD_STICK_0_DOWN);
	fprintf(fp, "SDL_JOY_0_TRIGGER=%d\n", KBD_TRIG_0);

	fprintf(fp, "SDL_JOY_1_ENABLED=%d\n", PLATFORM_kbd_joy_1_enabled);
	fprintf(fp, "SDL_JOY_1_LEFT=%d\n", KBD_STICK_1_LEFT);
	fprintf(fp, "SDL_JOY_1_RIGHT=%d\n", KBD_STICK_1_RIGHT);
	fprintf(fp, "SDL_JOY_1_UP=%d\n", KBD_STICK_1_UP);
	fprintf(fp, "SDL_JOY_1_DOWN=%d\n", KBD_STICK_1_DOWN);
	fprintf(fp, "SDL_JOY_1_TRIGGER=%d\n", KBD_TRIG_1);

	write_real_js_configs(fp);
}

static int get_SDL_joystick_state(SDL_Joystick *joystick)
{
	int x;
	int y;

	x = SDL_JoystickGetAxis(joystick, 0);
	y = SDL_JoystickGetAxis(joystick, 1);

	if (x > minjoy)
	{
		if (y < -minjoy)
			return INPUT_STICK_UR;
		else if (y > minjoy)
			return INPUT_STICK_LR;
		else
			return INPUT_STICK_RIGHT;
	}
	else if (x < -minjoy)
	{
		if (y < -minjoy)
			return INPUT_STICK_UL;
		else if (y > minjoy)
			return INPUT_STICK_LL;
		else
			return INPUT_STICK_LEFT;
	}
	else
	{
		if (y < -minjoy)
			return INPUT_STICK_FORWARD;
		else if (y > minjoy)
			return INPUT_STICK_BACK;
		else
			return INPUT_STICK_CENTRE;
	}
}

static int get_SDL_joystick_hat_state(SDL_Joystick *joystick)
{
	Uint8 hat = SDL_JoystickGetHat(joystick, 0);

	if ((hat & SDL_HAT_LEFT) == SDL_HAT_LEFT)
	{
		if ((hat & SDL_HAT_LEFTDOWN) == SDL_HAT_LEFTDOWN)
			return INPUT_STICK_LL;
		if ((hat & SDL_HAT_LEFTUP) == SDL_HAT_LEFTUP)
			return INPUT_STICK_UL;
		return INPUT_STICK_LEFT;
	}
	else if ((hat & SDL_HAT_RIGHT) == SDL_HAT_RIGHT)
	{
		if ((hat & SDL_HAT_RIGHTDOWN) == SDL_HAT_RIGHTDOWN)
			return INPUT_STICK_LR;
		else if ((hat & SDL_HAT_RIGHTUP) == SDL_HAT_RIGHTUP)
			return INPUT_STICK_UR;
		return INPUT_STICK_RIGHT;
	}
	else if ((hat & SDL_HAT_UP) == SDL_HAT_UP)
	{
		return INPUT_STICK_FORWARD;
	}
	else if ((hat & SDL_HAT_DOWN) == SDL_HAT_DOWN)
	{
		return INPUT_STICK_BACK;
	}

	return INPUT_STICK_CENTRE;
}

static int get_LPT_joystick_state(int fd)
{
#ifdef LPTJOY
	int status;

	ioctl(fd, LPGETSTATUS, &status);
	status ^= 0x78;

	if (status & 0x40)
	{ /* right */
		if (status & 0x10)
		{ /* up */
			return INPUT_STICK_UR;
		}
		else if (status & 0x20)
		{ /* down */
			return INPUT_STICK_LR;
		}
		else
		{
			return INPUT_STICK_RIGHT;
		}
	}
	else if (status & 0x80)
	{ /* left */
		if (status & 0x10)
		{ /* up */
			return INPUT_STICK_UL;
		}
		else if (status & 0x20)
		{ /* down */
			return INPUT_STICK_LL;
		}
		else
		{
			return INPUT_STICK_LEFT;
		}
	}
	else
	{
		if (status & 0x10)
		{ /* up */
			return INPUT_STICK_FORWARD;
		}
		else if (status & 0x20)
		{ /* down */
			return INPUT_STICK_BACK;
		}
		else
		{
			return INPUT_STICK_CENTRE;
		}
	}
#else
	return 0;
#endif /* LPTJOY */
}

static void get_platform_PORT(Uint8 *s0, Uint8 *s1, Uint8 *s2, Uint8 *s3)
{
	int stick0, stick1;
	stick0 = stick1 = INPUT_STICK_CENTRE;

	if (PLATFORM_kbd_joy_0_enabled)
	{
		if (kbhits[KBD_STICK_0_LEFT])
			stick0 &= INPUT_STICK_LEFT;
		if (kbhits[KBD_STICK_0_RIGHT])
			stick0 &= INPUT_STICK_RIGHT;
		if (kbhits[KBD_STICK_0_UP])
			stick0 &= INPUT_STICK_FORWARD;
		if (kbhits[KBD_STICK_0_DOWN])
			stick0 &= INPUT_STICK_BACK;
	}
	if (PLATFORM_kbd_joy_1_enabled)
	{
		if (kbhits[KBD_STICK_1_LEFT])
			stick1 &= INPUT_STICK_LEFT;
		if (kbhits[KBD_STICK_1_RIGHT])
			stick1 &= INPUT_STICK_RIGHT;
		if (kbhits[KBD_STICK_1_UP])
			stick1 &= INPUT_STICK_FORWARD;
		if (kbhits[KBD_STICK_1_DOWN])
			stick1 &= INPUT_STICK_BACK;
	}

	if (swap_joysticks)
	{
		*s1 = stick0;
		*s0 = stick1;
	}
	else
	{
		*s0 = stick0;
		*s1 = stick1;
	}

	if (fd_joystick0 != -1)
		*s0 &= get_LPT_joystick_state(fd_joystick0);
	else if (joystick[0] != NULL)
		*s0 &= sdl_js_state[0].port;

	if (fd_joystick1 != -1)
		*s1 &= get_LPT_joystick_state(fd_joystick1);
	else if (joystick[1] != NULL)
		*s1 &= sdl_js_state[1].port;

	*s2 = sdl_js_state[2].port;
	*s3 = sdl_js_state[3].port;
}

static void get_platform_TRIG(Uint8 *t0, Uint8 *t1, Uint8 *t2, Uint8 *t3)
{
	int trig0, trig1;
	trig0 = trig1 = 1;

	if (PLATFORM_kbd_joy_0_enabled)
	{
		trig0 = !kbhits[KBD_TRIG_0];
	}

	if (PLATFORM_kbd_joy_1_enabled)
	{
		trig1 = !kbhits[KBD_TRIG_1];
	}

	if (swap_joysticks)
	{
		*t1 = trig0;
		*t0 = trig1;
	}
	else
	{
		*t0 = trig0;
		*t1 = trig1;
	}

	if (fd_joystick0 != -1)
	{
#ifdef LPTJOY
		int status;
		ioctl(fd_joystick0, LPGETSTATUS, &status);
		*t0 &= ((status & 8) > 0);
#endif /* LPTJOY */
	}
	else if (joystick[0] != NULL)
	{
		trig0 = 1;
#ifdef USE_UI_BASIC_ONSCREEN_KEYBOARD
		if (OSK_enabled)
		{
			if (sdl_js_state[0].trig & (1 << OSK_BUTTON_TRIGGER))
				trig0 = 0;
		}
		else
#endif
			if (sdl_js_state[0].trig)
			trig0 = 0;
		*t0 &= trig0;
	}

	if (fd_joystick1 != -1)
	{
#ifdef LPTJOY
		int status;
		ioctl(fd_joystick1, LPGETSTATUS, &status);
		*t1 &= ((status & 8) > 0);
#endif /* LPTJOY */
	}
	else if (joystick[1] != NULL)
	{
		trig1 = 1;
		if (sdl_js_state[1].trig)
			trig1 = 0;
		*t1 &= trig1;
	}

	*t2 = sdl_js_state[2].trig ? 0 : 1;
	*t3 = sdl_js_state[3].trig ? 0 : 1;
}

static void update_SDL_joysticks(void)
{
	int joy;

	if (!joysticks_found)
		return;

	SDL_JoystickUpdate();

	for (joy = 0; joy < joysticks_found; joy++)
	{
		int i;

		if (real_js_configs[joy].use_hat == FALSE)
		{
			sdl_js_state[joy].port = get_SDL_joystick_state(joystick[joy]);
		}
		else
		{
			sdl_js_state[joy].port = get_SDL_joystick_hat_state(joystick[joy]);
		}

		sdl_js_state[joy].trig = 0;
		for (i = 0; i < joystick_nbuttons[joy]; i++)
		{
			if (SDL_JoystickGetButton(joystick[joy], i))
			{
				sdl_js_state[joy].trig |= 1 << i;
			}
		}
	}
}

int PLATFORM_PORT(int num)
{
#ifndef DONT_DISPLAY
	UBYTE a, b, c, d;
	update_SDL_joysticks();
	get_platform_PORT(&a, &b, &c, &d);
	if (num == 0)
	{
		return (b << 4) | (a & 0x0f);
	}
	else if (num == 1)
	{
		return (d << 4) | (c & 0x0f);
	}
#endif
	return 0xff;
}

int PLATFORM_TRIG(int num)
{
#ifndef DONT_DISPLAY
	UBYTE a, b, c, d;
	get_platform_TRIG(&a, &b, &c, &d);
	switch (num)
	{
	case 0:
		return a;
	case 1:
		return b;
	case 2:
		return c;
	case 3:
		return d;
	default:
		break;
	}
#endif
	return 1;
}
