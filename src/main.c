/*
 * ###   ###   ###  #   # #####
 * #  # #   # #   # #   # #
 * #  # #   # #   # #   # #
 * ###  #   # #     #   # ###
 * #  # #   # #  ## #   # #
 * #  # #   # #   # #   # #
 * #  #  ###   ###   ###  #####
 *
 * Exploring the Dungeons of Doom
 * Copyright (C) 1981 by Michael Toy, Ken Arnold, and Glenn Wichman
 * main.c	1.4 (A.I. Design) 11/28/84
 * All rights reserved
 * Copyright (C) 1983 by Mel Sibony, Jon Lane (AI Design update for the IBMPC)
 */

#include "rogue.h"
#include "screen.h"

#define is_key(s) ((*s=='-')||(*s=='/'))
#define is_char(c1,c2) ((c1==c2)||((c1+'a'-'A')==c2))

//@ both derived from `screen` in env file and used in curses.c
int monochrome_requested = FALSE;
#ifdef ROGUE_DOS_CURSES
char skip_retrace_check = FALSE;
#endif
#ifdef LOGFILE
int log_read, log_write;
#endif

/*
 * main:
 *	The main program, of course
 */
int
main(argc, argv)
	int argc;
	char **argv;
{
	register char *argument, *saved_game_path=0;

	//@ Allow non-ASCII output in <curses.h>
	setlocale(LC_ALL, "");

#ifdef ROGUE_DOS_CLOCK
	long junk = 0L;
	/*
	 * Clear the four-byte single-step interrupt vector at 0000:0004 (INT 01h).
	 * The original dos.asm clock_ handler reads both words and calls _halt_ if either
	 * is nonzero. This write establishes the value that the anti-debugger check expects.
	 */
	dos_write_memory(&junk,2,0,4);
	install_dos_timer_hook();
#endif
#ifdef ROGUE_SPLASH
	show_sdl_splash(getenv("ROGUE_PIC"));
#else
	show_dos_splash();
#endif //ROGUE_SPLASH
	allocate_game_state();

	load_options_file(ENVFILE);
	authenticate_game_disk(find_copy_protection_drive());
	/*
	 * Parse the screen environment variable.  if the string starts with
	 * "bw", then we force black and white mode.  If it ends with "fast"
	 * then we disable retrace checking
	 * @ skip_retrace_check is deprecated, so "fast" is useless now
	 */
	if (strncmp(screen_option, "bw", 2) == 0)
		monochrome_requested = TRUE;
#ifdef ROGUE_DOS_CURSES
	int sl;
	if ((sl = strlen(screen_option)) >= 4
	  && strncmp(&screen_option[sl - 4], "fast", 4) == 0)
		skip_retrace_check = TRUE;
#endif
	initial_random_seed = 0;
#ifdef ENABLE_COPY_PROTECTION_CHECKS
	while (--argc && disk_authentication_marker == 0xD0D) {
#else
	while (--argc) {
#endif
		argument = *(++argv);
		if (*argument == '-' || *argument == '/')
		{
			switch(argument[1])
			{
				case 'R': case 'r':
					 saved_game_path = save_filename;
					 break;
				case 's': case 'S':
					initialize_screen();
					score_disabled = TRUE;
					screen_updates_suspended = TRUE;
					update_high_scores(0,0,0);
					fatal("");
					break;
#ifdef LOGFILE
				case 'l':
					log_write = -1;
					initial_random_seed = 100;
					break;
				case 'k':
					log_read = -1;
					initial_random_seed = 100;
					break;
#endif //LOGFILE
			}
		}
		else if (saved_game_path == 0)
			saved_game_path = argument;
	}
	if (saved_game_path == 0) {
		saved_game_path = 0;
		initialize_screen();
		show_credits();
		if (initial_random_seed == 0)
			initial_random_seed = random_seed_from_clock();
		random_state = initial_random_seed;


		init_player();			/* Set up initial player stats */
		initialize_item_probabilities();			/* Set up probabilities of things */
		initialize_scroll_titles();			/* Set up names of scrolls */
		initialize_potion_colors();			/* Set up colors of potions */
		initialize_ring_gemstones();			/* Set up stone settings of rings */
		initialize_wand_materials();			/* Set up materials of wands */
		setup_game_io();
		drop_curtain();
		generate_level();			/* Draw current level */
		/*
		 * Start up daemons and fuses
		 */
		schedule_recurring_action(regenerate_health);
		schedule_delayed_action(start_wander_checks, WANDERTIME);
		schedule_recurring_action(consume_food);
		schedule_recurring_action(move_monsters);
		show_message("Hello %s%s.", player_name, verbose_text(".  Welcome to the Dungeons of Doom"));
		raise_curtain();
	}
	run_game(saved_game_path);
	return 0;
}

/*
 * exit_game_message:
 *	Exit the program abnormally.
 */
void
exit_game_message()
{
	fatal("Ok, if you want to exit that badly, I'll have to allow it\n");
}

#define RN		(((random_state = random_state*11109L+13849L) >> 16) & 0xffff)  //@ unused

//@ no need to declare in rogue.h
/*
 * Random number generator -
 * adapted from the FORTRAN version
 * in "Software Manual for the Elementary Functions"
 * by W.J. Cody, Jr and William Waite.
 */
long
next_random_value()
{
	random_state *= 125;
	random_state -= (random_state/2796203) * 2796203;
	return random_state;
}

/*
 * random_below:
 *	Pick a very random number.
 */
int
random_below(range)
	/* DOS callers used 16-bit ints; the native range uses the host int width. */
	register int range;
{
	return range < 1 ? 0 : ((next_random_value() + next_random_value())&0x7fffffffl) % range;
}

/*
 * roll_dice:
 *	Roll a number of dice
 */
int
roll_dice(number, sides)
	register int number, sides;
{
	register int dice_total = 0;

	while (number--)
	dice_total += random_below(sides)+1;
	return dice_total;
}

/*
 * run_game:
 *	The main loop of the program.  Loop until the game is over,
 *	refreshing things and looking at the proper times.
 */
void
run_game(saved_game_path)
	char *saved_game_path;
{
	if (saved_game_path) {
		restore_game(saved_game_path);
		setup_game_io();
#ifdef ROGUE_DOS_CURSES
		iscuron = TRUE;  //@ force the following set_cursor_visible() call to turn it off
#endif
		set_cursor_visible(FALSE);
	} else {
		previous_player_position.x = player_position.x;
		previous_player_position.y = player_position.y;
		previous_player_room = room_at(&player_position);
	}
#ifdef ME
	is_me = (strcmp(ME, player_name) == 0 || strcmp("Mr. Mctesq", player_name) == 0);
#endif
	while (playing)
		process_turn();			/* Command execution */
	exit_game_message();
}

/*
 * quit:
 *	Have player make certain, then exit.
 */
void
quit()
{
	int saved_y, saved_x;
	register byte answer;
	static bool quit_in_progress = FALSE;

	/*
	 * if they try to interupt with a control C while in
	 * this routine blow them away!
	 */
	if (quit_in_progress == TRUE)
		leave();
	quit_in_progress = TRUE;
	message_column = 0;
	getyx(ignored_window,saved_y, saved_x);  //@ Rogue devs cursing curses!
	move(0,0);
	clrtoeol();
	move(0,0);
	if (!terse)
		addstr("Do you wish to ");
	print_highlighted_text("end your quest now (%Yes/%No) ?");
	update_player_view(FALSE);
	answer = read_game_key();
	if (answer == 'y' || answer == 'Y') {
#ifdef DEMO
		demo(1);
#else
		clear();
		move(0,0);
		printw("You quit with %u gold pieces\n", player_gold);
		update_high_scores(player_gold, 1, 0);
		fatal("");
	} else {
		move(0, 0);
		clrtoeol();
		update_status_line();
		move(saved_y, saved_x);
		message_column = 0;
		command_repeat_count = 0;
#endif //DEMO
	}
	quit_in_progress = FALSE;
}

/*
 * leave:
 *	Leave quickly, but courteously
 */
void
leave()
{
	update_player_view(FALSE);
	move(LINES - 1, 0);
	clrtoeol();
	move(LINES - 2, 0);
	clrtoeol();
	move(LINES - 2, 0);
	fatal("Ok, if you want to leave that badly\n");
}
