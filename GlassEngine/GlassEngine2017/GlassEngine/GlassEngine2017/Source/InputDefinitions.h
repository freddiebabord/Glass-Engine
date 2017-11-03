#pragma once

#define SDL_MAIN_HANDLED
#include <SDL/SDL.h>
#undef main
#define KEYBOARD_SIZE 282

enum Key
{
	KEYCODE_UNKNOWN = SDL_SCANCODE_UNKNOWN,
	KEYCODE_A = SDL_SCANCODE_A,
	KEYCODE_B = SDL_SCANCODE_B,
	KEYCODE_C = SDL_SCANCODE_C,
	KEYCODE_D = SDL_SCANCODE_D,
	KEYCODE_E = SDL_SCANCODE_E,
	KEYCODE_F = SDL_SCANCODE_F,
	KEYCODE_G = SDL_SCANCODE_G,
	KEYCODE_H = SDL_SCANCODE_H,
	KEYCODE_I = SDL_SCANCODE_I,
	KEYCODE_J = SDL_SCANCODE_J,
	KEYCODE_K = SDL_SCANCODE_K,
	KEYCODE_L = SDL_SCANCODE_L,
	KEYCODE_M = SDL_SCANCODE_M,
	KEYCODE_N = SDL_SCANCODE_N,
	KEYCODE_O = SDL_SCANCODE_O,
	KEYCODE_P = SDL_SCANCODE_P,
	KEYCODE_Q = SDL_SCANCODE_Q,
	KEYCODE_R = SDL_SCANCODE_R,
	KEYCODE_S = SDL_SCANCODE_S,
	KEYCODE_T = SDL_SCANCODE_T,
	KEYCODE_U = SDL_SCANCODE_U,
	KEYCODE_V = SDL_SCANCODE_V,
	KEYCODE_W = SDL_SCANCODE_W,
	KEYCODE_X = SDL_SCANCODE_X,
	KEYCODE_Y = SDL_SCANCODE_Y,
	KEYCODE_Z = SDL_SCANCODE_Z,
	KEYCODE_1 = SDL_SCANCODE_1,
	KEYCODE_2 = SDL_SCANCODE_2,
	KEYCODE_3 = SDL_SCANCODE_3,
	KEYCODE_4 = SDL_SCANCODE_4,
	KEYCODE_5 = SDL_SCANCODE_5,
	KEYCODE_6 = SDL_SCANCODE_6,
	KEYCODE_7 = SDL_SCANCODE_7,
	KEYCODE_8 = SDL_SCANCODE_8,
	KEYCODE_9 = SDL_SCANCODE_9,
	KEYCODE_0 = SDL_SCANCODE_0,
	KEYCODE_RETURN = SDL_SCANCODE_RETURN,
	KEYCODE_ESCAPE = SDL_SCANCODE_ESCAPE,
	KEYCODE_BACKSPACE = SDL_SCANCODE_BACKSPACE,
	KEYCODE_TAB = SDL_SCANCODE_TAB,
	KEYCODE_SPACE = SDL_SCANCODE_SPACE,
	KEYCODE_MINUS = SDL_SCANCODE_MINUS,
	KEYCODE_EQUALS = SDL_SCANCODE_EQUALS,
	KEYCODE_LEFTBRACKET = SDL_SCANCODE_LEFTBRACKET,
	KEYCODE_RIGHTBRACKET = SDL_SCANCODE_RIGHTBRACKET,
	KEYCODE_BACKSLASH = SDL_SCANCODE_BACKSLASH,
	KEYCODE_NONUSHASH = SDL_SCANCODE_NONUSHASH,
	KEYCODE_SEMICOLON = SDL_SCANCODE_SEMICOLON,
	KEYCODE_APOSTROPHE = SDL_SCANCODE_APOSTROPHE,
	KEYCODE_GRAVE = SDL_SCANCODE_GRAVE,
	KEYCODE_COMMA = SDL_SCANCODE_COMMA,
	KEYCODE_PERIOD = SDL_SCANCODE_PERIOD,
	KEYCODE_SLASH = SDL_SCANCODE_SLASH,
	KEYCODE_CAPSLOCK = SDL_SCANCODE_CAPSLOCK,
	KEYCODE_F1 = SDL_SCANCODE_F1,
	KEYCODE_F2 = SDL_SCANCODE_F2,
	KEYCODE_F3 = SDL_SCANCODE_F3,
	KEYCODE_F4 = SDL_SCANCODE_F4,
	KEYCODE_F5 = SDL_SCANCODE_F5,
	KEYCODE_F6 = SDL_SCANCODE_F6,
	KEYCODE_F7 = SDL_SCANCODE_F7,
	KEYCODE_F8 = SDL_SCANCODE_F8,
	KEYCODE_F9 = SDL_SCANCODE_F9,
	KEYCODE_F10 = SDL_SCANCODE_F10,
	KEYCODE_F11 = SDL_SCANCODE_F11,
	KEYCODE_F12 = SDL_SCANCODE_F12,
	KEYCODE_PRINTSCREEN = SDL_SCANCODE_PRINTSCREEN,
	KEYCODE_SCROLLLOCK = SDL_SCANCODE_SCROLLLOCK,
	KEYCODE_PAUSE = SDL_SCANCODE_PAUSE,
	KEYCODE_INSERT = SDL_SCANCODE_INSERT,
	KEYCODE_HOME = SDL_SCANCODE_HOME,
	KEYCODE_PAGEUP = SDL_SCANCODE_PAGEUP,
	KEYCODE_DELETE = SDL_SCANCODE_DELETE,
	KEYCODE_END = SDL_SCANCODE_END,
	KEYCODE_PAGEDOWN = SDL_SCANCODE_PAGEDOWN,
	KEYCODE_RIGHT = SDL_SCANCODE_RIGHT,
	KEYCODE_LEFT = SDL_SCANCODE_LEFT,
	KEYCODE_DOWN = SDL_SCANCODE_DOWN,
	KEYCODE_UP = SDL_SCANCODE_UP,
	KEYCODE_NUMLOCKCLEAR = SDL_SCANCODE_NUMLOCKCLEAR,
	KEYCODE_KEYPAD_DIVIDE = SDL_SCANCODE_KP_DIVIDE,
	KEYCODE_KEYPAD_MULTIPLY = SDL_SCANCODE_KP_MULTIPLY,
	KEYCODE_KEYPAD_MINUS = SDL_SCANCODE_KP_MINUS,
	KEYCODE_KEYPAD_PLUS = SDL_SCANCODE_KP_PLUS,
	KEYCODE_KEYPAD_ENTER = SDL_SCANCODE_KP_ENTER,
	KEYCODE_KEYPAD_1 = SDL_SCANCODE_KP_1,
	KEYCODE_KEYPAD_2 = SDL_SCANCODE_KP_2,
	KEYCODE_KEYPAD_3 = SDL_SCANCODE_KP_3,
	KEYCODE_KEYPAD_4 = SDL_SCANCODE_KP_4,
	KEYCODE_KEYPAD_5 = SDL_SCANCODE_KP_5,
	KEYCODE_KEYPAD_6 = SDL_SCANCODE_KP_6,
	KEYCODE_KEYPAD_7 = SDL_SCANCODE_KP_7,
	KEYCODE_KEYPAD_8 = SDL_SCANCODE_KP_8,
	KEYCODE_KEYPAD_9 = SDL_SCANCODE_KP_9,
	KEYCODE_KEYPAD_0 = SDL_SCANCODE_KP_0,
	KEYCODE_KEYPAD_PERIOD = SDL_SCANCODE_KP_PERIOD,
	KEYCODE_NONUSBACKSLASH = SDL_SCANCODE_NONUSBACKSLASH,
	KEYCODE_APPLICATION = SDL_SCANCODE_APPLICATION,
	KEYCODE_POWER = SDL_SCANCODE_POWER,
	KEYCODE_KEYPAD_EQUALS = SDL_SCANCODE_KP_EQUALS,
	KEYCODE_F13 = SDL_SCANCODE_F13,
	KEYCODE_F14 = SDL_SCANCODE_F14,
	KEYCODE_F15 = SDL_SCANCODE_F15,
	KEYCODE_F16 = SDL_SCANCODE_F16,
	KEYCODE_F17 = SDL_SCANCODE_F17,
	KEYCODE_F18 = SDL_SCANCODE_F18,
	KEYCODE_F19 = SDL_SCANCODE_F19,
	KEYCODE_F20 = SDL_SCANCODE_F20,
	KEYCODE_F21 = SDL_SCANCODE_F21,
	KEYCODE_F22 = SDL_SCANCODE_F22,
	KEYCODE_F23 = SDL_SCANCODE_F23,
	KEYCODE_F24 = SDL_SCANCODE_F24,
	KEYCODE_EXECUTE = SDL_SCANCODE_EXECUTE,
	KEYCODE_HELP = SDL_SCANCODE_HELP,
	KEYCODE_MENU = SDL_SCANCODE_MENU,
	KEYCODE_SELECT = SDL_SCANCODE_SELECT,
	KEYCODE_STOP = SDL_SCANCODE_STOP,
	KEYCODE_AGAIN = SDL_SCANCODE_AGAIN,
	KEYCODE_UNDO = SDL_SCANCODE_UNDO,
	KEYCODE_CUT = SDL_SCANCODE_CUT,
	KEYCODE_COPY = SDL_SCANCODE_COPY,
	KEYCODE_PASTE = SDL_SCANCODE_PASTE,
	KEYCODE_FIND = SDL_SCANCODE_FIND,
	KEYCODE_MUTE = SDL_SCANCODE_MUTE,
	KEYCODE_VOLUMEUP = SDL_SCANCODE_VOLUMEUP,
	KEYCODE_VOLUMEDOWN = SDL_SCANCODE_VOLUMEDOWN,

	// They doesn't seem to exist...!
	// KEY_LOCKINGCAPSLOCK       = SDL_SCANCODE_LOCKINGCAPSLOCK,
	// KEY_LOCKINGNUMLOCK        = SDL_SCANCODE_LOCKINGNUMLOCK,
	// KEY_LOCKINGSCROLLLOCK     = SDL_SCANCODE_LOCKINGSCROLLLOCK,

	KEYCODE_KEYPAD_COMMA = SDL_SCANCODE_KP_COMMA,
	KEYCODE_KEYPAD_EQUALSAS400 = SDL_SCANCODE_KP_EQUALSAS400,
	KEYCODE_INTERNATIONAL1 = SDL_SCANCODE_INTERNATIONAL1,
	KEYCODE_INTERNATIONAL2 = SDL_SCANCODE_INTERNATIONAL2,
	KEYCODE_INTERNATIONAL3 = SDL_SCANCODE_INTERNATIONAL3,
	KEYCODE_INTERNATIONAL4 = SDL_SCANCODE_INTERNATIONAL4,
	KEYCODE_INTERNATIONAL5 = SDL_SCANCODE_INTERNATIONAL5,
	KEYCODE_INTERNATIONAL6 = SDL_SCANCODE_INTERNATIONAL6,
	KEYCODE_INTERNATIONAL7 = SDL_SCANCODE_INTERNATIONAL7,
	KEYCODE_INTERNATIONAL8 = SDL_SCANCODE_INTERNATIONAL8,
	KEYCODE_INTERNATIONAL9 = SDL_SCANCODE_INTERNATIONAL9,
	KEYCODE_LANG1 = SDL_SCANCODE_LANG1,
	KEYCODE_LANG2 = SDL_SCANCODE_LANG2,
	KEYCODE_LANG3 = SDL_SCANCODE_LANG3,
	KEYCODE_LANG4 = SDL_SCANCODE_LANG4,
	KEYCODE_LANG5 = SDL_SCANCODE_LANG5,
	KEYCODE_LANG6 = SDL_SCANCODE_LANG6,
	KEYCODE_LANG7 = SDL_SCANCODE_LANG7,
	KEYCODE_LANG8 = SDL_SCANCODE_LANG8,
	KEYCODE_LANG9 = SDL_SCANCODE_LANG9,
	KEYCODE_ALTERASE = SDL_SCANCODE_ALTERASE,
	KEYCODE_SYSREQ = SDL_SCANCODE_SYSREQ,
	KEYCODE_CANCEL = SDL_SCANCODE_CANCEL,
	KEYCODE_CLEAR = SDL_SCANCODE_CLEAR,
	KEYCODE_PRIOR = SDL_SCANCODE_PRIOR,
	KEYCODE_RETURN2 = SDL_SCANCODE_RETURN2,
	KEYCODE_SEPARATOR = SDL_SCANCODE_SEPARATOR,
	KEYCODE_OUT = SDL_SCANCODE_OUT,
	KEYCODE_OPER = SDL_SCANCODE_OPER,
	KEYCODE_CLEARAGAIN = SDL_SCANCODE_CLEARAGAIN,
	KEYCODE_CRSEL = SDL_SCANCODE_CRSEL,
	KEYCODE_EXSEL = SDL_SCANCODE_EXSEL,
	KEYCODE_KEYPAD_00 = SDL_SCANCODE_KP_00,
	KEYCODE_KEYPAD_000 = SDL_SCANCODE_KP_000,
	KEYCODE_THOUSANDSSEPARATOR = SDL_SCANCODE_THOUSANDSSEPARATOR,
	KEYCODE_DECIMALSEPARATOR = SDL_SCANCODE_DECIMALSEPARATOR,
	KEYCODE_CURRENCYUNIT = SDL_SCANCODE_CURRENCYUNIT,
	KEYCODE_CURRENCYSUBUNIT = SDL_SCANCODE_CURRENCYSUBUNIT,
	KEYCODE_KEYPAD_LEFTPAREN = SDL_SCANCODE_KP_LEFTPAREN,
	KEYCODE_KEYPAD_RIGHTPAREN = SDL_SCANCODE_KP_RIGHTPAREN,
	KEYCODE_KEYPAD_LEFTBRACE = SDL_SCANCODE_KP_LEFTBRACE,
	KEYCODE_KEYPAD_RIGHTBRACE = SDL_SCANCODE_KP_RIGHTBRACE,
	KEYCODE_KEYPAD_TAB = SDL_SCANCODE_KP_TAB,
	KEYCODE_KEYPAD_BACKSPACE = SDL_SCANCODE_KP_BACKSPACE,
	KEYCODE_KEYPAD_A = SDL_SCANCODE_KP_A,
	KEYCODE_KEYPAD_B = SDL_SCANCODE_KP_B,
	KEYCODE_KEYPAD_C = SDL_SCANCODE_KP_C,
	KEYCODE_KEYPAD_D = SDL_SCANCODE_KP_D,
	KEYCODE_KEYPAD_E = SDL_SCANCODE_KP_E,
	KEYCODE_KEYPAD_F = SDL_SCANCODE_KP_F,
	KEYCODE_KEYPAD_XOR = SDL_SCANCODE_KP_XOR,
	KEYCODE_KEYPAD_POWER = SDL_SCANCODE_KP_POWER,
	KEYCODE_KEYPAD_PERCENT = SDL_SCANCODE_KP_PERCENT,
	KEYCODE_KEYPAD_LESS = SDL_SCANCODE_KP_LESS,
	KEYCODE_KEYPAD_GREATER = SDL_SCANCODE_KP_GREATER,
	KEYCODE_KEYPAD_AMPERSAND = SDL_SCANCODE_KP_AMPERSAND,
	KEYCODE_KEYPAD_DBLAMPERSAND = SDL_SCANCODE_KP_DBLAMPERSAND,
	KEYCODE_KEYPAD_VERTICALBAR = SDL_SCANCODE_KP_VERTICALBAR,
	KEYCODE_KEYPAD_DBLVERTICALBAR = SDL_SCANCODE_KP_DBLVERTICALBAR,
	KEYCODE_KEYPAD_COLON = SDL_SCANCODE_KP_COLON,
	KEYCODE_KEYPAD_HASH = SDL_SCANCODE_KP_HASH,
	KEYCODE_KEYPAD_SPACE = SDL_SCANCODE_KP_SPACE,
	KEYCODE_KEYPAD_AT = SDL_SCANCODE_KP_AT,
	KEYCODE_KEYPAD_EXCLAM = SDL_SCANCODE_KP_EXCLAM,
	KEYCODE_KEYPAD_MEMSTORE = SDL_SCANCODE_KP_MEMSTORE,
	KEYCODE_KEYPAD_MEMRECALL = SDL_SCANCODE_KP_MEMRECALL,
	KEYCODE_KEYPAD_MEMCLEAR = SDL_SCANCODE_KP_MEMCLEAR,
	KEYCODE_KEYPAD_MEMADD = SDL_SCANCODE_KP_MEMADD,
	KEYCODE_KEYPAD_MEMSUBTRACT = SDL_SCANCODE_KP_MEMSUBTRACT,
	KEYCODE_KEYPAD_MEMMULTIPLY = SDL_SCANCODE_KP_MEMMULTIPLY,
	KEYCODE_KEYPAD_MEMDIVIDE = SDL_SCANCODE_KP_MEMDIVIDE,
	KEYCODE_KEYPAD_PLUSMINUS = SDL_SCANCODE_KP_PLUSMINUS,
	KEYCODE_KEYPAD_CLEAR = SDL_SCANCODE_KP_CLEAR,
	KEYCODE_KEYPAD_CLEARENTRY = SDL_SCANCODE_KP_CLEARENTRY,
	KEYCODE_KEYPAD_BINARY = SDL_SCANCODE_KP_BINARY,
	KEYCODE_KEYPAD_OCTAL = SDL_SCANCODE_KP_OCTAL,
	KEYCODE_KEYPAD_DECIMAL = SDL_SCANCODE_KP_DECIMAL,
	KEYCODE_KEYPAD_HEXADECIMAL = SDL_SCANCODE_KP_HEXADECIMAL,
	KEYCODE_LEFT_CTRL = SDL_SCANCODE_LCTRL,
	KEYCODE_LEFT_SHIFT = SDL_SCANCODE_LSHIFT,
	KEYCODE_LEFT_ALT = SDL_SCANCODE_LALT,
	KEYCODE_LEFT_GUI = SDL_SCANCODE_LGUI,
	KEYCODE_RIGHT_CTRL = SDL_SCANCODE_RCTRL,
	KEYCODE_RIGHT_SHIFT = SDL_SCANCODE_RSHIFT,
	KEYCODE_RIGHT_ALT = SDL_SCANCODE_RALT,
	KEYCODE_RIGHT_GUI = SDL_SCANCODE_RGUI,
	KEYCODE_MODE = SDL_SCANCODE_MODE,
	KEYCODE_AUDIONEXT = SDL_SCANCODE_AUDIONEXT,
	KEYCODE_AUDIOPREV = SDL_SCANCODE_AUDIOPREV,
	KEYCODE_AUDIOSTOP = SDL_SCANCODE_AUDIOSTOP,
	KEYCODE_AUDIOPLAY = SDL_SCANCODE_AUDIOPLAY,
	KEYCODE_AUDIOMUTE = SDL_SCANCODE_AUDIOMUTE,
	KEYCODE_MEDIASELECT = SDL_SCANCODE_MEDIASELECT,
	KEYCODE_WWW = SDL_SCANCODE_WWW,
	KEYCODE_MAIL = SDL_SCANCODE_MAIL,
	KEYCODE_CALCULATOR = SDL_SCANCODE_CALCULATOR,
	KEYCODE_COMPUTER = SDL_SCANCODE_COMPUTER,
	KEYCODE_AC_SEARCH = SDL_SCANCODE_AC_SEARCH,
	KEYCODE_AC_HOME = SDL_SCANCODE_AC_HOME,
	KEYCODE_AC_BACK = SDL_SCANCODE_AC_BACK,
	KEYCODE_AC_FORWARD = SDL_SCANCODE_AC_FORWARD,
	KEYCODE_AC_STOP = SDL_SCANCODE_AC_STOP,
	KEYCODE_AC_REFRESH = SDL_SCANCODE_AC_REFRESH,
	KEYCODE_AC_BOOKMARKS = SDL_SCANCODE_AC_BOOKMARKS,
	KEYCODE_BRIGHTNESSDOWN = SDL_SCANCODE_BRIGHTNESSDOWN,
	KEYCODE_BRIGHTNESSUP = SDL_SCANCODE_BRIGHTNESSUP,
	KEYCODE_DISPLAYSWITCH = SDL_SCANCODE_DISPLAYSWITCH,
	KEYCODE_KBDILLUMTOGGLE = SDL_SCANCODE_KBDILLUMTOGGLE,
	KEYCODE_KBDILLUMDOWN = SDL_SCANCODE_KBDILLUMDOWN,
	KEYCODE_KBDILLUMUP = SDL_SCANCODE_KBDILLUMUP,
	KEYCODE_EJECT = SDL_SCANCODE_EJECT,
	KEYCODE_SLEEP = SDL_SCANCODE_SLEEP
};

/// All mouse buttons (except for the middle one).
enum MouseButton
{
	MOUSE_LEFT,
	MOUSE_MIDDLE,
	MOUSE_RIGHT,

	MOUSE_MAX   // No button, just to define max
				// array size.
};
