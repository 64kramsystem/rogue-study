/*
 * routines for writing a fake dos
 *
 * @FIXME: Scrolling is bugged. Need support from curses.c to expose scrlok()
 *         or perhaps a higher-level API to properly enable and disable it.
 *
 * @FIXME: Originally it was possible to leave fakedos with a non-ASCII key
 *         as the first command char (for example, arrow keys or F1-F12). But
 *         read_line() now ignores all non-ASCII input, so the only way to leave
 *         is to type the "command" `rogue`. Fixing this will be tricky...
 */

#include	"rogue.h"
#include	"curses.h"

static bool	execute_fake_dos_command(char *com);
static int	select_drive(int drv);

void
show_fake_dos(void)
{
	char comline[132];
	char savedir[] = "a:", *comhead;

	save_screen();
	clear();
	move (0,0);
	set_cursor_visible(TRUE);
#ifdef ROGUE_DOS_DRIVE
	*savedir = dos_service(0x19,0) + 'A';
#else
	*savedir = current_drive + 'A';  //@ save current drive
#endif
	do {
		fill_bytes(comline, sizeof(comline), 0);
#ifdef ROGUE_DOS_DRIVE
		printw("\n%c>",dos_service(0x19,0)+'A');
#else
		printw("\n%c>",current_drive+'A');
#endif
		read_line(comline,130);
		comhead = skip_whitespace(comline);
		trim_trailing_whitespace(comhead);
	} while (execute_fake_dos_command(comhead));
	execute_fake_dos_command(savedir);  //@ restore current drive
	set_cursor_visible(FALSE);
	clear();
	restore_screen();
}

/*
 * execute a dos like command
 */
static
bool
execute_fake_dos_command(com)
	char *com;
{
	int drv;

	if ((!isascii(*com)) || (strcmp(com, "rogue") == 0))
	{
		return FALSE;
	}
	if (com[1] == ':' && com[2] == 0)
	{
		//@ smart way to get toupper() and 'A'=>0 in a single strike
		drv = (*com & 0x1f) - 1;

		printw("\n");
		if ((!is_alpha(*com)) || drv >= select_drive(drv))
		{
			printw("Invalid drive specification\n");
		}
	}
	else if (com[0])
	{
		printw("\nBad command or file name\n");
	}
	return TRUE;
}

/*
 * Emulate DOS INT 21h/AH=0Eh drive selection for the fake DOS screen.
 * Original called dos_service() directly in execute_fake_dos_command()
 */
static
int
select_drive(int drv)
{
#ifdef ROGUE_DOS_DRIVE
	return dos_service(0x0e, drv);
#else
	if (drv >= 0 && drv <= last_drive)
	{
		current_drive = drv;
	}
	return last_drive;
#endif
}
