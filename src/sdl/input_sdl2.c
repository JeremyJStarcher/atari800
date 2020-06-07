#include "config.h"
#include <SDL.h>

#include <string.h>

#include "input_common.h"
#include "screen.h"
#include "colours.h"
#include "antic.h" /* ypos */
#include "atari.h"
#include "binload.h"
#include "gtia.h" /* GTIA_COLPFx */
#include "input.h"
#include "akey.h"
#include "log.h"
#include "monitor.h"
#include "platform.h"
#include "ui.h" /* UI_alt_function */
#include "videomode.h"
#include "util.h"
#include "pokey.h"
#ifdef SOUND
#include "../sound.h"
#endif

static const Uint8 *kbhits;

int PLATFORM_kbd_joy_0_enabled = TRUE;	/* enabled by default, doesn't hurt */

int PLATFORM_kbd_joy_1_enabled = FALSE;	/* disabled, would steal normal keys */

static int SDLKeyBind(int *retval, char *sdlKeySymIntStr);
int PLATFORM_PORT(int num);
int PLATFORM_TRIG(int num);


/* For getting sdl key map out of the config...
   Authors: B.Schreiber, A.Martinez
   cleaned up by joy */
int SDL_INPUT_ReadConfig(char *option, char *parameters)
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

void SDL_INPUT_WriteConfig(FILE *fp)
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

int SDL_INPUT_Initialise(int *argc, char *argv[])
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


void SDL_INPUT_Exit(void)
{
	/* SDL_WM_GrabInput(SDL_GRAB_OFF); */
}

void SDL_INPUT_Restart(void)
{
    /*
	lastkey = SDLK_UNKNOWN;
	key_pressed = key_control = lastuni = 0;
	if(grab_mouse) SDL_WM_GrabInput(SDL_GRAB_ON);
    */
}

int PLATFORM_Keyboard(void)
{
    return AKEY_NONE;
}

void SDL_INPUT_Mouse(void)
{
    #if 0
	Uint8 buttons;

	if(INPUT_direct_mouse) {
		int potx, poty;

		buttons = SDL_GetMouseState(&potx, &poty);
		if(potx < 0) potx = 0;
		if(poty < 0) poty = 0;
		potx = (double)potx * (228.0 / (double)SDL_VIDEO_width);
		poty = (double)poty * (228.0 / (double)SDL_VIDEO_height);
		if(potx > 227) potx = 227;
		if(poty > 227) poty = 227;
		POKEY_POT_input[INPUT_mouse_port << 1] = 227 - potx;
		POKEY_POT_input[(INPUT_mouse_port << 1) + 1] = 227 - poty;
	} else {
		buttons = SDL_GetRelativeMouseState(&INPUT_mouse_delta_x, &INPUT_mouse_delta_y);
	}

	INPUT_mouse_buttons =
		((buttons & SDL_BUTTON(1)) ? 1 : 0) | /* Left button */
		((buttons & SDL_BUTTON(3)) ? 2 : 0) | /* Right button */
		((buttons & SDL_BUTTON(2)) ? 4 : 0); /* Middle button */
        #endif
}

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

#if 0
	if (ksym > SDLK_FIRST && ksym < SDLK_LAST) {
		*retval = ksym;
		return TRUE;
	}
	else {
		return FALSE;
	}
    #endif
}

int PLATFORM_PORT(int num)
{
    #if 0
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
    #endif
	return 0xff;
}

int PLATFORM_TRIG(int num)
{
    #if 0
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
#endif

	return 1;
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

int PLATFORM_GetRawKey(void)
{
    while (TRUE)
    {
        SDL_Event event;
        if (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_KEYDOWN:
                return event.key.keysym.sym;
            }
        }
    }
}

/*Get pointer to a real joystick configuration*/
SDL_INPUT_RealJSConfig_t *SDL_INPUT_GetRealJSConfig(int joyIndex)
{
	return &real_js_configs[joyIndex];
}

