/*
 * Private declarations for the screen implementation, including the retained DOS
 * backend and the native ncurses character/attribute conversions.
 */

#ifndef ROGUE_SCR_TYPE
#define ROGUE_SCR_TYPE 3  //@ 80x25 Color. See initialize_screen() for other values
#endif

#define BX_UL	0
#define BX_UR	1
#define BX_LL	2
#define BX_LR	3
#define BX_VW	4
#define BX_HT	5
#define BX_HB	6
#define BX_SIZE	7

//@ max is also in rogue.h
#define max(a,b)	((a) > (b) ? (a) : (b))
#define min(a,b)	((a) < (b) ? (a) : (b))

#ifdef ROGUE_DOS_CURSES
//@ in ncurses, as unsigned long. see wsetmem()
typedef uint16_t	chtype;  // character with attributes
#endif  // ROGUE_DOS_CURSES


/*@
 * Function prototypes
 * Some are unused
 */
char	*get_saved_screen(void);
void	release_saved_screen(void);
void	draw_custom_box(byte border_characters[BX_SIZE], int top, int left, int bottom, int right);
#ifdef ROGUE_DOS_CURSES
void	real_rc(int pn, int *rp, int *cp);
void	error(int mline, char *show_message, int a1, int a2, int a3, int a4, int a5);
void	set_cursor(void);
void	scroll_up(int start_row, int end_row, int nlines);
void	scroll_dn(int start_row, int end_row, int nlines);
void	scroll(void);
void	fixup(void);

//@ originally in zoom.asm
void	putchr(byte ch);

//@ originally in dos.asm
void	wsetmem(void *buffer, int command_repeat_count, chtype attrchar);
#endif  // ROGUE_DOS_CURSES

/*@
 * New stuff from now on
 */

#define A_DOS_BLACK    0x00
#define A_DOS_BLUE     0x01
#define A_DOS_GREEN    0x02
#define A_DOS_RED      0x04
#define A_DOS_WHITE    0x07
#define A_DOS_BRIGHT   0x08
#define A_DOS_BLINK    0x80

#define A_DOS_CYAN     A_DOS_BLUE  | A_DOS_GREEN
#define A_DOS_MAGENTA  A_DOS_BLUE  | A_DOS_RED
#define A_DOS_BROWN    A_DOS_GREEN | A_DOS_RED
#define A_DOS_YELLOW   A_DOS_BROWN | A_DOS_BRIGHT

#define A_DOS_FG_COLOR    0  // foreground color shift offset
#define A_DOS_BG_COLOR    4  // background color shift offset
#define A_DOS_COLOR_MASK  7  // to extract color after shifting

#define A_DOS_BG(fg)	(((fg) & A_DOS_COLOR_MASK) << A_DOS_BG_COLOR)

/*@
 * The DOS attribute set on standend(), also the initial value of current_dos_attribute
 * Light Gray ("non-bright White") on Black
 */
#define A_DOS_NORMAL	A_DOS_WHITE

// AKA "reverse"
#define A_DOS_STANDOUT	A_DOS_BG(A_DOS_WHITE)

/*
 * Actually, for underline just setting foreground to 1 would be enough,
 * but the game uses 17 (0x11) in uline()/set_display_attribute(12) for monochrome_attributes,
 * setting
 * also the background. Not needed, but harmless
 */
#define A_DOS_BW_ULINE    1 | A_DOS_BG(1)
#define A_DOS_BW_STANDOUT A_DOS_STANDOUT | A_DOS_BRIGHT

#ifndef ROGUE_DOS_CURSES
#define PAIR_INDEX(fg, bg)	(bg * screen_color_count + fg + 1)
#define COLOR_PAIR_N(fg, bg)	COLOR_PAIR(PAIR_INDEX(fg, bg))

/*
 * Original CGA colors
 * https://en.wikipedia.org/wiki/Color_Graphics_Adapter#Color_palette
 * red   := 2/3 * (colorNumber & 4)/4 + 1/3 * (colorNumber & 8)/8
 * green := 2/3 * (colorNumber & 2)/2 + 1/3 * (colorNumber & 8)/8
 * blue  := 2/3 * (colorNumber & 1)/1 + 1/3 * (colorNumber & 8)/8
 * if colorNumber = 6 then green := green / 2
 *
 * These macros swap Red and Blue components, so `c` is ANSI index (brown is 3)
 * Return a float in range [0, 1] (both ends included!)
 */
#define CGA_COMP(c, i)	(!!((c) & i) * 2 / 3.0 + !!((c) & 8) * 1 / 3.0)
#define CGA_RED(c)	 CGA_COMP(c, 1)
#define CGA_GREEN(c)	(CGA_COMP(c, 2) / ((c) == 3 ? 2 : 1))
#define CGA_BLUE(c)	 CGA_COMP(c, 4)

#define ARRAY_LENGTH(arr)	(sizeof (arr) / sizeof (*arr))
#define ARRAY_END(arr)	(arr + ARRAY_LENGTH(arr))

#define HORIZONTAL TRUE
#define VERTICAL   FALSE

#define screen_draw_horizontal_line(chd, length)	screen_draw_line(chd, length, HORIZONTAL)
#define screen_draw_vertical_line(chd, length)	screen_draw_line(chd, length, VERTICAL)
#define screen_draw_horizontal_line_at(y,x,c,n)	(screen_move(y,x) == ERR ? ERR : screen_draw_horizontal_line(c, n))
#define screen_draw_vertical_line_at(y,x,c,n)	(screen_move(y,x) == ERR ? ERR : screen_draw_vertical_line(c, n))

#ifdef ROGUE_WIDECHAR
#define curses_restore_cells	mvadd_wchnstr
#define curses_read_cells	mvin_wchnstr
#else
#define curses_restore_cells	mvaddchnstr
#define curses_read_cells	mvinchnstr
#endif  // ROGUE_WIDECHAR

#define TTY_ESC "\033"
#define TTY_CSI TTY_ESC "["
#define TTY_SS3 TTY_ESC "O"

struct terminal_key_sequence {
	char *def;
	int dest;
};
typedef struct terminal_key_sequence TerminalKeySequence;

struct character_mapping {
	byte ascii;
	wchar_t *unicode;
	byte dos;
};
typedef struct character_mapping CharacterMapping;

#ifdef ROGUE_WIDECHAR
cchar_t *unicode_from_dos(byte dos_character, byte dos_attr, CharacterMapping *mapping);
void	wide_attributes_from_dos(byte dos_attr, attr_t *attrs, short *color_pair);
#endif  // ROGUE_WIDECHAR
void	define_keys(void);
byte	ascii_from_dos(byte dos_character, CharacterMapping *mapping);
CharacterMapping	*charcode_from_dos(byte dos_character, CharacterMapping *mapping);
short	color_from_dos(byte dos_attr, bool foreground);
chtype	curses_attributes_from_dos(byte dos_attr);
void	init_curses_colors(void);
void	resize_screen(void);
int	screen_draw_line(byte dos_character, int length, bool orientation);
#endif  // not ROGUE_DOS_CURSES
