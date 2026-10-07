/*
 * Various input/output functions
 *
 * io.c		1.4		(A.I. Design) 12/10/84
 */

#include	"rogue.h"
#include	"curses.h"

#define AC(a) (-((a)-11))
#define PT(i,j) ((COLS==40)?i:j)
/*
 * msg:
 *	Display a message at the top of the screen.
 */
static int message_length = 0;

/* VARARGS1 */
/*@ nope, it was not vargars. But now it is */
void
message_by_verbosity(const char *tfmt, const char *format, ...)
{
	va_list arguments;
	va_start(arguments, format);

	if (expert)
		show_message_v(tfmt, arguments);
	else
		show_message_v(format, arguments);

	va_end(arguments);
}

//@ va_list variant of msg()
void
show_message_v(const char *format, va_list arguments)
{
	/*
	 * if the string is "", just clear the line
	 */
	if (*format == '\0')
	{
		move(0, 0);
		clrtoeol();
		message_column = 0;
		return;
	}
	/*
	 * otherwise add to the message and flush it out
	 */
	append_message_v(format, arguments);
	finish_message();
}

//@ varargs variant, now a wrapper for vmsg()
void
show_message(const char *format, ...)
{
	va_list arguments;
	va_start(arguments, format);

	show_message_v(format, arguments);

	va_end(arguments);
}
/* VARARGS1
 * @ now for real
 */
/*
 * addmsg:
 *	Add things to the current message
 */
void
append_message(const char *format, ...)
{
	va_list arguments;
	va_start(arguments, format);

	append_message_v(format, arguments);

	va_end(arguments);
}

/*
 * endmsg:
 *	Display a new msg (giving him a chance to see the previous one
 *	if it is up there with the -More-)
 */
void
finish_message(void)
{
	if (remember_message)
		strcpy(previous_message, message_buffer);
	if (message_column) {
		update_player_view(FALSE);
		move(0,message_column);
		show_more_prompt(" More ");
	}
	/*
	 * All messages should start with uppercase, except ones that
	 * start with a pack addressing character
	 */
	if (is_lower(message_buffer[0]) && message_buffer[1] != ')')
		message_buffer[0] = toupper(message_buffer[0]);
	display_wrapped_message(0,message_buffer);
	message_column = message_length;
	message_length = 0;
}


/*
 *  More:  tag the end of a line and wait for a space
 */
void
show_more_prompt(message_text)
	char *message_text;
{
	int x, y;
	register int i, message_size;
	char covered_text[80];
	int prompt_column = TRUE;
	int covered = FALSE;

	message_size = strlen(message_text);
	getxy(&x,&y);
	/*
	 * it is reasonable to assume that if the you are no longer
	 * on line 0, you must have wrapped.
	 */
	if (x != 0) {
		x=0;
		y=COLS;
	}
	if ((y+message_size)>COLS) {
		move(x,y=COLS-message_size);
		covered = TRUE;
	}

	for(i=0;i<message_size;i++) {
		covered_text[i] = inch();
		if ((i+y) < (COLS-2))
			move(x,y+i+1);
		covered_text[i+1] = 0;
	}

	move(x,y);
	standout();
	addstr(message_text);
	standend();

	while (read_game_key() != ' ') {
		if (covered && prompt_column) {
			move(x,y);
			addstr(covered_text);
			prompt_column = FALSE;
		}
		else if (covered)
		{
			move(x,y);
			standout();
			addstr(message_text);
			standend();
			prompt_column = TRUE;
		}
	}
	move(x,y);
	addstr(covered_text);
}


/*@
* arguments changed from fixed ints to va_list.
* no need of a varargs version as this is only used internally by io.c
* varargs-aware functions
*/
/*
 * doadd:
 *	Perform an add onto the message buffer
 */
void
append_message_v(const char *format, va_list arguments)
{

	vsnprintf(&message_buffer[message_length], BUFSIZE - message_length, format, arguments);
	message_length = strlen(message_buffer);
}

/*
 * putmsg:
 *  put a msg on the line, make sure that it will fit, if it won't
 *  scroll msg sideways until he has read it all
 */
void
display_wrapped_message(message_row,message_text)
	int message_row;
	char *message_text;
{
	register char *current_line, *previous_line=0, *next_line;
	int line_length;

	current_line = message_text;
	do {
		display_message_segment(message_row,previous_line,current_line);
		message_length = line_length = strlen(current_line);
		if (line_length > COLS) {
			show_more_prompt(" Cont ");
			previous_line = current_line;
			do {
				next_line = strpbrk(current_line," ");
				/*
				 * If there are no blanks in line
				 */
				if ((next_line==0 || next_line>=&previous_line[COLS]) && previous_line==current_line) {
					current_line = &previous_line[COLS];
					break;
				}
				if ((next_line >= (previous_line+COLS)) || ((signed)strlen(current_line) < COLS))
					break;
				current_line = next_line + 1;
			} while (1);
		}
	} while (line_length > COLS);
}

/*
 * scrlmsg:  scroll a message accross the line
 * @ renamed to avoid conflict with <curses.h>.
 * @ Purpose is completely unrelated to curses
 */
void
display_message_segment(message_row,scroll_start,scroll_end)
	int message_row;
	char *scroll_start, *scroll_end;
{
	char *format;

	if (COLS > 40)
		format = "%.80s";
	else
		format = "%.40s";

	if (scroll_start == 0) {
		move(message_row,0);
		if ((signed)strlen(scroll_end) < COLS)
			clrtoeol();
		printw(format,scroll_end);
	}
	else
		while (scroll_start <= scroll_end) {
			move(message_row,0);
			printw(format,scroll_start++);
			if ((signed)strlen(scroll_start) < (COLS-1))
				clrtoeol();
		}
}
/*
 * io_unctrl:
 *	Print a readable version of a certain character
 *	@ renamed to avoid conflict with <curses.h>
 *	@ same purpose but different behavior, so not using the curses version
 */
char *
describe_key(byte character)
{
	static char key_text[9];		/* Defined in curses library */

	if (is_space(character))
		strcpy(key_text," ");
	else if (!is_print(character))
		if (character < ' ')
			sprintf(key_text, "^%c", character + '@');
		else
			sprintf(key_text, "\\x%x",character);
	else {
		key_text[0] = character;
		key_text[1] = 0;
	}

	return key_text;
}

/*
 * status:
 *	Display the important stats line.  Keep the cursor where it was.
 */
void
update_status_line(void)
{
	int saved_y, saved_x;
	static int previous_hunger;
	static int previous_depth, previous_gold = -1, previous_hit_points, previous_armor = 0;
	static Strength previous_strength;
	static int previous_experience_level = 0;
	static char *state_name[] =
	{
		"      ", "Hungry", "Weak", "Faint","?"
	};

	update_keyboard_and_clock();

	getyx(stdscr, saved_y, saved_x);
	if (is_color)
		yellow();

	/*@
	 * Rogue used a rudimentary custom sprintf() that didn't fully support
	 * the (quite sophisticated) numeric formatting strings used on status.
	 * As <stdio.h>'s sprintf() does, formatting was simplified so the output
	 * matches the original.
	 */

	/*
	 * Level:
	 */
	if (previous_depth != dungeon_level)
	{
		previous_depth = dungeon_level;
	move(PT(22,23),0);
	printw("Level:%-4d", dungeon_level);
	}

	/*
	 * Hits:
	 */
	if (previous_hit_points != player_stats.hit_points)
	{
		previous_hit_points = player_stats.hit_points;
		move(PT(22,23),12);
		printw("Hits:%d(%d) ", player_stats.hit_points, player_max_hit_points);
		/* just in case they get wraithed with 3 digit max hits */
		if (player_stats.hit_points < 100)
			addch(' ');
	}

	/*
	 * Str:
	 */
	if (player_stats.strength != previous_strength)
	{
		previous_strength = player_stats.strength;
		move(PT(22,23),26);
		printw("Str:%d(%d) ", player_stats.strength, maximum_player_stats.strength);
	}

	/*
	 * Gold
	 */
	if(previous_gold != player_gold)
	{
		previous_gold = player_gold;
		move(23, PT(0,40));
		printw("Gold:%-5u",player_gold);
	}

	/*
	 * Armor:
	 */
	if(previous_armor != (equipped_armor != NULL ? equipped_armor->item_modifier : player_stats.armor_class))
	{
		previous_armor = (equipped_armor != NULL ? equipped_armor->item_modifier : player_stats.armor_class);
		if (hand_has_ring(LEFT,RING_PROTECTION))
			previous_armor -= equipped_rings[LEFT]->item_modifier;
		if (hand_has_ring(RIGHT,RING_PROTECTION))
			previous_armor -= equipped_rings[RIGHT]->item_modifier;
		move(23,PT(12,52));
		printw("Armor:%-2d",
		AC(equipped_armor != NULL ? equipped_armor->item_modifier : player_stats.armor_class));
	}

	/*
	 * Exp:
	 */
	if (previous_experience_level != player_stats.experience_level)
	{
		previous_experience_level = player_stats.experience_level;
		move(23, PT(22, 62));
		printw("%-12s", rank_names[previous_experience_level-1]);
	}

	/*
	 * Hungry state
	 */
	if (previous_hunger != hunger_state)
	{
		previous_hunger = hunger_state;
		move(24, PT(28,58));
		addstr(state_name[0]);
		move(24, PT(28,58));
		if (hunger_state)
		{
			bold();
			addstr(state_name[hunger_state]);
			standend();
		}
	}

	if (is_color)
		standend();

	move(saved_y, saved_x);
}

/*
 * wait_for
 *	Sit around until the guy types the right key
 */
void
wait_for_key(byte character)
{
	/*@
	 * stdio and ncurses will map all stream line endings to '\n'
	 * Hooray ANSI! :)
	 *
	register char c;

	if (ch == '\n')
		while ((c = readchar()) != '\n' && c != '\r')
			continue;
	else
	 */
	while (read_game_key() != character)
		continue;
}

/*@
 * Wait with a message until user press Enter
 * New function, used to block before leaving the game
 */
void
wait_for_enter(const char *message_text)
{
	standend();
	move(LINES-1,0);
	set_cursor_visible(TRUE);
	if (*message_text)
	{
		printw("[Press Enter to %s]", message_text);
	}
	else
	{
		printw("[Press Enter]");
	}
	clear_macro_input();
	wait_for_key('\n');
	move(LINES-1,0);
}

/*
 * show_win:
 *	Function used to display a window and wait before returning
 *	@ a window? looks like a single message to me!
 */
void
show_overlay_message(message)
	char *message;
{
	mvaddstr(0,0,message);
	move(player_position.y, player_position.x);
	wait_for_key(' ');
}


/*
 * str_attr:  format a string with attributes.
 *
 *    formats:
 *        %i - the following character is turned inverse vidio
 *        %I - All characters upto %$ or null are turned inverse vidio
 *        %u - the following character is underlined
 *        %U - All characters upto %$ or null are underlined
 *        %$ - Turn off all attributes
 *
 *     Attributes do not nest, therefore turning on an attribute while
 *     a different one is in effect simply changes the attribute.
 *
 *     "No attribute" is the default and is set on leaving this routine
 *
 *     Eventually this routine will contain colors and character intensity
 *     attributes.  And I'm not sure how I'm going to interface this with
 *     printf certainly '%' isn't a good choice of characters.  jll.
 */
void
print_highlighted_text(text)
	char *text;
{
#ifdef LUXURY
	register int is_attr_on = FALSE, was_touched = FALSE;

	while(*text)
	{
		if (was_touched == TRUE)
		{
			standend();
			is_attr_on = FALSE;
			was_touched = FALSE;
		}
	if (*text == '%')
	{
		text++;
		switch(*text)
		{
		case 'u':
					was_touched = TRUE;
				case 'U':
			uline();
					is_attr_on = TRUE;
					text++;
					break;
				case 'i':
					was_touched = TRUE;
				case 'I':
					standout();
					is_attr_on = TRUE;
					text++;
					break;
				case '$':
					if (is_attr_on)
						was_touched = TRUE;
					text++;
					continue;
			 }
		}
		if ((*text == '\n') || (*text == '\r'))
		{
			text++;
			printw("\n");
		}
		else if (*text != 0)
			addch(*text++);
	}
	if (is_attr_on)
		standend();
#else
	while (*text)
	{
		if (*text == '%') {
			text++;
			standout();
		}
		addch(*text++);
		standend();
	}
#endif //LUXURY
}

/*
 * key_state:
 */
void
update_keyboard_and_clock(void)
{
	static int keyboard_initialized = TRUE;
	static int num_lock, caps_lock;
	static int num_lock_column, caps_lock_column, clock_column;
	register int new_num_lock, new_caps_lock, new_run_mode;
	static int hour, minute;
	int clock_changed = FALSE, minutes_since_update;
	int x, y;
#ifdef DEMO
	static int tot_time = 0;
#endif //DEMO
#ifdef ROGUE_DOS_CLOCK
	static unsigned int ntick = 0;

	//@ only update every 6 ticks, ~3 times per second
	if (tick < ntick)
		return;
	ntick = tick + 6;
#else
	static long previous_clock_time = 0;
	long current_time = epoch_seconds();
#endif

	/*@
	 * Do not update between wdump()/wrestor() operations
	 * (when the user is in a non-game screen like inventory or discoveries)
	 * Or if the screen is not yet initialized.
	 */
	if (screen_updates_suspended || dos_screen_mode < 0)
		return;
#ifndef __linux__
	dos_regs->ax = 0x200;
	call_dos_interrupt(SW_KEY, dos_regs);
	new_num_lock = dos_regs->ax;
#else
	new_num_lock = keyboard_lock_flags();
#endif
	new_caps_lock = new_num_lock & 0x40;
	new_run_mode = new_num_lock & 0x10;  //@ scroll lock
	new_num_lock &= 0x20;
#ifdef ROGUE_DOS_CLOCK
	/*
	 * set up the clock the first time here
	 */
	/*@
	 * using DOS INT 21h/AH=2Ch - Get System Time
	 * CH = hour (0-23)
	 * CL = minutes (0-59)
	 */
	if (keyboard_initialized) {
		dos_regs->ax = 0x2c << 8;
		call_dos_interrupt(SW_DOS, dos_regs);
		hour = (dos_regs->cx >> 8) % 12;  //@ force 12-hour display format
		minute = dos_regs->cx & 0xFF;
		clock_changed = TRUE;
	}
	//@ 1092 ticks = 1 minute @ 18.2 ticks per second rate
	if (tick > 1092) {
		/*
		 * time os call kills jr and others we keep track of it
		 * ourselves
		 */
		minute = (minute + 1) % 60;
		if (minute == 0)
			hour = (hour + 1) % 12;
		tick = tick - 1092;
		ntick = tick + 6;
		clock_changed = TRUE;
	}
#else
	if (current_time - previous_clock_time >= 60)
	{
		LocalTime *local = current_local_time();
		hour = local->hour % 12;
		minute = local->minute;
		previous_clock_time = current_time - local->second;
		clock_changed = TRUE;
	}
#endif

	/*
	 * this is built for speed so set up once first time this
	 * is executed
	 */
	if (keyboard_initialized || status_layout_dirty)
	{
		status_layout_dirty = keyboard_initialized = FALSE;
		if (COLS == 40)
		{
			num_lock_column = 10;
			caps_lock_column = 19;
			clock_column = 35;
		}
		else
		{
			num_lock_column = 20;
			caps_lock_column = 39;
			clock_column = 75;
		}
		/*
		 * this will force all fields to be updated first time through
		 */
		num_lock = !new_num_lock;
		caps_lock = !new_caps_lock;
		clock_changed++;
		scroll_lock_run_enabled = !new_run_mode;
	}

	getxy(&x, &y);

	if (scroll_lock_run_enabled != new_run_mode)
	{

		scroll_lock_run_enabled = new_run_mode;
		command_repeat_count = 0;
		show_repeat_count();
		running = FALSE;
		move(LINES-1,0);
		if (scroll_lock_run_enabled)
		{
			bold();
			addstr("Fast Play");
			standend();
		}
		else
		{
			addstr("         ");
		}
	}

	if (num_lock != new_num_lock)
	{
		num_lock = new_num_lock;
		command_repeat_count = 0;
		show_repeat_count();
		running = FALSE;
		move(24,num_lock_column);
		if (num_lock)
		{
			bold();
			addstr("NUM LOCK");
			standend();
		}
		else
			addstr("        ");
	}
	if (caps_lock != new_caps_lock)
	{
		caps_lock = new_caps_lock;
		move(24,caps_lock_column);
		if (caps_lock)
		{
			bold();
			addstr("CAP LOCK");
			standend();
		}
		else
			addstr("        ");
	}
	if (clock_changed)
	{
		clock_changed = FALSE;
#ifdef DEMO
		/*
		 * Don't let them get by level 10 because they might do something
		 * nasty like disable the clock
		 */
		if (((tot_time++ - deepest_level) > DEMOTIME) || deepest_level > 10)
			demo(DEMOTIME);
#endif //DEMO
		/* work around the compiler buggie boos */
		minutes_since_update = minute % 10;
		move(24,clock_column);
		bold();
		printw("%2d:%1d%1d",hour?hour:12,minute/10,minutes_since_update);
		standend();
	}
	move(x, y);
}

char *
verbose_text(text)
	char *text;
{
	return( terse || expert ? empty_string : text);
}
