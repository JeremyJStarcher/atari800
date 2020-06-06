/*
 * sdl2/main.c- SDL2 graphics engine
 *
 * Copyright (c) 1995-1998 David Firth
 * Copyright (c) 1998-2014 Atari800 development team (see DOC/CREDITS)

 * This file is part of the Atari800 emulator project which emulates
 * the Atari 400, 800, 800XL, 130XE, and 5200 8-bit computers.
 *
 * Atari800 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Atari800 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Atari800; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "config.h"
#include <SDL.h>

#include <string.h>

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

#include "sdl2_input.h"
#include "sdl2_joystick.h"

#define GRAPHICS_WIDTH 336
#define GRAPHICS_HEIGHT 192

#define CANVAS_WIDTH GRAPHICS_WIDTH
#define CANVAS_HEIGHT GRAPHICS_HEIGHT

#define Atari_POT(x) 228

UBYTE quit = FALSE;
SDL_Window *window;
SDL_Renderer *renderer;
int last_atari_scan_code = AKEY_NONE;

static UBYTE STICK[4];
static UBYTE TRIG_input[4];
static UBYTE last_stick[4] = {INPUT_STICK_CENTRE, INPUT_STICK_CENTRE, INPUT_STICK_CENTRE, INPUT_STICK_CENTRE};

#ifdef SOUND
#include "../sound.h"
#endif

void get_render_size(SDL_Window *window, int *render_width, int *render_height);
void process_input(void);
void SDL2_VIDEO_WriteConfig(FILE *fp);
int SDL2_VIDEO_Initialise(int *argc, char *argv[]);

int window_width = CANVAS_WIDTH;
int window_height = CANVAS_HEIGHT;
int window_x = 0;
int window_y = 0;

int PLATFORM_Configure(char *option, char *parameters)
{
	if (strcmp(option, "SDL2_WINDOW_WIDTH") == 0)
	{
		window_width = Util_sscandec(parameters);
		return TRUE;
	}

	if (strcmp(option, "SDL2_WINDOW_HEIGHT") == 0)
	{
		window_height = Util_sscandec(parameters);
		return TRUE;
	}

	if (strcmp(option, "SDL2_WINDOW_X") == 0)
	{
		window_x = Util_sscandec(parameters);
		return TRUE;
	}

	if (strcmp(option, "SDL2_WINDOW_Y") == 0)
	{
		window_y = Util_sscandec(parameters);
		return TRUE;
	}

	return SDL2_INPUT_ReadConfig(option, parameters);
}

void SDL2_VIDEO_WriteConfig(FILE *fp)
{
	SDL_GetWindowPosition(window, &window_x, &window_y);
	SDL_GetWindowSize(window, &window_width, &window_height);
	fprintf(fp, "SDL2_WINDOW_WIDTH=%d\n", window_width);
	fprintf(fp, "SDL2_WINDOW_HEIGHT=%d\n", window_height);
	fprintf(fp, "SDL2_WINDOW_X=%d\n", window_x);
	fprintf(fp, "SDL2_WINDOW_Y=%d\n", window_y);
}

void PLATFORM_ConfigSave(FILE *fp)
{
	SDL2_VIDEO_WriteConfig(fp);
	SDL2_INPUT_WriteConfig(fp);
}

int SDL2_VIDEO_Initialise(int *argc, char *argv[])
{
	if (SDL_Init(SDL_INIT_VIDEO
#ifdef SOUND
				 | SDL_INIT_AUDIO
#endif

				 | SDL_INIT_JOYSTICK) < 0)
	{
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		return FALSE;
	}

	window = SDL_CreateWindow("SDL_ttf in SDL2",
							  SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
							  CANVAS_WIDTH, CANVAS_HEIGHT,
							  SDL_WINDOW_RESIZABLE /* | SDL_WINDOW_OPENGL */ | SDL_WINDOW_SHOWN);

	if (window_width >= CANVAS_WIDTH && window_height >= CANVAS_HEIGHT)
	{
		SDL_SetWindowSize(window, window_width, window_height);
	}
	SDL_SetWindowPosition(window, window_x, window_y);

	renderer = SDL_CreateRenderer(window, -1, 0);

	if (window == NULL)
	{
		printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
		return FALSE;
	}

	if (renderer == NULL)
	{
		printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
		return FALSE;
	}

	if (TRUE)
	{
		SDL_RendererInfo rendererInfo;
		SDL_GetRendererInfo(renderer, &rendererInfo);
		printf("Screen intialized: Using driver: %s", rendererInfo.name);
	}
	return TRUE;
}

void PLATFORM_GetPixelFormat(PLATFORM_pixel_format_t *format)
{
	format->bpp = 32;

	format->rmask = 0x00ff0000;
	format->gmask = 0x0000ff00;
	format->bmask = 0x000000ff;
}

void PLATFORM_MapRGB(void *dest, int const *palette, int size)
{
	int i;
	SDL_Surface *surf;
	SDL_PixelFormat *f;

	surf = SDL_CreateRGBSurfaceWithFormat(0, 10, 10, 32, SDL_PIXELFORMAT_ARGB8888);
	f = surf->format;

	for (i = 0; i < size; ++i)
	{
		Uint32 c = SDL_MapRGB(
			f,
			(palette[i] & 0x00ff0000) >> 16,
			(palette[i] & 0x0000ff00) >> 8,
			(palette[i] & 0x000000ff));
		switch (32)
		{
		case 16:
			((UWORD *)dest)[i] = (UWORD)c;
			break;
		case 32:
			((ULONG *)dest)[i] = (ULONG)c;
			break;
		}
	}

	SDL_FreeSurface(surf);
}

int PLATFORM_Initialise(int *argc, char *argv[])
{
	if (!SDL2_VIDEO_Initialise(argc, argv)
#ifdef SOUND
		|| !Sound_Initialise(argc, argv)
#endif
		|| !Init_SDL2_Joysticks(argc, argv))
		return FALSE;

	return TRUE;
}

int PLATFORM_Exit(int run_monitor)
{
	return FALSE;
}

/* Find the size of the window we can render into.
 * 
 * This scales to fit inside the SDL window and keep aspect ratio
 *
 *  */
void get_render_size(SDL_Window *window, int *render_width, int *render_height)
{
	int window_width, window_height;
	float canvasRatio;
	float windowRatio;

	SDL_GetWindowSize(window, &window_width, &window_height);

	canvasRatio = (float)GRAPHICS_HEIGHT / (float)GRAPHICS_WIDTH;
	windowRatio = (float)window_height / (float)window_width;

	if (windowRatio < canvasRatio)
	{
		*render_height = window_height;
		*render_width = *render_height / canvasRatio;
	}
	else
	{
		*render_width = window_width;
		*render_height = *render_width * canvasRatio;
	}
}

void process_input()
{
	int i;
	SDL_Event e;

	SDL_StartTextInput();

	while (SDL_PollEvent(&e) != 0)
	{
		if (e.type == SDL_QUIT)
		{
			quit = TRUE;
			INPUT_key_code = AKEY_EXIT;
		}

		if (e.type == SDL_KEYDOWN)
		{
			last_atari_scan_code = sdl_keysum2scan_down(e.key.keysym.sym);
			INPUT_key_code = last_atari_scan_code;
		}

		if (e.type == SDL_KEYUP)
		{
			last_atari_scan_code = sdl_keysum2scan_up(e.key.keysym.sym);
			INPUT_key_code = last_atari_scan_code;
		}

		if (e.type == SDL_TEXTINPUT)
		{
			UBYTE khar = (UBYTE)e.text.text[0];
			last_atari_scan_code = ascii2scan(khar);
			INPUT_key_code = last_atari_scan_code;
		}
	}

	/* handle joysticks */
#ifdef EVENT_RECORDING
	if (playingback)
	{
		gzgets(playbackfp, gzbuf, GZBUFSIZE);
		sscanf(gzbuf, "%d ", &i);
	}
	else
	{
#endif
		i = PLATFORM_PORT(0);
#ifdef EVENT_RECORDING
	}
	if (recording)
	{
		gzprintf(recordfp, "%d \n", i);
	}
#endif

	STICK[0] = i & 0x0f;
	STICK[1] = (i >> 4) & 0x0f;
#ifdef EVENT_RECORDING
	if (playingback)
	{
		gzgets(playbackfp, gzbuf, GZBUFSIZE);
		sscanf(gzbuf, "%d ", &i);
	}
	else
	{
#endif
		i = PLATFORM_PORT(1);
#ifdef EVENT_RECORDING
	}
	if (recording)
	{
		gzprintf(recordfp, "%d \n", i);
	}
#endif
	STICK[2] = i & 0x0f;
	STICK[3] = (i >> 4) & 0x0f;

	for (i = 0; i < 4; i++)
	{
		if (INPUT_joy_block_opposite_directions)
		{
			if ((STICK[i] & 0x0c) == 0)
			{							  /* right and left simultaneously */
				if (last_stick[i] & 0x04) /* if wasn't left before, move left */
					STICK[i] |= 0x08;
				else /* else move right */
					STICK[i] |= 0x04;
			}
			else
			{
				last_stick[i] &= 0x03;
				last_stick[i] |= STICK[i] & 0x0c;
			}
			if ((STICK[i] & 0x03) == 0)
			{							  /* up and down simultaneously */
				if (last_stick[i] & 0x01) /* if wasn't up before, move up */
					STICK[i] |= 0x02;
				else /* else move down */
					STICK[i] |= 0x01;
			}
			else
			{
				last_stick[i] &= 0x0c;
				last_stick[i] |= STICK[i] & 0x03;
			}
		}
		else
			last_stick[i] = STICK[i];
			/* Joystick Triggers */
#ifdef EVENT_RECORDING
		if (playingback)
		{
			int trigtemp;
			gzgets(playbackfp, gzbuf, GZBUFSIZE);
			sscanf(gzbuf, "%d ", &trigtemp);
			TRIG_input[i] = trigtemp;
		}
		else
		{
#endif
			TRIG_input[i] = PLATFORM_TRIG(i);
#ifdef EVENT_RECORDING
		}
		if (recording)
		{
			gzprintf(recordfp, "%d \n", TRIG_input[i]);
		}
#endif
		if ((INPUT_joy_autofire[i] == INPUT_AUTOFIRE_FIRE && !TRIG_input[i]) || (INPUT_joy_autofire[i] == INPUT_AUTOFIRE_CONT))
			TRIG_input[i] = (Atari800_nframes & 2) ? 1 : 0;
	}

	/* handle analog joysticks in Atari 5200 */
	if (Atari800_machine_type != Atari800_MACHINE_5200)
	{
		if (!INPUT_direct_mouse)
		{
			for (i = 0; i < 4; i++)
				POKEY_POT_input[i] = Atari_POT(i);
			if (Atari800_machine_type != Atari800_MACHINE_XLXE)
			{
				for (i = 4; i < 8; ++i)
					POKEY_POT_input[i] = Atari_POT(i);
			}
		}
	}
	else
	{
		for (i = 0; i < 4; i++)
		{
#ifdef DREAMCAST
			/* first get analog js data */
			POKEY_POT_input[2 * i] = Atari_POT(2 * i);		   /* x */
			POKEY_POT_input[2 * i + 1] = Atari_POT(2 * i + 1); /* y */
			if (POKEY_POT_input[2 * i] != INPUT_joy_5200_center || POKEY_POT_input[2 * i + 1] != INPUT_joy_5200_center)
				continue;
				/* if analog js is unused, alternatively try keypad */
#endif
			if ((STICK[i] & (INPUT_STICK_CENTRE ^ INPUT_STICK_LEFT)) == 0)
				POKEY_POT_input[2 * i] = INPUT_joy_5200_min;
			else if ((STICK[i] & (INPUT_STICK_CENTRE ^ INPUT_STICK_RIGHT)) == 0)
				POKEY_POT_input[2 * i] = INPUT_joy_5200_max;
			else
				POKEY_POT_input[2 * i] = INPUT_joy_5200_center;
			if ((STICK[i] & (INPUT_STICK_CENTRE ^ INPUT_STICK_FORWARD)) == 0)
				POKEY_POT_input[2 * i + 1] = INPUT_joy_5200_min;
			else if ((STICK[i] & (INPUT_STICK_CENTRE ^ INPUT_STICK_BACK)) == 0)
				POKEY_POT_input[2 * i + 1] = INPUT_joy_5200_max;
			else
				POKEY_POT_input[2 * i + 1] = INPUT_joy_5200_center;
		}
	}
}

void PLATFORM_DisplayScreen(void)
{
	int x, y;
	int render_width, render_height;
	SDL_Texture *texture;
	UBYTE *screen;
	static unsigned char pixels[CANVAS_WIDTH * CANVAS_HEIGHT * 4];

	process_input();

	get_render_size(window, &render_width, &render_height);

	SDL_RenderClear(renderer);
	texture = SDL_CreateTexture(
		renderer,
		SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,
		GRAPHICS_WIDTH, GRAPHICS_HEIGHT);

	screen = (UBYTE *)Screen_atari + Screen_WIDTH;

	screen += 384 * 24 + 24;
	for (y = 0; y < GRAPHICS_HEIGHT; y++)
	{
		for (x = 0; x < GRAPHICS_WIDTH; x++)
		{
			unsigned char c = screen[x];

			unsigned char r = Colours_GetR(c);
			unsigned char b = Colours_GetB(c);
			unsigned char g = Colours_GetG(c);

			const unsigned int offset = (GRAPHICS_WIDTH * 4 * y) + x * 4;
			pixels[offset + 0] = b;
			pixels[offset + 1] = g;
			pixels[offset + 2] = r;
			pixels[offset + 3] = SDL_ALPHA_OPAQUE;
		}
		screen += 384;
	}

	SDL_UpdateTexture(
		texture,
		NULL,
		&pixels[0],
		GRAPHICS_WIDTH * 4);

	SDL_RenderSetLogicalSize(renderer, render_width, render_height);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
}

int PLATFORM_Keyboard(void)
{
	process_input();
	return last_atari_scan_code;
}

int main(int argc, char **argv)
{
	/* initialise Atari800 core */
	if (!Atari800_Initialise(&argc, argv))
	{
		Atari800_ErrExit();
		return 3;
	}

	/* main loop */
	while (!quit)
	{
		/* INPUT_key_code = PLATFORM_Keyboard(); */
		Atari800_Frame();
		if (Atari800_display_screen)
			PLATFORM_DisplayScreen();
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	Atari800_ErrExit();
	return 0;
}
