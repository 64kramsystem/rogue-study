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
#include "screen.h"

static bool	execute_fake_dos_command(char *command_text);
static int	select_drive(int drive_index);

void
show_fake_dos(void)
{
	char command_buffer[132];
	char saved_drive[] = "a:", *command_start;

	save_screen();
	clear();
	move (0,0);
	set_cursor_visible(TRUE);
#ifdef ROGUE_DOS_DRIVE
	*saved_drive = dos_service(0x19,0) + 'A';
#else
	*saved_drive = current_drive + 'A';  //@ save current drive
#endif
	do {
		fill_bytes(command_buffer, sizeof(command_buffer), 0);
#ifdef ROGUE_DOS_DRIVE
		printw("\n%c>",dos_service(0x19,0)+'A');
#else
		printw("\n%c>",current_drive+'A');
#endif
		read_line(command_buffer,130);
		command_start = skip_whitespace(command_buffer);
		trim_trailing_whitespace(command_start);
	} while (execute_fake_dos_command(command_start));
	execute_fake_dos_command(saved_drive);  //@ restore current drive
	set_cursor_visible(FALSE);
	clear();
	restore_screen();
}

/*
 * execute a dos like command
 */
static bool
execute_fake_dos_command(char *command_text)
{
	int drive_index;

	if ((!isascii(*command_text)) || (strcmp(command_text, "rogue") == 0))
	{
		return FALSE;
	}
	if (command_text[1] == ':' && command_text[2] == 0)
	{
		//@ smart way to get toupper() and 'A'=>0 in a single strike
		drive_index = (*command_text & 0x1f) - 1;

		printw("\n");
		if ((!is_alpha(*command_text)) || drive_index >= select_drive(drive_index))
		{
			printw("Invalid drive specification\n");
		}
	}
	else if (command_text[0])
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
select_drive(int drive_index)
{
#ifdef ROGUE_DOS_DRIVE
	return dos_service(0x0e, drive_index);
#else
	if (drive_index >= 0 && drive_index <= last_drive)
	{
		current_drive = drive_index;
	}
	return last_drive;
#endif
}
