#include "input.h"
#include "SDL_keyboard.h"
#include "sdl2_input.h"
#include "input.h"
#include "akey.h"
#include "ui.h"
#include "platform.h"

#define FALSE 0

/* a runtime switch for the kbd_joy_X_enabled vars is in the UI */
int PLATFORM_kbd_joy_0_enabled = TRUE;  /* enabled by default, doesn't hurt */
int PLATFORM_kbd_joy_1_enabled = FALSE; /* disabled, would steal normal keys */

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

static Uint8 is_shift = 0;
static Uint8 is_ctrl = 0;

int control_shifted(int a);

int sdl_keysum2scan_up(SDL_Keycode sym)
{
    int keycode = AKEY_NONE;
    switch (sym)
    {
    case SDLK_LSHIFT:
    case SDLK_RSHIFT:
        is_shift = FALSE;
        break;
    case SDLK_LCTRL:
    case SDLK_RCTRL:
        is_ctrl = FALSE;
        break;
    case SDLK_F2:
        INPUT_key_consol |= INPUT_CONSOL_OPTION;
        break;

    case SDLK_F3:
        INPUT_key_consol |= INPUT_CONSOL_SELECT;
        break;

    case SDLK_F4:
        INPUT_key_consol |= INPUT_CONSOL_START;
        break;

    default:
        break;
    }

    return keycode;
}

int control_shifted(int a)
{
    if (is_ctrl)
    {
        return (a + (64 * is_shift) + (128 * is_ctrl));
    }
    return AKEY_NONE;
}

int sdl_keysum2scan_down(SDL_Keycode sym)
{
    int keycode = AKEY_NONE;
    int shiftctrl = 0;
    if (is_shift)
        shiftctrl ^= AKEY_SHFT;
    if (is_ctrl)
        shiftctrl ^= AKEY_CTRL;

    switch (sym)
    {
    case SDLK_LSHIFT:
    case SDLK_RSHIFT:
        is_shift = TRUE;
        break;
    case SDLK_LCTRL:
    case SDLK_RCTRL:
        is_ctrl = TRUE;
        break;
    case SDLK_RETURN:
        keycode = AKEY_RETURN ^ shiftctrl;
        break;
    case SDLK_LEFT:
        keycode = (!UI_is_active && Atari800_f_keys ? AKEY_F3 : (is_shift ? AKEY_PLUS : AKEY_LEFT)) ^ shiftctrl;
        break;
    case SDLK_RIGHT:
        keycode = (!UI_is_active && Atari800_f_keys ? AKEY_F4 : (is_shift ? AKEY_ASTERISK : AKEY_RIGHT)) ^ shiftctrl;
        break;
    case SDLK_UP:
        keycode = (!UI_is_active && Atari800_f_keys ? AKEY_F1 : (is_shift ? AKEY_MINUS : AKEY_UP)) ^ shiftctrl;
        break;
    case SDLK_DOWN:
        keycode = (!UI_is_active && Atari800_f_keys ? AKEY_F2 : (is_shift ? AKEY_EQUAL : AKEY_DOWN)) ^ shiftctrl;
        break;

    case SDLK_F1:
        keycode = AKEY_UI;
        break;

    case SDLK_F2:
        INPUT_key_consol &= ~INPUT_CONSOL_OPTION;
        keycode = AKEY_OPTION;
        break;

    case SDLK_F3:
        INPUT_key_consol &= ~INPUT_CONSOL_SELECT;
        keycode = AKEY_SELECT;
        break;

    case SDLK_F4:
        INPUT_key_consol &= ~INPUT_CONSOL_START;
        keycode = AKEY_START;
        break;

    case SDLK_F5:
        keycode = is_shift ? AKEY_COLDSTART : AKEY_WARMSTART;
        break;

    case SDLK_END:
    case SDLK_F6:
        keycode = AKEY_HELP ^ shiftctrl;

    case SDLK_F7:
        keycode = AKEY_BREAK;
        break;

    case SDLK_F8:
        UI_alt_function = UI_MENU_MONITOR;
        break;

    case SDLK_F9:
        keycode = AKEY_EXIT;
        break;

    case SDLK_F10:
        keycode = is_shift ? AKEY_SCREENSHOT_INTERLACE : AKEY_SCREENSHOT;
        break;

    case SDLK_F12:
        keycode = AKEY_TURBO;
        break;

    case SDLK_PAGEDOWN:
        keycode = AKEY_F2 | AKEY_SHFT;
        break;

    case SDLK_PAGEUP:
        keycode = AKEY_F1 | AKEY_SHFT;
        break;

    case SDLK_HOME:
        keycode = is_ctrl ? AKEY_LESS | shiftctrl : AKEY_CLEAR;
        break;

    case SDLK_DELETE:
        keycode = is_shift ? AKEY_DELETE_LINE | shiftctrl : AKEY_DELETE_CHAR;
        break;

    case SDLK_INSERT:
        keycode = is_shift ? AKEY_INSERT_LINE | shiftctrl : AKEY_INSERT_CHAR;
        break;

    case SDLK_CAPSLOCK:
        keycode = is_shift ? AKEY_CAPSTOGGLE | shiftctrl : AKEY_CAPSTOGGLE | shiftctrl;
        break;

        /* work-around for windows */
    case SDLK_BACKSLASH:
        keycode = AKEY_ESCAPE | shiftctrl;
        break;

    case SDLK_TAB:
        keycode = AKEY_TAB ^ shiftctrl;
        break;

    case SDLK_BACKSPACE:
        keycode = AKEY_BACKSPACE | shiftctrl;
        break;

    case SDLK_a:
        keycode = control_shifted(AKEY_a);
        break;
    case SDLK_b:
        keycode = control_shifted(AKEY_b);
        break;
    case SDLK_c:
        keycode = control_shifted(AKEY_c);
        break;
    case SDLK_d:
        keycode = control_shifted(AKEY_d);
        break;
    case SDLK_e:
        keycode = control_shifted(AKEY_e);
        break;
    case SDLK_f:
        keycode = control_shifted(AKEY_f);
        break;
    case SDLK_g:
        keycode = control_shifted(AKEY_g);
        break;
    case SDLK_h:
        keycode = control_shifted(AKEY_h);
        break;
    case SDLK_i:
        keycode = control_shifted(AKEY_i);
        break;
    case SDLK_j:
        keycode = control_shifted(AKEY_j);
        break;
    case SDLK_k:
        keycode = control_shifted(AKEY_k);
        break;
    case SDLK_l:
        keycode = control_shifted(AKEY_l);
        break;
    case SDLK_m:
        keycode = control_shifted(AKEY_m);
        break;
    case SDLK_n:
        keycode = control_shifted(AKEY_n);
        break;
    case SDLK_o:
        keycode = control_shifted(AKEY_o);
        break;
    case SDLK_p:
        keycode = control_shifted(AKEY_p);
        break;
    case SDLK_q:
        keycode = control_shifted(AKEY_q);
        break;
    case SDLK_r:
        keycode = control_shifted(AKEY_r);
        break;
    case SDLK_s:
        keycode = control_shifted(AKEY_s);
        break;
    case SDLK_t:
        keycode = control_shifted(AKEY_t);
        break;
    case SDLK_u:
        keycode = control_shifted(AKEY_u);
        break;
    case SDLK_v:
        keycode = control_shifted(AKEY_v);
        break;
    case SDLK_w:
        keycode = control_shifted(AKEY_w);
        break;
    case SDLK_x:
        keycode = control_shifted(AKEY_x);
        break;
    case SDLK_y:
        keycode = control_shifted(AKEY_y);
        break;
    case SDLK_z:
        keycode = control_shifted(AKEY_z);
        break;
    case SDLK_ESCAPE:
        keycode = AKEY_ESCAPE | shiftctrl;
        break;
    case SDLK_BACKQUOTE:
    /* Fallthrough */
    case SDLK_LGUI:
        keycode = AKEY_ATARI ^ shiftctrl;
        break;
    case SDLK_RGUI:
        keycode = is_shift ? AKEY_CAPSLOCK : AKEY_CAPSTOGGLE;
        break;
    }

    return keycode;
}

int ascii2scan(int code)
{

    int keycode;
    int shiftctrl = 0;
    if (is_shift)
        shiftctrl ^= AKEY_SHFT;
    if (is_ctrl)
        shiftctrl ^= AKEY_CTRL;

    switch (code)
    {
    case '0':
        keycode = AKEY_0;
        break;
    case '1':
        keycode = AKEY_1;
        break;
    case '2':
        keycode = AKEY_2;
        break;
    case '3':
        keycode = AKEY_3;
        break;
    case '4':
        keycode = AKEY_4;
        break;
    case '5':
        keycode = AKEY_5;
        break;
    case '6':
        keycode = AKEY_6;
        break;
    case '7':
        keycode = AKEY_7;
        break;
    case '8':
        keycode = AKEY_8;
        break;
    case '9':
        keycode = AKEY_9;
        break;
    case 'A':
    case 'a':
        keycode = AKEY_a;
        break;
    case 'B':
    case 'b':
        keycode = AKEY_b;
        break;
    case 'C':
    case 'c':
        keycode = AKEY_c;
        break;
    case 'D':
    case 'd':
        keycode = AKEY_d;
        break;
    case 'E':
    case 'e':
        keycode = AKEY_e;
        break;
    case 'F':
    case 'f':
        keycode = AKEY_f;
        break;
    case 'G':
    case 'g':
        keycode = AKEY_g;
        break;
    case 'H':
    case 'h':
        keycode = AKEY_h;
        break;
    case 'I':
    case 'i':
        keycode = AKEY_i;
        break;
    case 'J':
    case 'j':
        keycode = AKEY_j;
        break;
    case 'K':
    case 'k':
        keycode = AKEY_k;
        break;
    case 'L':
    case 'l':
        keycode = AKEY_l;
        break;
    case 'M':
    case 'm':
        keycode = AKEY_m;
        break;
    case 'N':
    case 'n':
        keycode = AKEY_n;
        break;
    case 'O':
    case 'o':
        keycode = AKEY_o;
        break;
    case 'P':
    case 'p':
        keycode = AKEY_p;
        break;
    case 'Q':
    case 'q':
        keycode = AKEY_q;
        break;
    case 'R':
    case 'r':
        keycode = AKEY_r;
        break;
    case 'S':
    case 's':
        keycode = AKEY_s;
        break;
    case 'T':
    case 't':
        keycode = AKEY_t;
        break;
    case 'U':
    case 'u':
        keycode = AKEY_u;
        break;
    case 'V':
    case 'v':
        keycode = AKEY_v;
        break;
    case 'W':
    case 'w':
        keycode = AKEY_w;
        break;
    case 'X':
    case 'x':
        keycode = AKEY_x;
        break;
    case 'Y':
    case 'y':
        keycode = AKEY_y;
        break;
    case 'Z':
    case 'z':
        keycode = AKEY_z;
        break;
    case ' ':
        keycode = AKEY_SPACE ^ shiftctrl;
        break;
    case '\t':
        keycode = AKEY_TAB;
        break;
    case '!':
        keycode = AKEY_EXCLAMATION;
        break;
    case '"':
        keycode = AKEY_DBLQUOTE;
        break;
    case '#':
        keycode = AKEY_HASH;
        break;
    case '$':
        keycode = AKEY_DOLLAR;
        break;
    case '%':
        keycode = AKEY_PERCENT;
        break;
    case '&':
        keycode = AKEY_AMPERSAND;
        break;
    case '\'':
        keycode = AKEY_QUOTE;
        break;
    case '@':
        keycode = AKEY_AT;
        break;
    case '(':
        keycode = AKEY_PARENLEFT;
        break;
    case ')':
        keycode = AKEY_PARENRIGHT;
        break;
    case '<':
        keycode = AKEY_LESS;
        break;
    case '>':
        keycode = AKEY_GREATER;
        break;
    case '=':
        keycode = AKEY_EQUAL;
        break;
    case '?':
        keycode = AKEY_QUESTION;
        break;
    case '-':
        keycode = AKEY_MINUS;
        break;
    case '+':
        keycode = AKEY_PLUS;
        break;
    case '*':
        keycode = AKEY_ASTERISK;
        break;
    case '/':
        keycode = AKEY_SLASH;
        break;
    case ':':
        keycode = AKEY_COLON;
        break;
    case ';':
        keycode = AKEY_SEMICOLON;
        break;
    case ',':
        keycode = AKEY_COMMA;
        break;
    case '.':
        keycode = AKEY_FULLSTOP | shiftctrl;
        break;
    case '_':
        keycode = AKEY_UNDERSCORE;
        break;
    case '[':
        keycode = AKEY_BRACKETLEFT;
        break;
    case ']':
        keycode = AKEY_BRACKETRIGHT;
        break;
    case '^':
        keycode = AKEY_CIRCUMFLEX;
        break;
    case '\\':
        keycode = AKEY_BACKSLASH;
        break;
    case '|':
        keycode = AKEY_BAR;
        break;
    default:
        keycode = AKEY_NONE;
        break;
    }

    return keycode;
}
