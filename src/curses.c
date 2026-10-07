/*
 *  Cursor motion stuff to simulate a "no refresh" version of curses
 */

#include	"extern.h"

#ifndef ROGUE_DOS_CURSES


/*@
 * Many references on the 'net suggest including <ncursesw/curses.h> directly,
 * but this is a task for the Makefile/build system, not source code.
 *
 * <curses.h> will set _XOPEN_CURSES if wide chars are available
 * https://pubs.opengroup.org/onlinepubs/7908799/xcurses/curses.h.html
 * https://publications.opengroup.org/c094
 */
#undef _XOPEN_CURSES
#include	<curses.h>
#endif  // not ROGUE_DOS_CURSES

#include	"curses_common.h"
#include	"curses_dos.h"
#include	"keypad.h"


/*
 *  Globals for curses
 *  (extern'ed in curses.h)
 */
int screen_updates_suspended = FALSE;  //@ in practice, TRUE disables status updates in update_keyboard_and_clock()
int dos_screen_mode = -1;
#ifdef ROGUE_DOS_CURSES
int LINES=25, COLS=80;
bool iscuron = TRUE;
int old_page_no;  //@ this is public, but page_no is not. Weird. See rip.c
int scr_ds=0xB800;
int svwin_ds = 0;
#else
// current terminal size as reported by curses. Will change on window resize
extern int LINES, COLS;

/*
 * The following are not used by the application, so not really extern'ed in
 * curses.h. But they could be, and some like change_colors should be set via
 * env file / options
 */

// Terminal size we *want*, not necessarily what we will get
int game_screen_rows = min(25, MAXLINES);
int game_screen_columns  = min(ROGUE_COLUMNS, MAXCOLS);

// if curses is initialized or not. If extern'ed, should be read-only
bool screen_initialized = FALSE;

/* Charset used. Could be initially set via env file, but should not be changed
 * mid-game unless we create a function to re-draw the screen. The code should
 * already graciously fallback to ASCII if UNICODE is requested here but not
 * available, either because it was not compiled with wide char support or the
 * current curses implementation does not support it. Both cases are tested
 * with _XOPEN_CURSES. ROGUE_CHARSET is a factory default that already accounts
 * for Unicode availability and compile-time options.
 */
int	character_set = ROGUE_CHARSET;

/*
 * Number of colors we're working with, regardless if terminal has more colors
 * available. This is set by init_curses_colors() and should be the result of
 * many factors, not only terminal COLORS reported by curses but also `screen`
 * env file setting (for bw), ROGUE_SCR_TYPE, etc.
 *
 * Values can be:
 * -  0 for monochrome
 * -  8 for 8 basic screen_color_count (light versions will use BOLD text attribute)
 * - 16 if all DOS colors are directly indexable
 *
 * If extern'ed, should obviously be read-only
 */
int 	screen_color_count;

// if user allows us to redefine color palette to match original RGB
bool change_colors = TRUE;

// if user wants to use default terminal foreground / background color
bool use_terminal_fgbg = TRUE;
#endif

//@ unused
int tab_size = 8;

//@ private
static int current_dos_attribute = A_DOS_NORMAL;
#ifdef ROGUE_DOS_CURSES
static int page_no = 0;
static int c_row, c_col;   /*  Save cursor positions so we don't ask dos */
static int scr_row[25];
static int no_check = FALSE;  //@ do not wait for video retrace. Former extern
#else
#ifdef ROGUE_WIDECHAR
static cchar_t  curtain[MAXLINES][MAXCOLS + 1];
static cchar_t  temporary_screen_cell;
#else
static chtype   curtain[MAXLINES][MAXCOLS + 1];  // temp buffer for curtain animations
#endif // ROGUE_WIDECHAR
static int	TERMINAL_KEY_MASK;
static wchar_t	fallback_unicode[2] = L" ";  // temp buffer
static CharacterMapping	fallback_character_mapping = {'\0', fallback_unicode, '\0'};  // temp charcode
static bool	colors_changed = FALSE;  // if colors palette was redefined
#endif  // ROGUE_DOS_CURSES

/*
 * Screen snapshots are used by overlays and the disabled legacy save implementation.
 * The native representation contains curses cells; it is not a portable save format.
 */
#if   defined (ROGUE_DOS_CURSES)
char saved_screen[2048 * sizeof(chtype)];  //@ originally 4096 bytes
#elif defined (ROGUE_WIDECHAR)
cchar_t	saved_screen[MAXLINES][MAXCOLS + 1];  // temp buffer to hold screen contents
#else
chtype	saved_screen[MAXLINES][MAXCOLS + 1];  // temp buffer to hold screen contents
#endif

/*@
 * Original used decimal literals for both tables
 */
#define MAXATTR 17
static byte color_attributes[] = {
	A_DOS_NORMAL,                  /*  0 normal         */
	A_DOS_GREEN,                   /*  1 green          */
	A_DOS_CYAN,                    /*  2 cyan           */
	A_DOS_RED,                     /*  3 red            */
	A_DOS_MAGENTA,                 /*  4 magenta        */
	A_DOS_BROWN,                   /*  5 brown          */
	A_DOS_BRIGHT | A_DOS_BLACK,    /*  6 dark grey      */
	A_DOS_BRIGHT | A_DOS_BLUE,     /*  7 light blue     */
	A_DOS_BRIGHT | A_DOS_GREEN,    /*  8 light green    */
	A_DOS_BRIGHT | A_DOS_RED,      /*  9 light red      */
	A_DOS_BRIGHT | A_DOS_MAGENTA,  /* 10 light magenta  */
	A_DOS_BRIGHT | A_DOS_BROWN,    /* 11 yellow         */
	A_DOS_BRIGHT | A_DOS_WHITE,    /* 12 uline          */
	A_DOS_BLUE,                    /* 13 blue           */
	A_DOS_STANDOUT,                /* 14 reverse        */
	A_DOS_BRIGHT | A_DOS_NORMAL,   /* 15 high intensity */
	A_DOS_STANDOUT,                /* bold              */
	0                              /* no more           */
} ;

/*
 * The monochrome table uses bright reverse video for reverse/bold and normal white
 * for high intensity. screen_write_character performs its terrain-color remapping
 * only when active_attributes == color_attributes, so monochrome output bypasses that
 * remapping regardless of the exact reverse-video byte.
 */
static byte monochrome_attributes[] = {
	A_DOS_NORMAL,      /*  0 normal         */
	A_DOS_NORMAL,      /*  1 green          */
	A_DOS_NORMAL,      /*  2 cyan           */
	A_DOS_NORMAL,      /*  3 red            */
	A_DOS_NORMAL,      /*  4 magenta        */
	A_DOS_NORMAL,      /*  5 brown          */
	A_DOS_NORMAL,      /*  6 dark grey      */
	A_DOS_NORMAL,      /*  7 light blue     */
	A_DOS_NORMAL,      /*  8 light green    */
	A_DOS_NORMAL,      /*  9 light red      */
	A_DOS_NORMAL,      /* 10 light magenta  */
	A_DOS_NORMAL,      /* 11 yellow         */
	A_DOS_BW_ULINE,    /* 12 uline          */
	A_DOS_NORMAL,      /* 13 blue           */
	A_DOS_BW_STANDOUT, /* 14 reverse        */
	A_DOS_NORMAL,      /* 15 white/hight    */
	A_DOS_BW_STANDOUT, /* 16 bold           */
	0                  /* no more           */
} ;

static byte *active_attributes;

/*@
 * Changes in ASCII chars from Unix Rogue (and roguelike ASCII tradition):
 * AMULET: ',' to '&'. Not meant to be subtle in DOS
 * BMAGIC: '+' to '~'. '+' is ASCII for door. BMAGIC is a DOS-only extension.
 *
 * Note: Swapping '+' with '{' would yield a better visual result, as '{' is
 * great as door and it better matches DOS CP437 char 0xCE ╬. But I will not be
 * the one to break such a well-known convention, and get flamed for heresy.
 * You do it.
 */
static CharacterMapping game_character_mappings[] = {
		/*
		 * Dungeon chars. If a char in this block is not unique, such as
		 * the ASCII for room corners, screen_read_character() reverse search will map
		 * them back to a different DOS char. So choose them carefully.
		 */
		{'@', L"\x263A", PLAYER},     // ☺
		{'^', L"\x2666", TRAP},       // ♦
		{':', L"\x2663", FOOD},       // ♣
		{']', L"\x25D8", ARMOR},      // ◘
		{'=', L"\x25CB", RING},       // ○
		{'&', L"\x2640", AMULET},     // ♀
		{'?', L"\x266A", SCROLL},     // ♪
		{'*', L"\x263C", GOLD},       // ☼
		{')', L"\x2191", WEAPON},     // ↑
		{'!', L"\x00A1", POTION},     // ¡
		{'#', L"\x2592", PASSAGE},    // ▒
		{'+', L"\x256C", DOOR},       // ╬
		{'/', L"\x03C4", STICK},      // τ
		{'.', L"\x00B7", FLOOR},      // ·
		{'%', L"\x2261", STAIRS},     // ≡
//		{'$', L"$",      MAGIC},      // $, maps to itself
//		{'~', L"~",      BMAGIC},     // ~, maps to itself
		{'|', L"\x2551", VWALL},      // ║
		{'-', L"\x2550", HWALL},      // ═
		{'-', L"\x2554", ULWALL},     // ╔
		{'-', L"\x2557", URWALL},     // ╗
		{'-', L"\x255A", LLWALL},     // ╚
		{'-', L"\x255D", LRWALL},     // ╝

		// Title screen
		{'X', L"\x2563", DVLEFT},     // ╣
		{'X', L"\x2560", DVRIGHT},    // ╠

		// F1 help screen and save game message
		{'<', L"\x25C4", 0x11},       // ◄ 'Enter' char 1
		{'/', L"\x2518", 0xD9},       // ┘ 'Enter' char 2

		// F1 help screen
		{'^', L"\x2191", 0x18},       // ↑ up (same as WEAPON)
		{'v', L"\x2193", 0x19},       // ↓ down
		{'>', L"\x2192", 0x1A},       // → right
		{'<', L"\x2190", 0x1B},       // ← left

		// F2 help screen
		{'#', L"\x2593", 0xB2},       // ▓ 'passage', different char

		{'`', L"`", 0}  // if ` appears on screen, something went wrong!
};

static CharacterMapping box_character_mappings[] = {
		// single-width box glyphs
		{'|', L"\x2502", VLINE},      // │
		{'-', L"\x2500", HLINE},      // ─
		{'.', L"\x250C", ULCORNER},   // ┌
		{'.', L"\x2510", URCORNER},   // ┐
		{'`', L"\x2514", LLCORNER},   // └
		{'\'',L"\x2518", LRCORNER},   // ┘

		// same as *WALL set, but used in boxes, with different ASCII
		{'H', L"\x2551", DVLINE},     // ║
		{'=', L"\x2550", DHLINE},     // ═
		{'#', L"\x2554", DULCORNER},  // ╔
		{'#', L"\x2557", DURCORNER},  // ╗
		{'#', L"\x255A", DLLCORNER},  // ╚
		{'#', L"\x255D", DLRCORNER},  // ╝

		// same as PASSAGE, used for curtain
		{'#', L"\x2592", FILLER},     // ▒

		{'\0', L"", 0}
};

/*@
 * Numpad keys missing from the terminfo data of some common terminals
 * See define_keys()
 *
 * CSI: Control Sequence Introducer: ESC [
 * SS3: Single Shift Select of G3 Character Set (SS3  is 0x8f): ESC O
 *      This affects next character only.
 *
 * xterm
 * *	SS3 j	* multiply
 * +	SS3 k	+ add
 * -	SS3 m	- minus
 * .	SS3 n	. period (VT220)
 * /	SS3 o	/ divide   ()
 * ENT	0527	KEY_ENTER
 * 7	0406	KEY_HOME
 * 5	0536	KEY_B2
 * 1	0550	KEY_END
 *
 * gnome-terminal TERM=xterm
 * .	.
 * 7	CSI 1 ~	Home (VT220)
 * 5	CSI E	5 begin (kb2/K2)
 * 1	CSI 4 ~	End (VT220)
 *
 * gnome-terminal TERM=gnome
 * ENT	SS3 M	CR, enter (kent/@8)
 * 7	0552	KEY_FIND
 * 1	0601	KEY_SELECT
 */
static TerminalKeySequence terminal_key_mappings[] = {
		{TTY_SS3 "j", '*'},
		{TTY_SS3 "k", '+'},
		{TTY_SS3 "m", '-'},
		{TTY_SS3 "n", '.'},
		{TTY_SS3 "o", '/'},
		{TTY_SS3 "M",  KEY_ENTER},
		{TTY_CSI "E",  KEY_B2},
		{TTY_CSI "1~", KEY_HOME},
		{TTY_CSI "4~", KEY_END},
};

static byte double_box_characters[BX_SIZE] = {
	DULCORNER, DURCORNER, DLLCORNER, DLRCORNER, DVLINE, DHLINE, DHLINE
};

static byte single_box_characters[BX_SIZE] = {
	ULCORNER, URCORNER, LLCORNER, LRCORNER, VLINE, HLINE, HLINE
};

/*@ unused
static byte fat_box[BX_SIZE] = {
	0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0xdf, 0xdc
};
 */
static byte blank_box_characters[BX_SIZE] = {
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20
};

/*
 * Table for IBM extended key translation
 * moved from march_dep.c
 */
static struct key_translation {
	int keycode;
	byte keyis;
} key_translations[] = {
#ifdef ROGUE_DOS_CURSES
	{C_HOME,	'y'},
	{C_UP,		'k'},
	{C_PGUP,	'u'},
	{C_LEFT,	'h'},
	{C_RIGHT,	'l'},
	{C_END,		'b'},
	{C_DOWN,	'j'},
	{C_PGDN,	'n'},
	{C_INS,		'>'},
	{C_DEL,		's'},
	{C_F1,		'?'},
	{C_F2,		'/'},
	{C_F3,		'a'},
	{C_F4,		CTRL('R')},
	{C_F5,		'c'},
	{C_F6,		'D'},
	{C_F7,		'i'},
	{C_F8,		'^'},
	{C_F9,		CTRL('F')},
	{C_F10,		'!'},
	{ALT_F9,	'F'}
#else
	{KEY_ENTER,	'\n'}, //@ Keypad Enter
	{KEY_HOME,	'y'},
	{KEY_FIND,	'y'},  //@ Keypad Home (7) in some terminals
	{KEY_A1,	'y'},  //@ Keypad upper left (7)
	{KEY_UP,	'k'},
	{KEY_PPAGE,	'u'},  //@ Page Up
	{KEY_A3,	'u'},  //@ Keypad upper right (9)
	{KEY_BACKSPACE, 'h'},
	{KEY_LEFT,	'h'},
	{KEY_RIGHT,	'l'},
	{KEY_END,	'b'},
	{KEY_SELECT,	'b'},  //@ Keypad End (1) in some terminals
	{KEY_C1,	'b'},  //@ Keypad lower left (1)
	{KEY_DOWN,	'j'},
	{KEY_NPAGE,	'n'},  //@ Page Down
	{KEY_C3,	'n'},  //@ Keypad lower right (3)
	{KEY_IC,	'>'},  //@ Insert
	{KEY_DC,	's'},  //@ Delete
	{KEY_F(1),	'?'},
	{KEY_F(2),	'/'},
	{KEY_F(3),	'a'},
	{KEY_F(4),	CTRL('R')},
	{KEY_F(5),	'c'},
	{KEY_F(6),	'D'},
	{KEY_F(7),	'i'},
	{KEY_F(8),	'^'},
	{KEY_F(9),	CTRL('F')},
	{KEY_F(10),	'!'},
	{KEY_F(57),	'F'}  //@ ALT+F9
#endif
};


/*@
 * Beep a an audible beep, if possible
 *
 * Originally in dos.asm
 *
 * Used hardware port 0x61 (Keyboard Controller) for direct PC Speaker access.
 * This behavior is reproduced here, to the best of my knowledge. Comments
 * were copied from original.
 *
 * The debug code prints a BEL (0x07) character in standard output, which is
 * supposed to make terminals play a beep.
 */
void
screen_beep(void)
{
#ifdef ROGUE_DOS_CURSES
	byte speaker = 0x61;       //@ speaker port
	byte saved = dos_read_port(speaker);  // input control info from keyboard/speaker port
	byte cmd = saved;
	int cycles = 300;          // count of speaker cycles
	int c;

	while (--cycles)
	{
		cmd &= 0x0fc;          // speaker off pulse (mask out bit 0 and 1)
		dos_write_port(speaker, cmd);     // send command to speaker port
		for(c=50; c; c--) {;}  // kill time for tone half-cycle
		cmd |= 0x10;           // speaker on pulse (bit 1 on)
		dos_write_port(speaker, cmd);     // send command to speaker port
		for(c=50; c; c--) {;}  // kill time for tone half-cycle
	}
	dos_write_port(speaker, saved);       // restore speaker/keyboard port value
#ifdef ROGUE_DEBUG
	printf("\a");  //@ lame, I know... but it works
#endif
#else
	beep();
#endif
}


/*@
 * Read a character from user input. getch() with non-blocking capability
 *
 * msdelay has same meaning as delay in timeout(): If no key was pressed after
 * msdelay milliseconds, return ERR. Negative values will block until a key
 * is pressed. Both blocking and timeout mode require a properly initialized
 * curses with initscr() (in initialize_screen()), otherwise it will be non-blocking.
 * This requirement did not exist in original
 *
 * After the wgetch() call, input will always restore to blocking mode using
 * nodelay(FALSE);
 *
 * Return ERR on non-ASCII chars and on window resize.
 */
int
screen_read_key(int timeout_ms)
{
#ifdef ROGUE_DOS_CURSES
	/*@
	 * getchar() is not a true replacement, as asm version has no echo and no
	 * buffering. Asm was also non-blocking, but only used after no_char(),
	 * a combination that effectively blocks until input.
	 */
	return getchar();
#else
	int character = 0;

	wtimeout(stdscr, timeout_ms);


#ifdef ROGUE_WIDECHAR
	wint_t wide_key;
	int input_status;
	if ((input_status = wget_wch(stdscr, &wide_key)) == ERR || (input_status == OK && !isascii(wide_key)))
	{
		// we're only interested in ASCII input
		character = ERR;
	}
	else
	{
		// KEY_* codes always fit int, so no need for any special test
		character = (int)wide_key;
	}
#else
	character = wgetch(stdscr);
#endif  // ROGUE_WIDECHAR
	// mask-map custom keys
	if (character != ERR)
	{
		character = TERMINAL_KEY_MASK & character;
	}

	// window resize needs special handling. all others go through xlate/xtab
	if (character == KEY_RESIZE)
	{
		resize_screen();
		character = ERR;
	}
	nodelay(stdscr, FALSE);
	return character;
#endif  // ROGUE_DOS_CURSES
}


/*@
 * Map a (possibly multi-byte or control) character to an 8-bit character using
 * the game translation table.
 *
 * Moved from mach_dep.c as part of read_game_key()
 */
byte
translate_key(int character)
{
	struct key_translation *translation;
	/*
	 * Now read a character and translate it if it appears in the
	 * translation table
	 */
	for (translation = key_translations; translation < key_translations + (sizeof key_translations) / sizeof *key_translations; translation++)
	{
		if (character == translation->keycode) {
			character = translation->keyis;
			break;
		}
	}
	return (byte)character;
}


/*@
 * Move the cursor to the given row and column
 *
 * Originally in zoom.asm
 *
 * As per original code, also updates C global variables c_col_ and c_row,
 * used as "cache" by curses. See get_cursor_position(). Actual cursor movement is only
 * performed if iscuron is set. See set_cursor_visible().
 *
 * Used BIOS INT 10h/AH=02h to set the cursor on C variable page_no.
 * The BIOS call is now performed via call_dos_interrupt()
 *
 * INT 10h/AH=02h - Set Cursor Position
 * BH = page number
 * DH = row
 * DL = col
 */
int
screen_move(row, col)
	int row;
	int col;
{
#ifdef ROGUE_DOS_CURSES
	c_row = row;
	c_col = col;

	if (iscuron)
	{
		dos_regs->ax = HIGH(2);
		dos_regs->bx = HIGH(page_no);
		dos_regs->dx = HILO(row, col);
		call_dos_interrupt(SW_SCR, dos_regs);
	}
#else
	return wmove(stdscr, row, col);
#endif
}


#ifdef ROGUE_DOS_CURSES
/*@
 * Put the given character on the screen
 *
 * Character is put at current (c_row, c_col) cursor position, and set with
 * current current_dos_attribute attributes.
 *
 * Works as a stripped-down <curses.h> addch(), or as an improved <stdio.h>
 * putchar(): it uses attributes but always operate on current current_dos_attribute instead
 * of extracting attributes from ch, and put at cursor position but does not
 * update its location, nor has any special CR/LF/scroll up handling for '\n'.
 *
 * A curses replacement could be:
 *    delch();
 *    insch(ch);
 *
 * Originally in zoom.asm
 *
 * By my understanding, asm function works as follows: if cursor is has_actor_flag (via
 * iscuron C var), it invokes BIOS INT 10h/AH=09h to put char with attributes.
 * If not, it waits for video retrace (unless no_check C var was TRUE) and
 * then write directly in Video Memory, using C vars scr_row and scr_ds to
 * calculate position address.
 *
 * This function replicates this behavior using dos_write_memory() and call_dos_interrupt().
 *
 * BIOS INT 10h/AH=09h - Write character with attribute at cursor position
 * AL = character
 * BH = page number
 * BL = character attribute
 * CX = number of times to write character
 *
 */
void
putchr(byte ch)
{
	if (iscuron)
	{
		// Use BIOS call
		dos_regs->ax = HILO(9, ch);
		dos_regs->bx = HILO(page_no, current_dos_attribute);
		dos_regs->cx = 1;
		call_dos_interrupt(SW_SCR, dos_regs);
	}
	else
	{
		if (!no_check){;}  // "wait" for video retrace
		/*
		 * Write to video memory
		 * Each char uses 2 bytes in video memory, hence doubling c_col.
		 * scr_row[] array takes that into account, so we can use c_row
		 * directly. See initialize_screen().
		 */
		dos_write_memory(HILO(current_dos_attribute, ch), 1,
			scr_ds, scr_row[c_row] + 2 * c_col);
	}
#ifdef ROGUE_DEBUG
	putchar(ch);
#endif
}
#endif


/*
 * Read the character at the current cursor position, without display attributes.
 * The archived zoom.asm curch_ returns a full character/attribute word. Its BIOS
 * branch first positions the cursor from cached coordinates, then reads it; its
 * other branch reads video memory, optionally waiting for retrace. The native wrapper
 * uses the curses cursor and extracts only the character.
 */
byte
screen_read_character(void)
{
#ifdef ROGUE_DOS_CURSES
	chtype chrattr = 0;

	if (iscuron)
	{
		dos_regs->ax = HIGH(8);
		dos_regs->bx = HIGH(page_no);
		chrattr = call_dos_interrupt(SW_SCR, dos_regs);
	}
	else
	{
		if (!no_check){;}
		dos_read_memory(&chrattr, 1, scr_ds, scr_row[c_row] + 2 * c_col);
	}
	return (byte)LOW(chrattr);
#else
	byte dos_character = 0;
	CharacterMapping *mapping;
	wchar_t screen_character;  // character on screen
	wchar_t candidate_character;  // reference character on ctab mapping
#ifdef ROGUE_WIDECHAR
	cchar_t screen_cell;
	wchar_t wide_characters[CCHARW_MAX + 1];
	attr_t ignored_attributes;
	short ignored_color_pair;

	win_wch(stdscr, &screen_cell);
	getcchar(&screen_cell, wide_characters, &ignored_attributes, &ignored_color_pair, NULL);
	screen_character = wide_characters[0];
#else

	screen_character = (wchar_t)(A_CHARTEXT & winch(stdscr));
#endif  // ROGUE_WIDECHAR
	// if not on mapping list, will report as itself
	dos_character = (byte)screen_character;

	if (character_set == CP437)
	{
		return dos_character;
	}
	for(mapping = game_character_mappings; mapping->dos; mapping++)
	{
		switch (character_set)
		{
		default:      candidate_character = L'\0';
		break;
		case ASCII:   candidate_character = (wchar_t)mapping->ascii;
		break;
		case UNICODE: candidate_character = *mapping->unicode;
		break;
		}
		if (screen_character == candidate_character)
		{
			dos_character = mapping->dos;
			break;
		}
	}
	return dos_character;
#endif  // ROGUE_DOS_CURSES
}


#ifdef ROGUE_DOS_CURSES
/*
 * Fill a buffer with count character/attribute cells. The count is in cells, not
 * bytes. The retained DOS backend defines a cell as uint16_t; native curses storage
 * must be sized using sizeof(chtype) or sizeof(cchar_t). Original routine: dos.asm.
 */
void
wsetmem(buffer, command_repeat_count, attrchar)
	void *buffer;
	int command_repeat_count;
	chtype attrchar;  // enforced to prevent misuse
{
	while (command_repeat_count--)
		((chtype *)buffer)[command_repeat_count] = (chtype)attrchar;
}
#endif

/*
 * clear screen
 */
void
screen_clear(void)
{
#ifdef ROGUE_DOS_CURSES
	if (scr_ds == svwin_ds)
		wsetmem(saved_screen, LINES*COLS, 0x0720);
	else
		blot_out(0,0,LINES-1,COLS-1);
#else
	wclear(stdscr);
#endif
}


/*
 *  Turn cursor on and off
 */
bool
set_cursor_visible(bool visible)
{
#ifdef ROGUE_DOS_CURSES
	register bool previous_visibility;

	if (iscuron == visible)
		return visible;
	previous_visibility = iscuron;
	iscuron = visible;

	dos_regs->ax = 0x100;
	if (visible)
	{
		dos_regs->cx = (is_color ? 0x607 : 0xb0c);
		call_dos_interrupt(SW_SCR, dos_regs);
		screen_move(c_row, c_col);
	}
	else
	{
		dos_regs->cx = 0xf00;
		call_dos_interrupt(SW_SCR, dos_regs);
	}
	return(previous_visibility);
#else
	/*@
	 * curs_set return values:
	 * ERR = terminal does not support the visibility requested
	 *   0 = invisible
	 *   1 = normal
	 *   2 = very visible
	 */
	int previous_visibility = curs_set(visible);

	if (previous_visibility == 0 || previous_visibility == ERR)
		return FALSE;
	else
		return TRUE;
#endif
}


/*
 * get curent cursor position
 */
void
get_cursor_position(row,column)
	int *row, *column;
{
#ifdef ROGUE_DOS_CURSES
	*row = c_row;
	*column = c_col;
#else
	getyx(stdscr, *row, *column);
#endif
}

#ifdef ROGUE_DOS_CURSES
void
real_rc(pn, rp,cp)
	int pn, *rp, *cp;
{
	/*
	 * pc bios: read current cursor position
	 */
	dos_regs->ax = 0x300;
	dos_regs->bx = pn << 8;

	call_dos_interrupt(SW_SCR, dos_regs);

	*rp = dos_regs->dx >> 8;
	*cp = dos_regs->dx & 0xff;
}
#endif

//@ Not in original
void
screen_refresh(void)
{
#ifndef ROGUE_DOS_CURSES
	wrefresh(stdscr);
#endif
}

/*
 *	clrtoeol
 */
void
screen_clear_to_eol(void)
{
#ifdef ROGUE_DOS_CURSES
	int r,c;

	if (scr_ds == svwin_ds)
		return;
	get_cursor_position(&r,&c);
	blot_out(r,c,r,COLS-1);
#else
	wclrtoeol(stdscr);
#endif
}

void
screen_write_text_at(row,column,s)
	int row,column;
	char *s;
{
	screen_move(row, column);
	screen_write_text(s);
}

void
screen_write_character_at(int row, int column, byte character)
{
	screen_move(row, column);
	screen_write_character(character);
}

byte
screen_read_character_at(row, column)
	int row, column;
{
	screen_move(row, column);
	return screen_read_character();
}


/*
 * put the character on the screen and update the
 * character position
 */
void
screen_write_character(byte character)
{
#ifdef ROGUE_DOS_CURSES
	int r, c;
#endif
	byte old_attr;

	old_attr = current_dos_attribute;

	if (active_attributes == color_attributes)
	{
		/* if it is inside a room */
		if (current_dos_attribute == A_DOS_NORMAL)
		{
			switch(character)
			{
			case DOOR:
			case VWALL:
			case HWALL:
			case ULWALL:
			case URWALL:
			case LLWALL:
			case LRWALL:
				current_dos_attribute = A_DOS_BROWN;  /* brown */
				break;
			case FLOOR:
				current_dos_attribute = A_DOS_GREEN | A_DOS_BRIGHT;  /* light green */
				break;
			case STAIRS:
				current_dos_attribute = A_DOS_BLACK | A_DOS_BG(A_DOS_GREEN) | A_DOS_BLINK; /* black on green */
				break;
			case TRAP:
				current_dos_attribute = A_DOS_MAGENTA;  /* magenta */
				break;
			case GOLD:
			case PLAYER:
				current_dos_attribute = A_DOS_YELLOW;  /* yellow */
				break;
			case POTION:
			case SCROLL:
			case STICK:
			case ARMOR:
			case AMULET:
			case RING:
			case WEAPON:
				current_dos_attribute = A_DOS_BLUE | A_DOS_BRIGHT;
				break;
			case FOOD:
				current_dos_attribute = A_DOS_RED;
				break;
			}
		}
		/* if inside a passage or a maze */
		else if (current_dos_attribute == A_DOS_STANDOUT)
		{
			switch(character)
			{
			case FOOD:
				current_dos_attribute = A_DOS_RED | A_DOS_STANDOUT ;  /* red @ on white */
				break;
			case GOLD:
			case PLAYER:
				current_dos_attribute = A_DOS_YELLOW | A_DOS_STANDOUT;  /* yellow on white */
				break;
			case POTION:
			case SCROLL:
			case STICK:
			case ARMOR:
			case AMULET:
			case RING:
			case WEAPON:
				current_dos_attribute = A_DOS_BLUE | A_DOS_STANDOUT;  /* blue on white */
				break;
			}
		}
		// Preserve stair colors if the caller selected the bright-normal attribute.
		else if (current_dos_attribute == (A_DOS_BRIGHT | A_DOS_NORMAL) && character == STAIRS)
			current_dos_attribute = A_DOS_BLACK | A_DOS_BG(A_DOS_GREEN) | A_DOS_BLINK;
	}

#ifdef ROGUE_DOS_CURSES
	/*@
	 * For '\n', perform a cursor CR+LF if not on last line (off-screen),
	 * or just CR and scroll the whole window content up.
	 * Otherwise just put the char and advance the cursor.
	 */
	get_cursor_position(&r,&c);
	if (character == '\n') {
		if (r == LINES-1)
		{
			scroll_up(0, LINES-1, 1);
			screen_move(LINES-1, 0);
		}
		else
		{
			screen_move(r+1, 0);
		}
	}
	else
	{
		putchr(character);
		screen_move(r,c+1);
	}
#else
	switch (character_set)
	{
	default:
	case ASCII:
		character = ascii_from_dos(character, game_character_mappings);
		/* fallthrough */
	case CP437:
		waddch(stdscr, character | curses_attributes_from_dos(current_dos_attribute));
		break;
#ifdef ROGUE_WIDECHAR
	case UNICODE:
		wadd_wch(stdscr, unicode_from_dos(character, current_dos_attribute, game_character_mappings));
		break;
#endif  // ROGUE_WIDECHAR
	}
#endif  // ROGUE_DOS_CURSES
	current_dos_attribute = old_attr;
	return;
}


void
screen_write_text(s)
	char *s;
{
#ifdef ROGUE_DEBUG
	print_int_calls = FALSE;
#endif
	while(*s)
		screen_write_character(*s++);
#ifdef ROGUE_DEBUG
#ifdef ROGUE_DOS_CURSES
	printf("\n");
#endif
	print_int_calls = TRUE;
#endif
}

#ifndef ROGUE_DOS_CURSES
#ifdef ROGUE_WIDECHAR
cchar_t *
unicode_from_dos(byte dos_character, byte dos_attr, CharacterMapping *mapping)
{
	short color;
	attr_t attrs;

	CharacterMapping *matched_mapping = charcode_from_dos(dos_character, mapping);
	wide_attributes_from_dos(dos_attr, &attrs, &color);

	setcchar(&temporary_screen_cell,
			matched_mapping->unicode,
			attrs,
			color,
			NULL);
	return &temporary_screen_cell;
}
#endif  // ROGUE_WIDECHAR


void
define_keys(void)
{
#ifdef NCURSES_VERSION
	int i;
	int shift;
	TerminalKeySequence *sequence, *sequence_end;

	// get the shift offset of the bit past KEY_MAX
	for (i=KEY_MAX, shift=1; i>>=1; shift++);

	// define the mask that will be used by cur_getch() and friends
	TERMINAL_KEY_MASK = (1 << shift) - 1;

	// define keys. first key gets i>0 to leave room for user terminfo keys
	for (i=8, sequence=terminal_key_mappings, sequence_end=ARRAY_END(terminal_key_mappings); sequence < sequence_end; sequence++)
	{
		if(!key_defined(sequence->def))
		{
			define_key(sequence->def, ((i++) << shift) | sequence->dest);
		}
	}
#endif  // NCURSES_VERSION
}


byte
ascii_from_dos(byte dos_character, CharacterMapping *mapping)
{
	return charcode_from_dos(dos_character, mapping)->ascii;
}


CharacterMapping *
charcode_from_dos(byte dos_character, CharacterMapping *mapping)
{
	CharacterMapping *candidate_mapping;

	// Shortcut for "ordinary" chars that map to themselves
	if (dos_character == '\0' || dos_character == '\n' || (isascii(dos_character) && isprint(dos_character)))
	{
		fallback_character_mapping.ascii = fallback_character_mapping.dos = *fallback_character_mapping.unicode = dos_character;
		return &fallback_character_mapping;
	}

	for(candidate_mapping = mapping; candidate_mapping->dos; candidate_mapping++)
	{
		if (dos_character == candidate_mapping->dos)
		{
			return candidate_mapping;
		}
	}
	// if not found, will return the sentinel
	return candidate_mapping;
}

/*
		 * Return a color index, based on DOS char attribute.
		 * For now index is in range 0-7, and the interpretation of A_DOS_BRIGHT bit to
		 * either `fg index += 8` or `color | [W]A_BOLD` is the caller's responsibility.
		 * This could be changed in the future.
		 */
short
color_from_dos(byte dos_attr, bool foreground)
{
	byte color = (dos_attr >> (foreground ? A_DOS_FG_COLOR : A_DOS_BG_COLOR)) & \
			A_DOS_COLOR_MASK;

	// swap red and blue components so DOS index match ANSI's
	return swap_bits(color, 0, 2, 1);
}


/*
 * Convert a DOS/CGA character attribute to its curses equivalent
 *
 * The attribute model used by addch() and attrset() has no distinct type for
 * attributes: addch() expects a chtype OR'ed with A_* constants, and attrset()
 * expects an int, hinting that all A_* constants, as well as any chtype AND'ed
 * with A_ATTRIBUTES, fit int range.
 *
 * Return type chtype was chosen for consistency with termattrs() and vidattr(),
 * which are the only known functions dealing with an isolated set of OR'ed A_*
 * attributes. int could also have been chosen, as per attrset() usage.
 *
 * Return type could be attr_t, a dedicated type for attributes. In ncurses it
 * is typedef'd to chtype, but they are semantically different: attr_t expects
 * to be manipulated using the WA_* constants, and it's not expected to contain
 * color pair information: functions with a attr_t argument also have another
 * argument for color pair of type short.
 *
 * The only functions that deal exclusively with the attr_t model and have no
 * counterpart using the old model are the ones working with cchar_t wide chars
 * ("complex renditions" in ncurses docs). For those there is wide_attributes_from_dos()
 */
chtype
curses_attributes_from_dos(byte dos_attr)
{
	chtype attr = A_NORMAL | COLOR_PAIR(0);
	short foreground, background;

	// shortcut to avoid setting (and calculating) a spurious color pair
	if (dos_attr == A_DOS_NORMAL)
		return attr;

	if (dos_attr & A_DOS_BLINK)
		attr |= A_BLINK;

	foreground = color_from_dos(dos_attr, TRUE);
	background = color_from_dos(dos_attr, FALSE);

	if (dos_attr & A_DOS_BRIGHT)
	{
		if (screen_color_count < 16)
			attr |= A_BOLD;
		else
			foreground += 8;
	}

#ifdef NCURSES_VERSION
	/*
	 * Set terminal reverse attribute when Rogue implies it
	 *
	 * CGA does not have a "reverse" mode, Rogue achieves it by manually
	 * setting a white background with black foreground (black by default,
	 * but foreground could also be set to other colors, see screen_write_character()).
	 * Thus, A_DOS_STANDOUT require no special handling and could be treated
	 * like any other color pair, and this block is entirely optional.
	 *
	 * By activating the terminal reverse mode and swapping fg with bg to
	 * revert Rogue's reversal, we allow the default fg/bg terminal colors
	 * to be used instead of hard-coded black on white, making reversed
	 * A_DOS_STANDOUT text consistent with A_DOS_NORMAL text even if user's
	 * terminal color theme is different from Rogue's default.
	 *
	 * This does not work with 8-color terminals: on reversed mode, A_BOLD
	 * operates on the background color, making it impossible to get yellow
	 * as foreground.
	 */
	if (((dos_attr & A_DOS_STANDOUT) == A_DOS_STANDOUT)
			&& screen_color_count != 8
			&& use_terminal_fgbg)
	{
		attr |= A_REVERSE;

		short tmp = background;
		background = foreground;
		foreground = tmp;
	}
#endif  // NCURSES_VERSION

	/*
	 * BIG problem here: if colors == 0, we should not use color pairs at
	 * all, but map the entries in monochrome_attributes from original intentions to
	 * current curses A_* attributes like underline, bold, standout, etc.
	 */
	if (screen_color_count > 0)
		attr |= COLOR_PAIR_N(foreground, background);

	return attr;
}


#ifdef ROGUE_WIDECHAR
void
wide_attributes_from_dos(byte dos_attr, attr_t *attrs, short *color_pair)
{
	/*
	 * A sloppy version could simply assume that attr_t is typedef'd to
	 * chtype and all WA_* == A_*, which is true for current ncurses,
	 * and this function would be simplified to:
	 *
	 * attr_t bute = curses_attributes_from_dos(dos_attr);
	 * *attrs = bute & A_ATTRIBUTES & ~A_COLOR,
	 * *color_pair = PAIR_NUMBER(bute);
	 *
	 * Tempting, but we shall not make such assumptions. By the book, boys!
	 */

	short foreground, background;

	*attrs = WA_NORMAL;
	*color_pair = 0;

	if (dos_attr == A_DOS_NORMAL)
		return;

	if (dos_attr & A_DOS_BLINK)
		*attrs |= WA_BLINK;

	foreground = color_from_dos(dos_attr, TRUE);
	background = color_from_dos(dos_attr, FALSE);

	if (dos_attr & A_DOS_BRIGHT)
	{
		if (screen_color_count < 16)
			*attrs |= WA_BOLD;
		else
			foreground += 8;
	}

#ifdef NCURSES_VERSION
	if (((dos_attr & A_DOS_STANDOUT) == A_DOS_STANDOUT)
			&& screen_color_count != 8
			&& use_terminal_fgbg)
	{
		*attrs |= WA_REVERSE;

		short tmp = background;
		background = foreground;
		foreground = tmp;
	}
#endif  // NCURSES_VERSION

	if (screen_color_count > 0)
		*color_pair = PAIR_INDEX(foreground, background);
}
#endif  // ROGUE_WIDECHAR


void
init_curses_colors(void)
{
	int foreground, dos_fg, dfg;
	int background, dos_bg, dbg;
	int i, row, g, b, cube;
	int colormode;
	int cmap[16];

	/*
	 * Terminal capabilities determine the available color pairs. monochrome_requested
	 * selects monochrome_attributes later in initialize_screen; it does not change the
	 * reported terminal capabilities or emulate a different DOS video adapter.
	 */
	if (!has_colors() || COLORS < 8)
	{
		screen_color_count = 0;
		return;
	}

	/*
	 * Some notes on colors and mappings:
	 *
	 * DOS only uses 8 basic colors, and the foreground could be bumped to
	 * 16 via bright attribute. So for all code outside this function,
	 * background and foreground color indexes range from 0-15, so at most
	 * 16 * 16 = 256 color pairs are needed, and should be always accessed
	 * via PAIR_INDEX(fg, bg) or COLOR_PAIR_N(fg, bg) macros.
	 *
	 * Color indexes in DOS (actually, in CGA/VGA) are an RGB bitmap, blue
	 * being the least significant bit, so DOS colors have the Red and Blue
	 * components swapped compared to ANSI colors, which the curses named
	 * constants derive from. color_from_dos() takes care of this, so fg/bg
	 * indexes should be interpreted by ANSI table, ie, 1=Red, 4=Blue, etc
	 *
	 * The actual colors mapped to each of this 16 color indexes depends on
	 * terminal color capabilities:
	 * - If only 8, achieve the 16 via curses [W]A_BOLD attribute.
	 * - For 16 color terminals there's a 1:1 mapping
	 * - For 8 and 16, try to redefine terminal RGB values to match CGA
	 * - 88 and 256, remap the 16 color indexes to the 4x4x4 or 6x6x6 color
	 *   cube to get an exact CGA color match.
	 */

	// colormode is only used here, colors is global
	screen_color_count = 16;
	if      (COLORS >= 256) colormode = 256;
	else if (COLORS >=  88) colormode =  88;
	else if (COLORS >=  16) colormode =  16;
	else                    colormode = screen_color_count = 8;

	switch(colormode)
	{
	case 8:
	case 16:
		cube = 0;
		if (can_change_color() && change_colors)
		{
			colors_changed = TRUE;
		}
		break;
	case  88:
		cube = 4;
		break;
	case 256:
		cube = 6;
		break;
	}

	if (cube)
	{
		for (i = 0; i < screen_color_count; i++)
		{
			row = (cube - 1) * CGA_RED(i);
			g = (cube - 1) * CGA_GREEN(i);
			b = (cube - 1) * CGA_BLUE(i);
			cmap[i] = 16 + cube * cube * row + cube * g + b;
		}
	}
	else if (colors_changed)
	{
		for (i = 0; i < screen_color_count; i++)
		{
			init_color(i,
					1000 * CGA_RED(i),
					1000 * CGA_GREEN(i),
					1000 * CGA_BLUE(i));
			cmap[i] = i;  // 1:1 mapping
		}
	}
	else
	{
		for (i = 0; i < screen_color_count; i++)
		{
			cmap[i] = i;  // 1:1 mapping
		}
	}

	/*@
	 * More notes on color mappings and pairs:
	 *
	 * - Color pair 0 is not initialized, as per recommendation in curses
	 *   documentation, and it is only used when DOS attributes are set to
	 *   A_DOS_NORMAL, for example by cur_standend().
	 *
	 * - The default foreground and background colors used by DOS, as
	 *   defined by A_DOS_NORMAL, are mapped to (COLOR_WHITE, COLOR_BLACK).
	 *   If the curses implementation is ncurses and the user allows it,
	 *   they are mapped to (-1,-1), the default foreground and background
	 *   terminal colors.
	 */

	dos_fg = color_from_dos(A_DOS_NORMAL, TRUE);
	dos_bg = color_from_dos(A_DOS_NORMAL, FALSE);
	if ((A_DOS_NORMAL & A_DOS_BRIGHT) && screen_color_count > 8)
		dos_fg += 8;

	dfg = cmap[COLOR_WHITE];
	dbg = cmap[COLOR_BLACK];

#ifdef NCURSES_VERSION
	/*
	 * One could argue that change_colors == TRUE should imply
	 * use_terminal_fgbg == TRUE, but currently they are independent.
	 */
	if (use_terminal_fgbg)
	{
		use_default_colors();
		dfg = dbg = -1;
	}
#else
	use_terminal_fgbg = FALSE;
#endif  // NCURSES_VERSION

	for (background = 0; background < screen_color_count; background++)
	{
		for (foreground = screen_color_count - (background ? 2 : 1); foreground >= 0; foreground--)
		{
			init_pair(PAIR_INDEX(foreground, background),
					(foreground == dos_fg) ? dfg : cmap[foreground],
					(background == dos_bg) ? dbg : cmap[background]);
		}
	}

#ifdef ROGUE_DEBUG
	int j;
	printw("COLOR TEST - Displayed colors should match [R,G,B] values\n");
	printw("Color mode: %d colors, using %s\n", colormode,
			cube ? "color cube" :
			colors_changed ? "RGB" :
			screen_color_count > 8 ? "ANSI 16" :
			"ANSI 8 + Bold");
	for (i = 0; i < 8; i++)
	{
		printw(" %d [%3d,%3d,%3d] #%02X%02X%02X ",
				i,
				(int)(255 * CGA_RED(i)),
				(int)(255 * CGA_GREEN(i)),
				(int)(255 * CGA_BLUE(i)),
				(int)(255 * CGA_RED(i)),
				(int)(255 * CGA_GREEN(i)),
				(int)(255 * CGA_BLUE(i))
		);
		for(j=15;j;j--) waddch(stdscr, '#' | COLOR_PAIR_N(i, 0));
		if (screen_color_count > 8)
			for(j=15;j;j--) waddch(stdscr, '#' | COLOR_PAIR_N(i + 8, 0));
		else
			for(j=15;j;j--) waddch(stdscr, '#' | COLOR_PAIR_N(i, 0) | A_BOLD);
		printw(" #%02X%02X%02X [%3d,%3d,%3d]\n",
				(int)(255 * CGA_RED(i+8)),
				(int)(255 * CGA_GREEN(i+8)),
				(int)(255 * CGA_BLUE(i+8)),
				(int)(255 * CGA_RED(i+8)),
				(int)(255 * CGA_GREEN(i+8)),
				(int)(255 * CGA_BLUE(i+8))
		);
	}
#endif  // ROGUE_DEBUG
}


void
resize_screen()
{
	if ((LINES != game_screen_rows) || (COLS != game_screen_columns))
	{
		if (resizeterm(game_screen_rows, game_screen_columns) == OK)
		{
			flushinp();  //@ eat up the generated KEY_RESIZE
		}
		else
		{
			fatal("Could not resize resize terminal to %u x %u\n",
					game_screen_columns, game_screen_rows);
		}
	}
}
#endif  // not ROGUE_DOS_CURSES


void
set_display_attribute(attribute_index)
	int attribute_index;
{
	if (attribute_index < MAXATTR)
		current_dos_attribute = active_attributes[attribute_index];
	else
		current_dos_attribute = attribute_index;
#ifndef ROGUE_DOS_CURSES
	/*@
	 * XOpen Curses standard and ncurses docs clearly says that both
	 * attrset() and attr_set() should operate on the same video attributes,
	 * regardless if current screen cell is chtype or cchar_t. So we still
	 * use attrset() for ncursesw
	 */
	attrset(curses_attributes_from_dos(current_dos_attribute));
#endif
}

#ifdef ROGUE_DOS_CURSES
//@ unused, and proper varargs implementation would require cur_vprintw()
void
error(mline,show_message,a1,a2,a3,a4,a5)
	int mline;
	char *show_message;
	int a1,a2,a3,a4,a5;
{
	int row, col;

	get_cursor_position(&row,&col);
	screen_move(mline,0);
	screen_clear_to_eol();
	screen_printf(show_message,a1,a2,a3,a4,a5);
	screen_move(row,col);
}

//@ unused, and already stubbed in original
/*
 * Called when rogue runs to move our cursor to be where DOS thinks
 * the cursor is
 */
void
set_cursor(void)
{
/*
	regs->ax = 15 << 8;
	call_dos_interrupt(SW_SCR, regs);
	real_rc(regs->bx >> 8, &c_row, &c_col);
 */
}

/*@
 * Return TRUE if the system is identified as an IBM PCJr ("PC Junior")
 *
 * Moved from mach_dep.c, only used for setting no_check in initialize_screen().
 *
 * 0xF000:0xFFFE 1  IBM computer-type code; see also BIOS INT 15h/C0h
 *  0xFF = Original PC
 *  0xFE = XT or Portable PC
 *  0xFD = PCjr
 *  0xFC = AT (or XT model 286) (or PS/2 Model 50/60)
 *  0xFB = XT with 640K motherboard
 *  0xFA = PS/2 Model 30
 *  0xF9 = Convertible PC
 *  0xF8 = PS/2 Model 80
 */
#define PC  0xff
#define XT  0xfe
#define JR  0xfd
#define AT  0xfc
bool
isjr()
{
	static int machine = 0;

	if (machine == 0) {
		dos_read_memory(&machine,1,0xf000,0xfffe);
		machine &= 0xff;
	}
	return machine == JR;
}
#endif

/*
 *  initialize_screen(win_name):
 *		initialize window -- open disk window
 *						  -- determine type of moniter
 *						  -- determine screen memory location for dma
 */
void
initialize_screen(void)
{
#ifdef ROGUE_DOS_CURSES
	register int i, cnt;

	/*
	 * Get monitor type
	 */
#ifdef ROGUE_DOS_SCREEN
	//@ if get_dos_video_mode() also returned BH, it could be used here
	dos_regs->ax = 15 << 8;
	call_dos_interrupt(SW_SCR, dos_regs);
	old_page_no = dos_regs->bx >> 8;
	dos_screen_mode = dos_regs->ax = 0xff & dos_regs->ax;
#else
	old_page_no = 0;
	dos_screen_mode = ROGUE_SCR_TYPE;
#endif
	/*
	 * initialization is any good because restarting game
	 * has old values!!!
	 * So reassign defaults
	 * @ by "restarting" he means "restoring a saved game file"
	 */
	LINES   =  25;
	COLS    =  80;
	scr_ds  =  0xB800;
	active_attributes = monochrome_attributes;

	/*@
	 * BIOS INT 10h/AX=0Fh table for AL values used in the switch:
	 * 00h - Text 40x25 mono  - Only CGA, PCjr, Tandy
	 * 01h - Text 40x25 color - Only CGA, PCjr, Tandy
	 * 02h - Text 80x25 mono  - Only CGA, PCjr, Tandy
	 * 03h - Text 80x25 color - All adapters
	 * 07h - Text 80x25 mono  - MDA, Hercules, EGA, VGA
	 * Notes:
	 * - mono is actually 16 shades of gray
	 * - colors are always 16 except for EGA in mode 3, which could be 64
	 * - EGA in mode 3 could also be 80x43
	 * - VGA in mode 3 could also be 80x43 or 80x50
	 */
	switch (dos_screen_mode) {
		/*
		 *  It is a TV
		 */
		case 1:
			active_attributes = color_attributes;
			/* fallthrough */
		case 0:
			COLS = 40;
			break;

		/*
			 * Its a high resolution monitor
			 */
		case 3:
			active_attributes = color_attributes;
			/* fallthrough */
		case 2:
			break;
		case 7:
			scr_ds = 0xB000;
			no_check = TRUE;
			break;
		/*
			 * Just to save text space lets eliminate these
			 *
		case 4:
		case 5:
		case 6:
			move(24,0);
			fatal("Program can't be run in graphics mode");
			 */
		default:
			screen_move(24,0);
			fatal("Unknown screen type (%d)",dos_regs->ax);
			break;
	}

	/*
	 * Read current cursor position
	 */
	real_rc(old_page_no, &c_row, &c_col);
	/*@ savewin is now a fixed size array.
	 * _dsval is an extern set by begin.asm with its DS register value
	 *
	if ((savewin = sbrk(4096)) == (void *)-1) {
		svwin_ds = -1;
		savewin = (char *) cell_flags;
		if (dos_screen_mode == 7)
			fatal(out_of_memory_message);
	} else {
		savewin = (char *) (((intptr) savewin + 0xf) & 0xfff0);
		svwin_ds = (((intptr) savewin >> 4) & 0xfff) + _dsval;
	}
	 */

	for (i = 0, cnt = 0; i < 25; cnt += 2*COLS, i++)
		scr_row[i] = cnt;
	//@ allocate_memory(2);  // no longer need memory alignment
	switch_page(3);
	if (old_page_no != page_no)
		screen_clear();
	screen_move(c_row, c_col);
	if (isjr())
		no_check = TRUE;

	//@ this was right after all calls to initialize_screen(), so moved here
	if (!no_check)
		no_check = skip_retrace_check;
#else
	if (screen_initialized)
		return;

	/*@
	 * ROGUE_SCR_TYPE should affect both columns and colors, and dos_screen_mode
	 * is also used by game in various contexts with ambiguous meanings.
	 *
	 * My current implementation is "messy", to say the least:
	 * ROGUE_SCR_TYPE is ignored, it does not affect neither columns
	 * (controlled by ROGUE_COLUMNS) nor colors, and dos_screen_mode will be
	 * inconsistent with it if anything but 80-column color mode is used.
	 *
	 * I see 2 elegant approaches to solve this mess:
	 * - dos_screen_mode is "crafted" based on colors and columns, reversing the
	 *   logic in original initialize_screen() switch.
	 * - dos_screen_mode is completely removed, and all tests based on that are
	 *   changed to match original *intention*, if one can figure that out.
	 *
	 * Not to mention game_screen_columns itself should not be defined only at
	 * compile-time, but perhaps also subject to initial terminal size
	 * and/or env file setting.
	 */
	dos_screen_mode = ROGUE_SCR_TYPE;

	setenv("ESCDELAY", "25", FALSE);
	initscr();
	screen_initialized = TRUE;
	if ((LINES < game_screen_rows) || (COLS < game_screen_columns))
	{
		fatal("%u-column mode requires at least a %u x %u screen\n"
				"Your terminal size is %u x %u\n",
				game_screen_columns, game_screen_columns, game_screen_rows, COLS, LINES);
	}
#ifdef ROGUE_DEBUG
	printw("Real terminal size:  %3u x %3u\n", LINES, COLS);
	printw("Setting up Rogue to: %3u x %3u\n", game_screen_rows, game_screen_columns);
#endif
	start_color();
	cbreak();  //@ do not buffer input until ENTER
	noecho();  //@ do not echo typed characters
	nodelay(stdscr, FALSE); //@ use a blocking getch() (already the default)
	keypad(stdscr, TRUE);   //@ enable directional arrows, keypad, home, etc

	resize_screen();
	define_keys();
	init_curses_colors();

	active_attributes = screen_color_count ? color_attributes : monochrome_attributes;
#ifdef ROGUE_DEBUG
	wgetch(stdscr);
#endif
#endif  // ROGUE_DOS_CURSES
	/*@
	 * The only common code in initialize_screen() for both old and new curses.
	 * it was scattered after all initialize_screen() calls, so moved here.
	 * This replaces disabled forcebw()
	 */
	if (monochrome_requested)
		active_attributes = monochrome_attributes;
}

/*@ no longer needed, integrated in initialize_screen()
void
forcebw()
{
	active_attributes = monochrome_attributes;
}
 */

#ifdef ROGUE_DOS_CURSES
/*
 *  save_screen(windex)
 *		dump the screen off to disk, the window is save so that
 *		it can be retieved using windex
 */
void
save_screen()
{
	get_saved_screen();
	dos_read_memory(saved_screen,LINES*COLS,scr_ds,0);
	screen_updates_suspended = TRUE;
}

char *
get_saved_screen()
{
	/*@ savewin is now a fixed size array
	if (savewin == (char *)cell_flags)
		dos_write_memory(savewin,LINES*COLS,0xb800,8192);
	 */
	return(saved_screen);
}

void
release_saved_screen()
{
	/*@ savewin is now a fixed size array
	if (savewin == (char *)cell_flags)
		dos_read_memory(savewin,LINES*COLS,0xb800,8192);
	 */
}

/*
 *	restore_screen(windex):
 *		restor the window saved on disk
 */
void
restore_screen()
{
	dos_write_memory(saved_screen,LINES*COLS,scr_ds,0);
	release_saved_screen();
	screen_updates_suspended = FALSE;
}
#else
/*@
 * Dump the screen to the savewin buffer
 */
void
save_screen(void)
{
	int line;
	int saved_row, saved_column;

	getyx(stdscr, saved_row, saved_column);
	for (line = 0; line < LINES; line++)
	{
		curses_read_cells(line, 0, saved_screen[line], COLS);
	}
	wmove(stdscr, saved_row, saved_column);

	screen_updates_suspended = TRUE;
}

/*@
 * Restore the screen from the savewin buffer
 */
void
restore_screen(void)
{
	int line;
	int saved_row, saved_column;

	getyx(stdscr, saved_row, saved_column);
	for (line = 0; line < LINES; line++)
	{
		curses_restore_cells(line, 0, saved_screen[line], COLS);
	}
	wmove(stdscr, saved_row, saved_column);
	wrefresh(stdscr);

	screen_updates_suspended = FALSE;
}
#endif

/*
 *   close the window file
 *   @renamed from wclose()
 */
void
shutdown_screen()
{
#ifdef ROGUE_DOS_CURSES
	/*
	 * Restor set_cursor_visible (really you want to restor video state, but be carefull)
	 */
	if (dos_screen_mode >= 0)
		set_cursor_visible(TRUE);
	if (page_no != old_page_no)
		switch_page(old_page_no);
#else
	if (screen_initialized)
	{
		endwin();

		/*
		 * Curses resets color RGB based on terminfo, which is somewhat
		 * useless, as (1) few terminals have terminfo default colors
		 * entries (linux does, xterm does not), and (2) those terminfo
		 * colors might not be the current ones before game start: user
		 * might have themed the terminal in .bashrc, .Xresources, etc.
		 *
		 * So we have 2 choices: we can redefine colors back to ANSI's
		 * default RGB, which is also useless has_actor_flag (2), or we can try
		 * `system("type reset 2>/dev/null && reset");`, which reset
		 * colors on some terminals (xterm, but not gnome-terminal)
		 *
		 * There's also a 3rd choice: do nothing! After all, user told
		 * us to change_colors = TRUE, didn't they? So we did it :)
		 *
		 * For now, I'll settle with #3 ;)
		 *
		if (colors_changed)
		{
			// ?
		}
		 */
		screen_initialized = FALSE;

#ifdef ROGUE_DEBUG
		printf("Curses window closed\n");
#endif
	}
#endif
}

/*
 *  Some general drawing routines
 */
int
screen_draw_line(byte dos_character, int length, bool orientation)
{
	chtype character;
#ifdef ROGUE_WIDECHAR
	cchar_t *cch;
#endif  // ROGUE_WIDECHAR

	switch (character_set)
	{
	default:
	case ASCII:
		character = ascii_from_dos(dos_character, box_character_mappings);
		if (character == '\0')
			character = ascii_from_dos(dos_character, game_character_mappings);
		dos_character = (byte)character;
		/* fallthrough */
	case CP437:
		character = dos_character | curses_attributes_from_dos(current_dos_attribute);
		if (orientation == VERTICAL)
			wvline(stdscr, character, length);
		else
			whline(stdscr, character, length);
		break;
#ifdef ROGUE_WIDECHAR
	case UNICODE:
		cch = unicode_from_dos(dos_character, current_dos_attribute, box_character_mappings);
		if (cch->chars[0] == L'\0')
			cch = unicode_from_dos(dos_character, current_dos_attribute, game_character_mappings);
		if (orientation == VERTICAL)
			wvline_set(stdscr, cch, length);
		else
			whline_set(stdscr, cch, length);
		break;
#endif  // ROGUE_WIDECHAR
	}
	return OK;
}

void
screen_draw_box(int top, int left, int bottom, int right)
{
	draw_custom_box(double_box_characters, top, left, bottom, right);
}

/*
		 *  box:  draw a box using given the
		 *        upper left coordinate and the lower right
		 */
void
draw_custom_box(border_characters, top,left,bottom,right)
	byte border_characters[BX_SIZE];
	int top,left,bottom,right;
{
	bool wason;
	int i;
	int row,column;

	wason = set_cursor_visible(FALSE);
	get_cursor_position(&row,&column);

#ifdef ROGUE_DOS_CURSES
	/*
	 * draw horizontal boundry
	 */
	screen_move(top, left+1);
	repeat_character(border_characters[BX_HT], i = (right - left - 1));
	screen_move(bottom, left+1);
	repeat_character(border_characters[BX_HB], i);
	/*
	 * draw vertical boundry
	 */
	for (i=top+1;i<bottom;i++) {
		screen_write_character_at(i,left,border_characters[BX_VW]);
		screen_write_character_at(i,right,border_characters[BX_VW]);
	}
	/*
	 * draw corners
	 */
	screen_write_character_at(top,left,border_characters[BX_UL]);
	screen_write_character_at(top,right,border_characters[BX_UR]);
	screen_write_character_at(bottom,left,border_characters[BX_LL]);
	screen_write_character_at(bottom,right,border_characters[BX_LR]);
#else
	i = (right - left - 1); screen_draw_horizontal_line_at(top, left+1, border_characters[BX_HT], i);
	                       screen_draw_horizontal_line_at(bottom, left+1, border_characters[BX_HB], i);
	i = (bottom - top - 1); screen_draw_vertical_line_at(top+1, left, border_characters[BX_VW], i);
	                       screen_draw_vertical_line_at(top+1, right, border_characters[BX_VW], i);

	//@ corners - do not go through screen_write_character(), different mapping
	screen_draw_horizontal_line_at(top,left,border_characters[BX_UL], 1);
	screen_draw_horizontal_line_at(top,right,border_characters[BX_UR], 1);
	screen_draw_horizontal_line_at(bottom,left,border_characters[BX_LL], 1);
	screen_draw_horizontal_line_at(bottom,right,border_characters[BX_LR], 1);
#endif
	screen_move(row,column);
	set_cursor_visible(wason);
}

/*
 * center a string according to how many columns there really are
 */
void
center(row,string)
	int row;
	char *string;
{
	screen_write_text_at(row,(COLS-strlen(string))/2,string);
}


/*
 * The original printw takes eight explicit formatting arguments. This wrapper uses
 * standard C variadic arguments and vsnprintf to bound the temporary text buffer.
 */
/*
 * printw(Ieeeee)
 */
void
screen_printf(const char *format, ...)
{
	char text[132];
	va_list arguments;

	va_start(arguments, format);
	vsnprintf(text, sizeof(text), format, arguments);
	va_end(arguments);
	screen_write_text(text);
}

#ifdef ROGUE_DOS_CURSES
void
scroll_up(start_row,end_row,nlines)
	int start_row,end_row,nlines;
{
	dos_regs->ax = 0x600 + nlines;
	dos_regs->bx = 0x700;
	dos_regs->cx = start_row << 8;
	dos_regs->dx = (end_row << 8) + COLS - 1;
	call_dos_interrupt(SW_SCR,dos_regs);
	screen_move(end_row,c_col);
}

//@ unused
void
scroll_dn(start_row,end_row,nlines)
	int start_row,end_row,nlines;
{
	dos_regs->ax = 0x700 + nlines;
	dos_regs->bx = 0x700;
	dos_regs->cx = start_row << 8;
	dos_regs->dx = (end_row << 8) + COLS - 1;
	call_dos_interrupt(SW_SCR,dos_regs);
	screen_move(start_row,c_col);
}

//@ unused
void
scroll(void)
{
	scroll_up(0,24,1);
}

/*
 * blot_out region
 *    (upper left row, upper left column)
 *	  (lower right row, lower right column)
 */
void
blot_out(ul_row,ul_col,lr_row,lr_col)
	int ul_row,ul_col,lr_row,lr_col;
{
	dos_regs->ax = 0x600;
	dos_regs->bx = 0x700;
	dos_regs->cx = (ul_row<<8) + ul_col;
	dos_regs->dx = (lr_row<<8) + lr_col;
	call_dos_interrupt(SW_SCR,dos_regs);
	screen_move(ul_row,ul_col);
}

/*
 * try to fixup screen after we get a control break
 * @ unused
 */
void
fixup(void)
{
	blot_out(c_row,c_col,c_row,c_col+1);
}
#endif


/*@
 * Repeat a character cnt times, advancing the cursor
 * Use current attribute, and do not go through screen_write_character() processing
 */
void
repeat_character(byte character, int count)
{
#ifdef ROGUE_DOS_CURSES
	while(count-- > 0) {
		putchr(character);
		saved_column++;
	}
#else
	int saved_row, saved_column;
	getyx(stdscr, saved_row, saved_column);
	screen_draw_horizontal_line(character, count);
	wmove(stdscr, saved_row, saved_column + count);
#endif
}

/*
 * Clear the screen in an interesting fashion
 */
void
animate_level_transition()
{
	int j, delay, row, column, column_step = COLS/10/2, bottom, right;

	bottom = (COLS == 80 ? LINES-3 : LINES-4);
#ifdef ROGUE_DOS_CURSES
	/*
	 * If the curtain is down, just clear the memory
	 */
	if (scr_ds == svwin_ds) {
		wsetmem(saved_screen, (bottom + 1) * COLS, 0x0720);
		return;
	}
	delay = dos_screen_mode == 7 ? 500 : 10;
#else
	delay = 50;
#endif
	for (row = 0,column = 0,right = COLS-1; row < 10; row++,column += column_step,bottom--,right -= column_step) {
		draw_custom_box(single_box_characters, row, column, bottom, right);
		wrefresh(stdscr);
		msleep(delay);
		for (j = row+1; j <= bottom-1; j++) {
#ifdef ROGUE_DOS_CURSES
			screen_move(j, column+1); repeat_character(' ', column_step-1);
			screen_move(j, right-column_step+1); repeat_character(' ', column_step-1);
#else
			screen_draw_horizontal_line_at(j, column+1, ' ', column_step-1);
			screen_draw_horizontal_line_at(j, right-column_step+1, ' ', column_step-1);
#endif
		}
		draw_custom_box(blank_box_characters, row, column, bottom, right);
	}
	wrefresh(stdscr);
}


#ifdef ROGUE_DOS_CURSES
/*
 * drop_curtain:
 *	Close a door on the screen and redirect output to the temporary buffer
 *	@ requires a later call to raise_curtain()
 */
static int old_ds;
void
drop_curtain(void)
{
	register int r, j, delay;

	if (svwin_ds == -1)
		return;
	old_ds = scr_ds;
	dos_read_memory(saved_screen, LINES * COLS, scr_ds, 0);
	set_cursor_visible(FALSE);
	/*@
	 * The different delay for mono and color adapters implies the BIOS call
	 * used by repeat_character()->putchr() is significantly faster under mono video mode,
	 */
	delay = (dos_screen_mode == 7 ? 3000 : 2000);
	green();
	draw_custom_box(single_box_characters, 0, 0, LINES-1, COLS-1);
	yellow();
	for (r = 1; r < LINES-1; r++) {
		screen_move(r, 1);
		repeat_character(PASSAGE, COLS-2);
		for (j = delay; j--; )
			;
	}
	scr_ds = svwin_ds;
	screen_move(0,0);
	cur_standend();
}

void
raise_curtain(void)
{
	register int i, j, o, delay;

	if (svwin_ds == -1)
		return;
	scr_ds = old_ds;
	delay = (dos_screen_mode == 7 ? 3000 : 2000);
	for (i = 0, o = (LINES-1)*COLS*2; i < LINES; i++, o -= COLS*2) {
		dos_write_memory(saved_screen + o, COLS, scr_ds, o);
		for (j = delay; j--; )
			;
	}
}
#else
/*@
 * I am breaking a tradition here by replacing the whole function with a new
 * one instead of changing just the necessary bits of the original.
 * But with <curses.h> and sleep_nanoseconds() the original would be severely
 * mutilated with at least 4 additional #ifdefs blocks, destroying readability,
 * for very little gain. So new it is.
 *
 * Note on the new version: "proper" drop/raise curtain routine should actually
 * redirect all output from addch() and possibly many others, to a buffer such
 * as savewin, just like the original, or to a new curses window/panel/screen.
 * This would require a somewhat complex mechanism of an "effective WINDOW"
 * pointer much similar to the very scr_ds / svwin_ds / old_page_no low level
 * juggling I'm trying to remove from the project.
 *
 * So, for simplicity's sake, currently drop_curtain() merely disables screen
 * (immediate) refresh, and all drawing is still performed stdscr, the curses
 * virtual screen. raise_curtain() takes care of the rest.
 */
/*@
 * Display a curtain down animation and disable screen refresh
 */
void
drop_curtain(void)
{
	int row;
	int delay = CURTAIN_TIME / LINES;

	set_cursor_visible(FALSE);
	green();
	draw_custom_box(single_box_characters, 0, 0, LINES-1, COLS-1);
	curses_read_cells(0, 0, curtain[0], COLS);
	wrefresh(stdscr);
	msleep(delay);  // not in original
	yellow();
	for (row = 1; row < LINES-1; row++) {
		screen_draw_horizontal_line_at(row, 1, FILLER, COLS-2);
		curses_read_cells(row, 0, curtain[row], COLS);
		wrefresh(stdscr);
		msleep(delay);
	}
	curses_read_cells(LINES-1, 0, curtain[LINES-1], COLS);
	msleep(delay);  // not in original, optional
	screen_move(0,0);
	cur_standend();
	wclear(stdscr);
}


/*@
 * Display a curtain up animation and re-enable screen refresh
 */
void
raise_curtain(void)
{
	int line;
	int saved_row, saved_column;
	int delay = CURTAIN_TIME / LINES;

	// save current screen
	getyx(stdscr, saved_row, saved_column);
	save_screen();

	// restore and display the curtain
	for (line = 0; line < LINES; line++)
	{
		curses_restore_cells(line, 0, curtain[line], COLS);
	}

	// progressively restore screen
	for (line = LINES-1; line >= 0; line--)
	{
		curses_restore_cells(line, 0, saved_screen[line], COLS);
		wrefresh(stdscr);
		msleep(delay);
	}
	wmove(stdscr, saved_row, saved_column);
	screen_updates_suspended = FALSE;
}
#endif


#ifdef ROGUE_DOS_CURSES
void
switch_page(pn)
	int pn;
{
	register int pgsize;

	if (dos_screen_mode == 7) {
		page_no = 0;
		return;
	}
	if (COLS == 40)
		pgsize = 2048;
	else
		pgsize = 4096;
	dos_regs->ax = 0x0500 | pn;
	call_dos_interrupt(SW_SCR, dos_regs);
	scr_ds = 0xb800 + ((pgsize * pn) >> 4);
	page_no = pn;
}
#endif


byte
get_dos_video_mode(void)
/*@
 * Get current video mode using software interrupt 10h, AH=0Fh
 *
 * Response is:
 * AL = Video Mode, AH = number of character columns, BH = active page
 * https://en.wikipedia.org/wiki/INT_10H
 * http://www.ctyme.com/intr/rb-0108.htm
 *
 * Return only AL, the low A byte representing Video Mode, as a 16-bit int
 * For EGA text, AL is 03h for color or 07h for monochrome
 */
{
	struct dos_registers dos_regs;

	dos_regs.ax = 0xF00;  //@ AH = 0Fh
	call_dos_interrupt(SW_SCR,&dos_regs);
	return 0xff & dos_regs.ax;
}

/*@
 * Set current video mode using software interrupt 10h, AH=00h
 *
 * Response is:
 * AL = Video Mode or CRT controller
 * http://www.ctyme.com/intr/rb-0069.htm
 *
 * Return AL
 */
byte
set_dos_video_mode(type)
	int type;
{
	struct dos_registers dos_regs;

	dos_regs.ax = type;
	call_dos_interrupt(SW_SCR,&dos_regs);
	return dos_regs.ax;
}


/*
 * This routine reads information from the keyboard
 * It should do all the strange processing that is
 * needed to retrieve sensible data from the user
 *
 * @ "Strange processing" indeed:
 * - ESCAPE abort the input, set the first character of str to ESCAPE but leave
 *   all other typed characters there. It does *NOT* null-terminate str!!!
 *   Like in printw(), it couldn't care less about buffer exploits.
 *   Return ESCAPE.
 * - '\n' finishes input and null-terminate str. '\n' is not included in str.
 *   Return '\n'
 * - A non-ascii char (>127) also finishes input, but it *does* get included
 *   in the buffer. The source does not establish whether including it was intended.
 *   Return the non-ascii char.
 * - All other chars are accepted as normal input, including symbols (< 32).
 *
 * Original behavior is changed:
 * - Aborted input are null-terminated (ESCAPE + '\0')
 * - Only printable ASCII chars accepted (32 <= ch <= 126). This is universally
 *   compatible, until proper CP437 and UTF-8 support is implemented.
 *
 * In a sane, safe API this function would return a bool, FALSE if aborted
 * by ESCAPE and TRUE otherwise. In case of abortion, str could either
 * keep typed string or set first char to '\0', effectively blanking str.
 */
int
read_line(text,size)
	char *text;
	int size;
{
	register char *input_start;
	int character;
	int input_length = 0;
	int previous_cursor_visibility, result = 1;
#ifdef ROGUE_DOS_CURSES
	/*
	 * The retained DOS branch snapshots 80 screen cells at the start of input, then
	 * restores them at row zero. That hard-coded row does not follow the input cursor.
	 * The native branch leaves input visible; callers manage their own screen contents.
	 */
	char buf[160];

	dos_read_memory(buf, 80, scr_ds, 0);
#endif
	input_start = text;
	*text = 0;
	previous_cursor_visibility = set_cursor_visible(TRUE);
	while(result == 1)
	{
#ifdef ROGUE_DOS_CURSES
		if((character = cur_getch()) == EOF)
		{
			character = '\n';
		}
#else
		//@ Blocking getch() is fine, as update_keyboard_and_clock() is not called anyway
		while ((character = wgetch(stdscr)) == ERR);
		if (character > KEY_MIN)
		{
			character = TERMINAL_KEY_MASK & character;
		}
#endif
		switch(character)
		{
			case ESCAPE:
				while(text != input_start) {
					backspace();
					input_length--;
					text--;
				}
				//@ null-termination was not in original
				result = *text++ = ESCAPE;
				*text = 0;
				set_cursor_visible(previous_cursor_visibility);
				break;
#ifndef ROGUE_DOS_CURSES
			case KEY_RESIZE:
				resize_screen();
				break;
			case KEY_BACKSPACE:
#endif
			case '\b':
				if (text != input_start) {
					backspace();
					input_length--;
					text--;
				}
				break;
			default:
				if ( input_length >= size) {
					beep();
					break;
				}
				if (!isprint(character))
				{
					break;
				}
				input_length++;
				addch(character);
				*text++ = character;
				break;
#ifndef ROGUE_DOS_CURSES
			case KEY_ENTER:
#endif
			case '\n':
				*text = 0;
				set_cursor_visible(previous_cursor_visibility);
				result = character;  //@ any value different than ESCAPE or 1 would do.
				break;
		}
	}
#ifdef ROGUE_DOS_CURSES
	dos_write_memory(buf, 80, scr_ds, 0);
#endif
	return result;
}

/*@
 * No need of #ifdef ROGUE_DOS_CURSES here as the non-curses input of getch()
 * used by read_line() is performed by <stdio.h> getchar(), which is line
 * buffered anyway, so a backspace char will never be seen and this will never
 * be called.
 */
void
backspace(void)
{
	int row, column;
	getyx(stdscr, row, column);
	if (column > 0)
		wmove(stdscr, row, column-1);
	wdelch(stdscr);
	winsch(stdscr, ' ');
}
