/*@
 * Header for curses-related objects used by both the game files and
 * the internal curses.c
 *
 * Some of the defines, the ones part of an informal group such as the colors,
 * are not actually used by both, and may even not be used at all, but were
 * kept together here for consistency and completeness.
 */

//@ not in original, to make IDE happy about 'bool'
#include "extern.h"

//@ Available charsets, not in original
#define ASCII	1
#define CP437	2
#define UNICODE	3

//@ User-selected charset, also not in original
#if !(ROGUE_CHARSET == ASCII || ROGUE_CHARSET == CP437 || ROGUE_CHARSET == UNICODE)
	//@ Factory default charset
	#define ROGUE_CHARSET	UNICODE
#endif
//@ UNICODE is subject to curses wide char availability
#if ROGUE_CHARSET == UNICODE && !defined (_XOPEN_CURSES)
	#undef  ROGUE_CHARSET
	#define ROGUE_CHARSET	ASCII
#endif
//@ Only enable wide chars if actually needed
#if ROGUE_CHARSET == UNICODE
	#define ROGUE_WIDECHAR
#endif


//@ Columns mode - should (but currently isn't) be selected at run-time
#ifndef ROGUE_COLUMNS
#define ROGUE_COLUMNS 80
#endif

/*
 * Don't change the constants, since they are used for sizes in many
 * places in the program.
 * @ Heed the warning! 80 and 25 are hard coded in many places... sigh
 * @ moved from rogue.h
 */
#define MAXSTR  	80	/* maximum length of strings */
#define MAXLINES	25	/* maximum number of screen lines used */
#define MAXCOLS 	80	/* maximum number of screen columns used */


/*@
 * This color/bw checks are inconsistent with each other:
 * scr_type 0 and 2 evaluate as TRUE for both (but they are mono),
 * scr_type 7 evaluate as FALSE for both (also mono)
 * See winit()
 */
#define is_color (dos_screen_mode!=7)
#define is_bw (dos_screen_mode==0 || dos_screen_mode==2)

//@ moved from rogue.h
#ifndef CTRL
#define CTRL(ch)	((ch) & 037)
#endif

#define cur_standend() set_display_attribute( 0)  //@ normal white (light gray) on black
#define green()        set_display_attribute( 1)
#define cyan()         set_display_attribute( 2)
#define red()          set_display_attribute( 3)
#define magenta()      set_display_attribute( 4)
#define brown()        set_display_attribute( 5)  //@ yellow, made brown by CGA hardware
#define dgrey()        set_display_attribute( 6)  //@ "bright black". unused
#define lblue()        set_display_attribute( 7)
#define lgrey()        set_display_attribute( 8)  //@ bright *green*, not gray. unused
#define lred()         set_display_attribute( 9)
#define lmagenta()     set_display_attribute(10)
#define yellow()       set_display_attribute(11)
#define uline()        set_display_attribute(12)  //@ bright white on color, underline on bw
#define blue()         set_display_attribute(13)
#define cur_standout() set_display_attribute(14)  //@ black on normal white (reverse)
#define high()         set_display_attribute(15)  //@ bright white on color, normal on bw
#define bold()         set_display_attribute(16)  //@ black on normal white (reverse)

#define cur_getch()	screen_read_key(-1)

/*
 * Things that appear on the screens
 * @ moved from rogue.h
 */
#define PASSAGE		(0xb1)
#define DOOR		(0xce)
#define FLOOR		(0xfa)
#define PLAYER		(0x01)
#define TRAP		(0x04)
#define STAIRS		(0xf0)
#define GOLD		(0x0f)
#define POTION		(0xad)
#define SCROLL		(0x0d)
#define MAGIC		'$'
#define BMAGIC		'~'  //@ originally '+'. Reverse ASCII map must be unique
#define FOOD		(0x05)
#define STICK		(0xe7)
#define ARMOR		(0x08)
#define AMULET		(0x0c)
#define RING		(0x09)
#define WEAPON		(0x18)
#define CALLABLE	-1

#define VWALL	(0xba)
#define HWALL	(0xcd)
#define ULWALL	(0xc9)
#define URWALL	(0xbb)
#define LLWALL	(0xc8)
#define LRWALL	(0xbc)

//@ The following were not in original - values were hard-coded

//@ single-width box glyphs
#define HLINE	(0xc4)
#define VLINE	(0xb3)
#define CORNER	'+'  //@ unused, added just for completeness
#define ULCORNER	(0xda)
#define URCORNER	(0xbf)
#define LLCORNER	(0xc0)
#define LRCORNER	(0xd9)

//@ double-width box glyphs
#define DHLINE	HWALL  // 205 in credits()
#define DVLINE	VWALL
#define DCORNER	'#'  //@ also unused
#define DULCORNER	ULWALL
#define DURCORNER	URWALL
#define DLLCORNER	LLWALL
#define DLRCORNER	LRWALL

//@ only used in credits()
#define DVLEFT	(0xb9)  //@ 185
#define DVRIGHT	(0xcc)  //@ 204

//@ only used in drop_curtain()
#define FILLER	PASSAGE


//@ moved from rogue.h
#define ESCAPE	(27)

//@ same as ERR, but different semantics
#define NOCHAR	(-1)

//@ total time, in milliseconds, for each drop and raise curtain animation
#define CURTAIN_TIME	1500

/*@
 * Function prototypes
 * Names with 'cur_' prefix were renamed to avoid conflict with <curses.h>
 */
byte	translate_key(int character);
void	screen_clear(void);
bool	set_cursor_visible(bool visible);
void	get_cursor_position(int *row, int *column);
void	screen_refresh(void);
void	screen_clear_to_eol(void);
void	screen_write_text_at(int row, int column, char *s);
void	screen_write_character_at(int row, int column, byte character);
byte	screen_read_character_at(int row, int column);
void	screen_write_character(byte character);
void	screen_write_text(char *s);
void	set_display_attribute(int attribute_index);
void	initialize_screen(void);
void	save_screen(void);
void	restore_screen(void);
void	shutdown_screen(void);
void	screen_draw_box(int top, int left, int bottom, int right);
void	center(int row, char *string);
void	screen_printf(const char *format, ...);
void	repeat_character(byte character, int count);
void	animate_level_transition(void);
void	drop_curtain(void);
void	raise_curtain(void);
byte	get_dos_video_mode(void);
byte	set_dos_video_mode(int type);
#ifdef ROGUE_DOS_CURSES
void	switch_page(int pn);
void	blot_out(int ul_row, int ul_col, int lr_row, int lr_col);
#endif

//@ originally in dos.asm
void	screen_beep(void);
int 	screen_read_key(int timeout_ms);

//@ originally in zoom.asm
int	screen_move(int row, int col);
byte	screen_read_character(void);

//@ moved from io.c
int 	read_line(char *text, int size);
void	backspace(void);
