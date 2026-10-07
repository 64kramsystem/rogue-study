/*
 * save and restore routines
 *
 * save.c	1.32	(A.I. Design)	12/13/84
 */

/*
 *  routines for saving a program in any given state.
 *  This is the first pass of this, so I have no idea
 *  how it is really going to work.
 *  The two basic functions here will be "save" and "restor".
 */

#include "rogue.h"
#include "screen.h"

/*
 * Placeholders for the original linker-provided static-data boundaries. They do not
 * contain the native game's globals. The legacy memory-dump implementation below is
 * unreachable because save_game returns and restore_game exits before using it.
 */
char legacy_static_placeholder[1234];  // Not the original data-segment size.
char * legacy_static_start = legacy_static_placeholder;  /* Adresss of first save-able memory */
char * legacy_static_end = legacy_static_placeholder + sizeof(legacy_static_placeholder);  /* Address of end of user data space */

/*
 * The original init_ds assigns end_sb to the first allocation (_flags) and startmem
 * to the first temporary printing buffer (tbuf). Their difference covers the map,
 * entity pool, and allocation flags, excluding the temporary buffers. These placeholder
 * arrays only keep the unreachable DOS implementation linkable in the native port.
 */
char legacy_heap_placeholder[4321];  // Not the original heap size.
char *legacy_heap_start = legacy_heap_placeholder;  /* Pointer to the end of static base */
char *legacy_heap_end = legacy_heap_placeholder + sizeof(legacy_heap_placeholder);  /* Pointer to the start of static memory */


#define SAVE_SIGNATURE_SIZE 10
static char *save_signature = "AI Design";

/*
 * SAVE_BLOCK_SIZE: size of block read/written on each io operation
 *        Has to be less than 4096 and a factor of 4096
 *        so the screen can be read in exactly.
 */
#define SAVE_BLOCK_SIZE 512

static int	write_legacy_memory_dump(char *filename);

/*
 * save_game:
 *	Implement the "save game" command
 */
void
save_game()
{
#ifndef DEMO
	int result;
	char filename[20];

	show_message("Sorry, saving games is disabled. Patches are welcome!");
	return;

	show_message("");
	message_column = 0;
	if (terse)
		addstr("Save file ? ");
	else
		printw("Save file (press enter (\x11\xd9) to default to \"%s\") ? ",
				save_filename );
	/*@
	 * FIXME
	 * Previous message length + 19 input chars > 80 columns, message will
	 * wrap to 2nd line, overwriting the dungeon. This bug happened in
	 * original too, if "savefile" entry in ROGUE.OPT had length 14.
	 * Not a problem if save is successful, as game will exit afterwards,
	 * but in case of any non-fatal error dungeon will be corrupt.
	 *
	 * In any case, this UI must be redesigned for filenames beyond DOS 8+3.
	 * Possible approach: save 2nd line, use it for input (80/40 char limit
	 * is acceptable), then restore line on errors.
	 */
	result = read_line(filename,19);
	if (*filename == 0)
		strcpy(filename,save_filename);
	show_message("");
	message_column = 0;
	if (result != ESCAPE)
	{
		if ((result = write_legacy_memory_dump(filename)) == -1)
		{
			if (remove(filename) == 0)
				message_by_verbosity1("out of space?","out of space, can not write %s",filename);
			show_message("Sorry, you can't save the game just now");
			//@ screen_updates_suspended = FALSE;  //@ restore_screen() did that already
		}
		else if (result > 0)
			fatal("\nGame saved as %s.", filename);
	}
#endif
}
#ifndef DEMO
/*
 *  Save:
 * 		Determine the entire data area that needs to be saved,
 *		Open save file, first write in to save file a header
 *		that demensions the data area that will be saved,
 *		and then dump data area determined previous to opening
 *		file.
 */
static
int
write_legacy_memory_dump(filename)
	char *filename;
{
	register FILE *file;
	register char answer;

	if ((file = fopen(filename, "r")) != NULL)
	{
		fclose(file);
		show_message("%s %sexists, overwrite (y/n) ?",filename,verbose_text("already "));
		answer = read_game_key();
		show_message("");
		if ((answer != 'y') && (answer != 'Y'))
			return(-2);
	}

	if ((file = fopen(filename, "w")) == NULL)
	{
		show_message("Could not create %s",filename);
		return (-2);
	}
	//@ screen_updates_suspended = TRUE;  //@ save_screen() will do that
	message_column = 0;

	errno = 1;
	if ( ! fwrite(save_signature, SAVE_SIGNATURE_SIZE, 1, file)
		|| ! fwrite(&legacy_static_start , &legacy_static_end - &legacy_static_start, 1, file)
			|| ! fwrite(legacy_heap_start, legacy_heap_end - legacy_heap_start, 1, file))
			goto wr_err;
	/*
	 * save the screen (have to bring it into current data segment first)
	 */
	save_screen();
	if (fwrite(saved_screen, 4000, 1, file))
		errno = 0;
	restore_screen();

wr_err:
	fclose(file);
	switch (errno)
	{
		default:
			show_message("Could not write savefile to disk!");
			return -1;
		case 0:
			move(24,0);
			clrtoeol();
			move(23,0);
			return 1;
	}
}
#endif //DEMO

/*
 *	Restore:
 *		Open saved data file, read in header, and determine how much
 *		data area is going to be restored,
 *		Close save data file,
 *		Allocate enough data space so that open data file information
 *		will be stored outside the data area that will be restored,
 *		Now reopen data save file,
 *		skip header,
 *		dump into memory all saved data.
 */
void
restore_game(char *savefile)
{
	fatal("Sorry, restoring games is disabled. Patches are welcome!\n");

/*
 * In the archived DEMO build, restore has no body, but playit still calls it and
 * then setup. The main restore branch skips init_player and the normal new-game
 * initialization. The source therefore contains no successful demo restore path.
 */
#ifndef DEMO
	int saved_major_version, saved_minor_version; //@, old_check;
	register int saved_columns;
	register FILE *file;
	char error_text[11], save_name[MAXSTR];
	char *read_error = "Read Error";
	struct dos_registers *saved_registers;
	unsigned byte_count;
	char signature[SAVE_SIGNATURE_SIZE];

	saved_registers = dos_regs;
	initialize_screen();
	//@ old_check = no_check;  //@ no_check is now inside initialize_screen()
	strcpy(error_text,read_error);
	/*
	 * save things that will be bombed on when the
	 * restor takes place
	 */
	saved_major_version = version_major;
	saved_minor_version = version_minor;

	if (!strcmp(copy_protection_drive,"?"))
	{
		int saved_video_mode = dos_screen_mode;
		printw("Press space to restart game");
		dos_screen_mode = -1;
		wait_for_key(' ');
		dos_screen_mode = saved_video_mode;
		addstr("\n");
	}
	if ((file = fopen(savefile, "r")) == NULL)
		fatal("%s not found\n",savefile);
	else
		printw("Restoring %s",savefile);
	strcpy(save_name, savefile);
	byte_count = &legacy_static_end - &legacy_static_start;
	if (fread(signature, SAVE_SIGNATURE_SIZE, 1, file) || strcmp(signature,save_signature) )
		addstr("\nNot a savefile\n");
	else
	{
		if (fread(&legacy_static_start, byte_count, 1, file))
			if (fread(legacy_heap_start, legacy_heap_end - legacy_heap_start, 1, file))
				goto rok;
		addstr(error_text);
	}
	fclose(file);
	exit_game(EXIT_FAILURE);

rok:
	dos_regs = saved_registers;
	if (version_major != saved_major_version || version_minor != saved_minor_version)
	{
		fclose(file);
		exit_game(EXIT_FAILURE);
	}

	saved_columns = COLS;
	//@ no longer needed, memory is now managed via malloc()
	//@ brk(legacy_heap_start);					/* Restore heap to empty state */
	allocate_game_state();
	/*
	 * In the original DOS code, brk(end_sb) resets the allocator break; init_ds then
	 * repeats the same allocation order. sbrk.asm returns the previous break and advances
	 * it without clearing the allocated bytes, preserving the restored state at those
	 * addresses. begin.asm establishes the DOS data/stack segment before game startup.
	 *
	 * These allocator calls do not map or protect pages. That explains how the original
	 * could write within its segment before advancing the break, but does not prove that
	 * an arbitrary saved image fits safely below the stack. Separate malloc allocations
	 * in the native port cannot reproduce this layout.
	 */
	endwin();
	initialize_screen();
	if (saved_columns != COLS)
	{
		fclose(file);
		fatal("Restore Error: new screen size\n");
	}

	save_screen();
	if (!fread(saved_screen, 4000, 1, file))
	{
		fclose(file);
		fatal("Serious restore error");
	}
	restore_screen();

	fclose(file);
	//@ no_check = old_check;  //@ no longer your concern
	message_column = 0;
	message_by_verbosity1("%s, Welcome back!","Hello %s, Welcome back to the Dungeons of Doom!",player_name);
	initial_random_seed = random_seed_from_clock();     /* make it a little tougher on cheaters */
	remove(save_name);
#endif //DEMO
}
