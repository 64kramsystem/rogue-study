/*
 * Read and execute the user comamnds
 *
 * command.c	1.44	(A.I. Design)	2/14/85
 */

#include	"rogue.h"
#include "screen.h"

static int previous_repeat_count;
static byte previous_command, pickup_enabled, previous_pickup_enabled;

void
process_turn()
{
	register int remaining_actions;

	if (has_actor_flag(player, ACTOR_HASTED))
		remaining_actions = random_below(2) + 2;
	else
		remaining_actions = 1;
	while (remaining_actions--) {
		update_status_line();
#ifdef WIZARD
		if (wizard)
			score_disabled = TRUE;
#endif
		if (incapacitated_turns) {
			if (--incapacitated_turns <= 0) {
				show_message("you can move again");
				incapacitated_turns = 0;
			}
			screen_refresh();  //@ sleeping, fainted, frozen, etc
		} else
			execute_command();
		run_delayed_actions();
		run_recurring_actions();
		/*
		 * The DOS source reused the action counter for ring iteration, leaving
		 * it at RIGHT + 1 (2) and preventing the outer loop from finishing.
		 */
		for (int hand = LEFT; hand <= RIGHT; hand++)
		{
			if (equipped_rings[hand])
			{
				switch (equipped_rings[hand]->item_subtype)
				{
				when RING_SEARCHING:
					search();
				when RING_TELEPORTATION:
					if (random_below(50) == 17)
						teleport();
					break;
				}
			}
		}
	}
}

//@ No need to declare in rogue.h
byte
read_command_key()
{
	register bool run_mode_unchanged;
	register byte character;

	run_mode_unchanged = (auto_run_enabled == scroll_lock_run_enabled);
	character = read_game_key();
	if (run_mode_unchanged)
		auto_run_enabled = scroll_lock_run_enabled;
	else
		auto_run_enabled = !scroll_lock_run_enabled;
	switch (character) {
		when '\b': character = 'h';
		when '+': character = 't';
		when '-': character = 'z';
		break;
	}
	if (message_column && !running)
		show_message("");
	return character;
}

//@ No need to declare in rogue.h
/*
 * Read a command, setting thing up according to prefix like devices
 * Return the command character to be executed.
 */
byte
read_command_prefix()
{
	register int parsed_repeat_count;
	register byte command_key, character;

	turn_consumed = TRUE;
	auto_run_enabled = scroll_lock_run_enabled;
	update_player_view(TRUE); //@ draw player in updated position on every non-sleep frame
	if (!running)
		door_stop = FALSE;
	pickup_enabled = TRUE;
	repeating_command = FALSE;
	if (--command_repeat_count > 0) {
		pickup_enabled = previous_pickup_enabled;
		command_key = previous_command;
		auto_run_enabled = FALSE;
		screen_refresh();  //@ repeated commands, ie, "10s"
	} else {
		command_repeat_count = 0;
		if (running) {
			command_key = run_direction;
			pickup_enabled = previous_pickup_enabled;
			screen_refresh();  //@ running ("H", "fh", "L", etc)
		} else {
			for (command_key = 0; command_key == 0; ) {
				switch (character = read_command_key()) {
					case '0': case '1': case '2': case '3': case '4':
					case '5': case '6': case '7': case '8': case '9':
						parsed_repeat_count = command_repeat_count * 10;
						if ((parsed_repeat_count += character - '0') > 0 && parsed_repeat_count < 10000)
							command_repeat_count = parsed_repeat_count;
						show_repeat_count();
					when 'f':
						auto_run_enabled = !auto_run_enabled;
					when 'g':
						pickup_enabled = FALSE;
					when 'a':
						command_key = previous_command;
						command_repeat_count = previous_repeat_count;
						pickup_enabled = previous_pickup_enabled;
						repeating_command = TRUE;
					when ' ':	/* Spaces are ignored */
					when ESCAPE:
						door_stop = FALSE;
						command_repeat_count = 0;
						show_repeat_count();
					otherwise:
						command_key = character;
				}
			}
		}
	}
	if (command_repeat_count)
		auto_run_enabled = FALSE;
	switch (command_key) {
	case 'h': case 'j': case 'k': case 'l':
	case 'y': case 'u': case 'b': case 'n':
		if (auto_run_enabled && !running ) {
			if (!has_actor_flag(player, ACTOR_BLIND)) {
				door_stop = TRUE;
				first_run_step = TRUE;
			}
			command_key = toupper(command_key);
		}
		/* fallthrough */
	case 'H': case 'J': case 'K': case 'L':
	case 'Y': case 'U': case 'B': case 'N':
	case 'q': case 'r': case 's': case 'z':
	case 't': case '.':
#ifdef WIZARD
	case CTRL(D): case 'C':
#endif //WIZARD
		break;
	default:
		command_repeat_count = 0;
		break;
	}
	if (command_repeat_count || previous_repeat_count)
		show_repeat_count();
	previous_command = command_key;
	previous_repeat_count = command_repeat_count;
	previous_pickup_enabled = pickup_enabled;
	return command_key;
}

void
show_repeat_count()
{
	move(LINES-2, COLS-4);
	if (command_repeat_count)
		printw("%-4d", command_repeat_count);
	else
		addstr("    ");
}

void
execute_command()
{
	Position movement;
	register int character;

	do {
		switch (character = read_command_prefix()) {
		when 'h': case 'j': case 'k': case 'l':
		case 'y': case 'u': case 'b': case 'n':
			decode_direction(character, &movement);
			move_player(movement.y, movement.x);
		when 'H': case 'J': case 'K': case 'L':
		case 'Y': case 'U': case 'B': case 'N':
			start_player_run(tolower(character));
		when 't':
			if (read_direction())
				throw_item(action_direction.y, action_direction.x);
			else
				turn_consumed = FALSE;
		when 'Q': turn_consumed = FALSE; quit();
		when 'i': turn_consumed = FALSE; show_inventory(player_inventory, 0, "");
		when 'd': drop_item();
		when 'q': drink_potion();
		when 'r': read_scroll();
		when 'e': eat_food();
		when 'w': wield();
		when 'W': wear_armor();
		when 'T': remove_armor();
		when 'P': put_on_ring();
		when 'R': remove_ring();
		when 'c': turn_consumed = FALSE; name_item_type();
		when '>': turn_consumed = FALSE; descend_stairs();
		when '<': turn_consumed = FALSE; ascend_stairs();
		when '/': turn_consumed = FALSE; show_help(symbol_help);
		when '?': turn_consumed = FALSE; show_help(command_help);
		when '!': turn_consumed = FALSE; show_fake_dos();
		when 's': search();
		when 'z':
			if (read_direction())
				zap_wand();
			else
				turn_consumed = FALSE;
		when 'D': turn_consumed = FALSE; show_discoveries();
		when CTRL('T'):
			turn_consumed = FALSE;
			show_message((expert ^= 1)
				? "Ok, I'll be brief"
				: "Goodie, I can use big words again!");
		when 'F': turn_consumed = FALSE; edit_keyboard_macro(keyboard_macro, MACROSZ);
		when CTRL('F'): turn_consumed = FALSE; pending_macro_input = keyboard_macro;
		when CTRL('R'):
			turn_consumed = FALSE;
			/* The stored message is already formatted and may contain literal '%'. */
			if (*previous_message)
				show_message("%s", previous_message);
			else
				show_message("");
		when 'v':
			turn_consumed = FALSE;
			if (strcmp(player_name,"The Grand Beeking") == 0)
				append_message("(%d)",code_checksum());
			show_message("Rogue version %d.%d (Mr. Mctesq was here)", version_major, version_minor);
		when 'S': turn_consumed = FALSE; save_game();
		when '.': regenerate_health();
		when '^':
			turn_consumed = FALSE;
			if (read_direction()) {
				Position trap_position;

				trap_position.y = player_position.y + action_direction.y;
				trap_position.x = player_position.x + action_direction.x;
				if (terrain_at(trap_position.y, trap_position.x) != TRAP)
					show_message("no trap there.");
				else
					show_message("you found %s",
						trap_name(cell_flags_at(trap_position.y, trap_position.x) & TRAP_TYPE_MASK));
			}
		when 'o': turn_consumed = FALSE; show_message("i don't have any options, oh my!");
		when CTRL('L'):
			turn_consumed = FALSE;
			show_message("the screen looks fine to me (jll was here)");
#ifdef WIZARD
		when 'C': turn_consumed = FALSE; create_obj();
#endif
		otherwise:
			turn_consumed = FALSE;
			remember_message = FALSE;
			show_message("illegal command '%s'", describe_key(character));
			command_repeat_count = 0;
			remember_message = TRUE;
		}
		if (pickup_symbol && pickup_enabled)
			pick_up_item(pickup_symbol);
		pickup_symbol = 0;
		if (!running)
			door_stop = FALSE;
	} while (turn_consumed == FALSE);
}
