
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

#define GRAPHICS_WIDTH 336
#define GRAPHICS_HEIGHT 192

#define CANVAS_WIDTH GRAPHICS_WIDTH
#define CANVAS_HEIGHT GRAPHICS_HEIGHT

int window_width = CANVAS_WIDTH;
int window_height = CANVAS_HEIGHT;
int window_x = 0;
int window_y = 0;

SDL_Window *window;
SDL_Renderer *renderer;

/* This value must be set during initialisation, because on Windows BPP
   autodetection works only before the first call to SDL_SetVideoMode(). */
int SDL_VIDEO_native_bpp;

int SDL_VIDEO_scanlines_percentage = 5;
int SDL_VIDEO_interpolate_scanlines = TRUE;
int SDL_VIDEO_width;
int SDL_VIDEO_height;

VIDEOMODE_MODE_t SDL_VIDEO_current_display_mode = VIDEOMODE_MODE_NORMAL;

int SDL_VIDEO_vsync = FALSE;
int SDL_VIDEO_vsync_available;

static int window_maximised = FALSE;

static int force_windowed = FALSE;
static int force_standard_screen = FALSE;

int SDL_VIDEO_ReadConfig(char *option, char *parameters)
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
}

void SDL_VIDEO_WriteConfig(FILE *fp)
{
	SDL_GetWindowPosition(window, &window_x, &window_y);
	SDL_GetWindowSize(window, &window_width, &window_height);
	fprintf(fp, "SDL2_WINDOW_WIDTH=%d\n", window_width);
	fprintf(fp, "SDL2_WINDOW_HEIGHT=%d\n", window_height);
	fprintf(fp, "SDL2_WINDOW_X=%d\n", window_x);
	fprintf(fp, "SDL2_WINDOW_Y=%d\n", window_y);
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

int SDL_VIDEO_SW_Initialise(int *argc, char *argv[]) {
    /* There is no SW side of things */
    return TRUE;
}

int SDL_VIDEO_Initialise(int *argc, char *argv[])
{
	int i, j;
	int help_only = FALSE;

	for (i = j = 1; i < *argc; i++) {
		int i_a = (i + 1 < *argc);		/* is argument available? */
		int a_m = FALSE;			/* error, argument missing! */
		if (strcmp(argv[i], "-scanlines") == 0) {
			if (i_a) {
				SDL_VIDEO_scanlines_percentage  = Util_sscandec(argv[++i]);
			}
			else a_m = TRUE;
		}
		else if (strcmp(argv[i], "-scanlinesint") == 0)
			SDL_VIDEO_interpolate_scanlines = TRUE;
		else if (strcmp(argv[i], "-no-scanlinesint") == 0)
			SDL_VIDEO_interpolate_scanlines = FALSE;
#if HAVE_OPENGL
		else if (strcmp(argv[i], "-video-accel") == 0)
			currently_opengl = SDL_VIDEO_opengl = TRUE;
		else if (strcmp(argv[i], "-no-video-accel") == 0)
			currently_opengl = SDL_VIDEO_opengl = FALSE;
#endif /* HAVE_OPENGL */
		else if (strcmp(argv[i], "-vsync") == 0)
			SDL_VIDEO_vsync = TRUE;
		else if (strcmp(argv[i], "-no-vsync") == 0)
			SDL_VIDEO_vsync = FALSE;
		else {
			if (strcmp(argv[i], "-help") == 0) {
				help_only = TRUE;
				Log_print("\t-scanlines        Set visibility of scanlines (0..100)");
				Log_print("\t-scanlinesint     Enable scanlines interpolation");
				Log_print("\t-no-scanlinesint  Disable scanlines interpolation");
#if HAVE_OPENGL
				Log_print("\t-video-accel      Use hardware video acceleration");
				Log_print("\t-no-video-accel   Don't use hardware video acceleration");
#endif /* HAVE_OPENGL */
				Log_print("\t-vsync            Synchronize display to vertical retrace");
				Log_print("\t-no-vsync         Don't synchronize display to vertical retrace");
			}
			argv[j++] = argv[i];
		}

		if (a_m) {
			Log_print("Missing argument for '%s'", argv[i]);
			return FALSE;
		}
	}
	*argc = j;

	if (!SDL_VIDEO_SW_Initialise(argc, argv)
#if HAVE_OPENGL
	    || !SDL_VIDEO_GL_Initialise(argc, argv)
#endif
	)
		return FALSE;

	if (!help_only) {
#ifdef HAVE_WINDOWS_H
		/* On Windows the DirectX SDL backend is glitchy in windowed modes, but allows
		   for vertical synchronisation in fullscreen modes. Unless the user specified
		   his own backend, use DirectX in fullscreen modes and Windib in windowed modes. */
		if (SDL_getenv("SDL_VIDEODRIVER") != NULL)
			user_video_driver = TRUE;
		else if (VIDEOMODE_windowed)
			SDL_putenv("SDL_VIDEODRIVER=windib");
		else
			SDL_putenv("SDL_VIDEODRIVER=directx");
#endif /* HAVE_WINDOWS_H */
		SDL_VIDEO_InitSDL();
	}

	return TRUE;
}


int SDL_VIDEO_InitSDL()
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

void SDL_VIDEO_Exit(void)
{
	SDL_VIDEO_QuitSDL();

#if 0
#ifdef NTSC_FILTER
	if (FILTER_NTSC_emu)
#endif
	{
		/* Turning filter off */
		FILTER_NTSC_Delete(FILTER_NTSC_emu);
		FILTER_NTSC_emu = NULL;
	}
#endif
}

int VIDEOMODE_Update(void)
{
    #if 0
	if (VIDEOMODE_windowed || force_windowed)
		return UpdateVideoWindowed(FALSE);
	else
		return UpdateVideoFullscreen();
        #endif
}

void PLATFORM_DisplayScreen(void)
{
	int x, y;
	int render_width, render_height;
	SDL_Texture *texture;
	UBYTE *screen;
	static unsigned char pixels[CANVAS_WIDTH * CANVAS_HEIGHT * 4];

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

void SDL_VIDEO_QuitSDL(void)
{
    #if 0
	if (SDL_VIDEO_screen != NULL) {
#if HAVE_OPENGL
		if (currently_opengl)
			SDL_VIDEO_GL_Cleanup();
#endif
		SDL_VIDEO_screen = NULL;

		SDL_QuitSubSystem(SDL_INIT_VIDEO);
	}
    #endif
}


