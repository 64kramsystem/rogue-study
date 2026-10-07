/*
 *  Cursor motion header for Monochrome display
 */

/*@
 * Contains only the curses-related declarations needed for the game files.
 *
 * Headers used only by curses.c were moved to curses_dos.h
 * Headers used by both curses.c and game files were moved to curses_common.h
 * Headers not related to curses or provided externally were moved elsewhere.
 * Unused headers were removed.
 *
 * The DOS curses implementation, curses.c, shall NOT include this header
 *
 * This is, along with the included curses_common.h header, is the curses
 * public API as used by the game.
 */

#include "curses_common.h"

#define stdscr	NULL
#define hw	stdscr
#define ignored_window	stdscr

//@ Original macros
#define	wclear	clear
#define mvwaddch(w,a,b,c)	mvaddch(a,b,c)
#define getyx(a,b,c)	getxy(&b,&c)
#define getxy	get_cursor_position

//@ Modified macros
#define inch	screen_read_character
#define standend	cur_standend
#define standout	cur_standout
#define endwin	shutdown_screen

//@ Function mappings
#define beep	screen_beep
#define move	screen_move
#define clear	screen_clear
#define clrtoeol	screen_clear_to_eol
#define mvaddstr	screen_write_text_at
#define mvaddch	screen_write_character_at
#define mvinch	screen_read_character_at
#define addch	screen_write_character
#define addstr	screen_write_text
#define box	screen_draw_box
#define printw	screen_printf
#define getch	cur_getch  //@ no longer used
#define getch_timeout	screen_read_key


/*@
 * Global variables declarations. All defined in curses.c
 */
extern int LINES, COLS;
extern int screen_updates_suspended;
extern int dos_screen_mode;
#ifdef ROGUE_DOS_CURSES
extern bool iscuron;
extern int old_page_no;
extern int scr_ds;
extern int svwin_ds;
#endif

/*
 * we need to know location of screen being saved
 * @ used in save.c
 */
extern char saved_screen[];
