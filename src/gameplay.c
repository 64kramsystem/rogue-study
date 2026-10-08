/*
 * All sorts of miscellaneous routines
 *
 * misc.c	1.4		(A.I. Design)	12/14/84
 */

#include "rogue.h"
#include "screen.h"

/*
 * trap_name:
 *	Print the name of a trap
 */
char *
trap_name(byte type)
{
	switch (type)
	{
	case TRAP_TRAPDOOR:
		return "a trapdoor";
	case TRAP_BEAR:
		return "a beartrap";
	case TRAP_SLEEP_GAS:
		return "a sleeping gas trap";
	case TRAP_ARROW:
		return "an arrow trap";
	case TRAP_TELEPORT:
		return "a teleport trap";
	case TRAP_DART:
		return "a poison dart trap";
	}
	show_message("wierd trap: %d", type);
	return NULL;
}

/*
 * update_player_view:
 *	A quick glance all around the player
 */
void
update_player_view(bool wakeup)
{
	register int x, y;
	register byte character, previous_symbol;
	register int index;
	register Entity *monster;
	register struct room *room;
	register int neighbor_y_end, neighbor_x_end;
	register int adjacent_passages = 0;
	register byte current_flags, *cell_flag_ptr;
	register int neighbor_y_start, neighbor_x_start, player_diagonal_sum = 0, player_diagonal_difference = 0;

	room = player_room;
	index = map_index(player_position.y, player_position.x);
	current_flags = cell_flags[index];
	previous_symbol = terrain_map[index];
	/*
	 * if the hero has moved
	 */
	if (!positions_equal(previous_player_position, player_position)) {
		if (!has_actor_flag(player,ACTOR_BLIND)) {
			for (x = previous_player_position.x - 1; x <= (previous_player_position.x + 1); x++)
				for (y = previous_player_position.y - 1; y <= (previous_player_position.y + 1); y++) {
					if ((y == player_position.y && x == player_position.x) || outside_dungeon(y,x))
						continue;
					move(y,x);
					character = inch();
					if (character == FLOOR) {
						if ((previous_player_room->flags & (ROOM_ABSENT|ROOM_DARK)) == ROOM_DARK)
							addch(' ');
					} else {
						cell_flag_ptr = &cell_flags[map_index(y,x)];
						/*
						 * if the maze or passage (that the hero is in!!)
						 * needs to be redrawn (passages once draw always
						 * stay on) do it now.
						 */
						if (((*cell_flag_ptr&CELL_MAZE) || (*cell_flag_ptr&CELL_PASSAGE)) && (character!=PASSAGE)
							&& (character != STAIRS) &&
							((*cell_flag_ptr & PASSAGE_NUMBER_MASK) == (current_flags & PASSAGE_NUMBER_MASK)) )
								addch(PASSAGE);
					}
				}
		}
		previous_player_position = player_position;
		previous_player_room = room;
	}
	neighbor_y_end = player_position.y + 1;
	neighbor_x_end = player_position.x + 1;
	neighbor_x_start = player_position.x - 1;
	neighbor_y_start = player_position.y - 1;
	if (door_stop && !first_run_step && running) {
		player_diagonal_sum = player_position.y + player_position.x;
		player_diagonal_difference = player_position.y - player_position.x;
	}
	for (y = neighbor_y_start; y <= neighbor_y_end; y++)
		if (y > 0 && y < dungeon_bottom_row) for (x = neighbor_x_start; x <= neighbor_x_end; x++) {
			if (x <= 0 || x >= COLS)
				continue;
			if (!has_actor_flag(player, ACTOR_BLIND)) {
				if (y == player_position.y && x == player_position.x)
					continue;
			} else if (y != player_position.y || x != player_position.x)
				continue;

			index = map_index(y, x);
			/*
			 * THIS REPLICATES THE monster_at() MACRO.  IF MOAT IS CHANGED,
			 * THIS MUST BE CHANGED ALSO ?? What does this really mean ??
			 */
			cell_flag_ptr = &cell_flags[index];
			character = terrain_map[index];
			/*
			 * No Doors
			 */
			if (previous_symbol != DOOR && character != DOOR) {
				/*
				 * Either hero or other in a passage
				 */
				if ((current_flags & CELL_PASSAGE) != (*cell_flag_ptr & CELL_PASSAGE)) {
					/*
					 * Neither is in a maze
					 */
					if ( ! (current_flags & CELL_MAZE) && ! (*cell_flag_ptr & CELL_MAZE))
						continue;
				}
				/*
				 * Not in same passage
				 */
				else if ((*cell_flag_ptr & CELL_PASSAGE) && (*cell_flag_ptr & PASSAGE_NUMBER_MASK) != (current_flags & PASSAGE_NUMBER_MASK))
					continue;
			}

			if ((monster = monster_at(y,x)) != NULL) {
				if (has_actor_flag(player, ACTOR_DETECTS_MONSTERS) && has_actor_flag(*monster, ACTOR_INVISIBLE)) {
					if (door_stop && !first_run_step)
						running = FALSE;
					continue;
				} else {
					if (wakeup)
						wake_monster(y, x);
					if (monster->actor_previous_tile != ' ' ||
						(!(room->flags & ROOM_DARK) && !has_actor_flag(player, ACTOR_BLIND)))
							monster->actor_previous_tile = terrain_map[index];
					if (player_can_see_monster(monster))
						character = monster->actor_disguise;
				}
			}

			if ((character!=PASSAGE) && (*cell_flag_ptr & (CELL_PASSAGE | CELL_MAZE)))
				/*
				 * The current character used for IBM ARMOR doesn't
				 * look right in Inverse
				 */
				if (character != ARMOR)
					standout();

			move(y, x);
			addch(character);
			standend();

			if (door_stop && !first_run_step && running) {
				switch (run_direction) {
				case 'h':
					if (x == neighbor_x_end)
						continue;
					break;
				case 'j':
					if (y == neighbor_y_start)
						continue;
					break;
				case 'k':
					if (y == neighbor_y_end)
						continue;
					break;
				case 'l':
					if (x == neighbor_x_start)
						continue;
					break;
				case 'y':
					if ((y + x) - player_diagonal_sum >= 1)
						continue;
					break;
				case 'u':
					if ((y - x) - player_diagonal_difference >= 1)
						continue;
					break;
				case 'n':
					if ((y + x) - player_diagonal_sum <= -1)
						continue;
					break;
				case 'b':
					if ((y - x) - player_diagonal_difference <= -1)
						continue;
					break;
				}
				switch (character) {
				case DOOR:
					if (x == player_position.x || y == player_position.y)
						running = FALSE;
					break;
				case PASSAGE:
					if (x == player_position.x || y == player_position.y)
						adjacent_passages++;
					break;
				case FLOOR:
				case VWALL:
				case HWALL:
				case ULWALL:
				case URWALL:
				case LLWALL:
				case LRWALL:
				case ' ':
					break;
				default:
					running = FALSE;
					break;
				}
			}
		}
	if (door_stop && !first_run_step && adjacent_passages > 1)
		running = FALSE;
	move(player_position.y, player_position.x);
	/*
	 * trigger_trap sets trap_display_state to 1; its teleport case increments it to 2.
	 * The > TRUE test therefore selects teleport traps. update_player_view uses that state
	 * to select the player attribute, sounds a beep for either trap state, then clears it.
	 * The original bool typedef was unsigned char, so converting this field to C bool
	 * would lose the teleport state.
	 */
	if ((cell_flags_at(player_position.y,player_position.x) & CELL_PASSAGE) || (trap_display_state > TRUE)
					|| (cell_flags_at(player_position.y,player_position.x) & CELL_MAZE))
		standout();
	addch(PLAYER);
	standend();
	if (trap_display_state) {
		beep();
		trap_display_state = FALSE;
	}
}

/*
 * item_at:
 *	Find the unclaimed object at y, x
 */
Entity *
item_at(register int y, register int x)
{
	register Entity *other_item;

	for (other_item = level_items; other_item != NULL; other_item = next(other_item))
		if (other_item->item_position.y == y && other_item->item_position.x == x)
			return other_item;
#ifdef DEBUG
	debug(sprintf(description_buffer, "Non-object %c %d,%d", terrain_at(y, x), y, x));
	return NULL;
#else
	/* NOTREACHED */
#endif
	return NULL;
}

/*
	 * eat_food:
	 *	She wants to eat something, so let her try
	 */
void
eat_food(void)
{
	register Entity *item;

	if ((item = select_inventory_item("eat", FOOD)) == NULL)
		return;
	if (item->item_category != FOOD)
	{
		show_message("ugh, you would get ill if you ate that");
		return;
	}
	inventory_count--;
	if (--item->item_quantity < 1)
	{
		detach(player_inventory, item);
		release_entity(item);
	}
	if (food_remaining < 0)
		food_remaining = 0;
	if (food_remaining > (STOMACHSIZE - 20))
		incapacitated_turns += 2 + random_below(5);
	if ((food_remaining += HUNGERTIME - 200 + random_below(400)) > STOMACHSIZE)
		food_remaining = STOMACHSIZE;
	hunger_state = 0;
	if (item == equipped_weapon)
		equipped_weapon = NULL;
	if (item->item_subtype == 1)
		show_message("my, that was a yummy %s", favorite_fruit);
	else
		if (random_below(100) > 70)
		{
			player_stats.experience++;
			show_message("yuk, this food tastes awful");
			check_experience_level();
		}
		else
			show_message("yum, that tasted good");
	if (incapacitated_turns)
		show_message("You feel bloated and fall asleep");
}

/*
 * change_player_strength:
 *	Used to modify the player's strength.  It keeps track of the
 *	highest it has been, just in case
 */
void
change_player_strength(register int adjustment)
{
	Strength previous_strength;

	if (adjustment == 0)
	return;
	adjust_strength(&player_stats.strength, adjustment);
	previous_strength = player_stats.strength;
	if (hand_has_ring(LEFT, RING_ADD_STRENGTH))
		adjust_strength(&previous_strength, -equipped_rings[LEFT]->item_modifier);
	if (hand_has_ring(RIGHT, RING_ADD_STRENGTH))
		adjust_strength(&previous_strength, -equipped_rings[RIGHT]->item_modifier);
	if (previous_strength > maximum_player_stats.strength)
		maximum_player_stats.strength = previous_strength;
}

/*
 * adjust_strength:
 *	Perform the actual add, checking upper and lower bound
 */
void
adjust_strength(register Strength *strength, int adjustment)
{
	if ((*strength += adjustment) < 3)
		*strength = 3;
	else if (*strength > 31)
		*strength = 31;
}

/*
 * add_haste:
 *	Add a haste to the player
 */
bool
add_haste(bool potion)
{
	if (has_actor_flag(player, ACTOR_HASTED))
	{
		incapacitated_turns += random_below(8);
		player.actor_flags &= ~ACTOR_CHASING;
		cancel_delayed_action(end_haste);
		player.actor_flags &= ~ACTOR_HASTED;
		show_message("you faint from exhaustion");
		return FALSE;
	}
	else
	{
		player.actor_flags |= ACTOR_HASTED;
		if (potion)
			schedule_delayed_action(end_haste, random_below(4)+10);
		return TRUE;
	}
}

/*
 * aggravate_monsters:
 *	Aggravate all the monsters on this level
 */
void
aggravate_monsters(void)
{
	register Entity *monster;

	for (monster = level_monsters; monster != NULL; monster = next(monster))
		start_monster_chase(&monster->actor_position);
}

/*
 * article_suffix:
 *      For printfs: if string starts with a vowel, return "n" for an
 *	"an".
 */
char *
article_suffix(register char *text)
{
	switch (*text)
	{
	case 'a': case 'A':
	case 'e': case 'E':
	case 'i': case 'I':
	case 'o': case 'O':
	case 'u': case 'U':
		return "n";
	default:
		return "";
	}
}

/*
 * is_equipped:
 *	See if the object is one of the currently used items
 */
bool
is_equipped(register Entity *item)
{
	if (item == NULL)
		return FALSE;
	if (item == equipped_armor || item == equipped_weapon || item == equipped_rings[LEFT]
		|| item == equipped_rings[RIGHT]) {
		show_message("That's already in use");
		return TRUE;
	}
	return FALSE;
}

/*
 * read_direction:
 *      Set up the direction co_ordinate for use in varios "prefix"
 *	commands
 */
bool
read_direction(void)
{
	register int character;

	if (repeating_command)
		return TRUE;
	show_message("which direction? ");
	do
		if ((character = read_game_key()) == ESCAPE) {
			show_message("");
			return FALSE;
		}
	while (decode_direction(character, &action_direction) == 0);
	show_message("");
	if (has_actor_flag(player, ACTOR_CONFUSED) && random_below(5) == 0)
		do {
			action_direction.y = random_below(3) - 1;
			action_direction.x = random_below(3) - 1;
		} while (action_direction.y == 0 && action_direction.x == 0);
	return TRUE;
}

bool
decode_direction(byte character, Position *direction)
{
	bool gotit;

	gotit = TRUE;
	switch (character) {
	case 'h':
	case 'H':
		direction->y = 0;
		direction->x = -1;
		break;
	case 'j':
	case 'J':
		direction->y = 1;
		direction->x = 0;
		break;
	case 'k':
	case 'K':
		direction->y = -1;
		direction->x = 0;
		break;
	case 'l':
	case 'L':
		direction->y = 0;
		direction->x = 1;
		break;
	case 'y':
	case 'Y':
		direction->y = -1;
		direction->x = -1;
		break;
	case 'u':
	case 'U':
		direction->y = -1;
		direction->x = 1;
		break;
	case 'b':
	case 'B':
		direction->y = 1;
		direction->x = -1;
		break;
	case 'n':
	case 'N':
		direction->y = 1;
		direction->x = 1;
		break;
	default:
		gotit = FALSE;
	}
	return gotit;
}

/*
 * sign:
 *	Return the sign of the number
 */
shint
sign(register int value)
{
	if (value < 0)
		return -1;
	else
		return (value > 0);
}

/*
 * randomize_duration:
 *	Give a spread around a given number (+/- 10%)
 */
int
randomize_duration(register int base_duration)
{
	return base_duration - base_duration / 10 + random_below(base_duration / 5);
}

/*
 * prompt_item_label:
 *	Call an object something after use.
 */
void
prompt_item_label(bool identified, char **label)
{
	if (identified && **label)
		**label = '\0';
	else if (!identified && **label == '\0') {
		show_message("%scall it? ",verbose_text("what do you want to "));
		read_line(description_buffer,MAXNAME);
		if (*description_buffer != ESCAPE)
			strcpy(*label, description_buffer);
		show_message("");
	}
}

/*
 * is_walkable_symbol:
 *	Returns true if it is ok to step on ch
 */
bool
is_walkable_symbol(byte character)
{
	switch (character)
	{
	case ' ':
	case VWALL:
	case HWALL:
	case ULWALL:
	case URWALL:
	case LLWALL:
	case LRWALL:
		return FALSE;
	default:
		return (!is_monster_symbol(character));
	}
}

/*
 * display_item_symbol:
 *	Decide how good an object is and return the correct character for
 * printing.
 */
char
display_item_symbol(register Entity *item)
{
	register char display_symbol = MAGIC;

	if (item->item_flags & ITEM_CURSED)
		display_symbol = BMAGIC;
	switch (item->item_category) {
	case ARMOR:
		if (item->item_modifier > armor_classes[item->item_subtype])
			display_symbol = BMAGIC;
		break;
	case WEAPON:
		if (item->item_hit_bonus < 0 || item->item_damage_bonus < 0)
			display_symbol = BMAGIC;
		break;
	case SCROLL:
		switch (item->item_subtype) {
		case SCROLL_SLEEP:
		case SCROLL_CREATE_MONSTER:
		case SCROLL_AGGRAVATE_MONSTERS:
			display_symbol = BMAGIC;
			break;
		}
		break;
	case POTION:
		switch (item->item_subtype) {
		case POTION_CONFUSION:
		case POTION_PARALYSIS:
		case POTION_POISON:
		case POTION_BLINDNESS:
			display_symbol = BMAGIC;
			break;
		}
		break;
	case STICK:
		switch (item->item_subtype) {
		case WAND_HASTE_MONSTER:
		case WAND_TELEPORT_TO:
			display_symbol = BMAGIC;
			break;
		}
		break;
	case RING:
		switch (item->item_subtype) {
		case RING_PROTECTION:
		case RING_ADD_STRENGTH:
		case RING_DAMAGE:
		case RING_DEXTERITY:
			if (item->item_modifier < 0)
				display_symbol = BMAGIC;
			break;
		case RING_AGGRAVATION:
		case RING_TELEPORTATION:
			display_symbol = BMAGIC;
			break;
		}
		break;
	}
	return display_symbol;
}

/*
 * show_help: prints out help screens
 */
void
show_help(struct help_entry *entries)
{
#ifdef HELP
	register int entry_index = 0;
	register int row, column;
	int page_full;
	byte answer = 0;

	save_screen();
	while (*entries->description && answer != ESCAPE)
	{
		page_full = FALSE;
		if ((entry_index % (terse?23:46)) == 0)
			clear();
		/*
		 * determine row and column
		 */
		column = 0;
		if (terse)
		{
			row = entry_index % 23;
			if (row == 22)
				page_full = TRUE;
		}
		else
		{
			row = (entry_index % 46) / 2;
			if (entry_index % 2)
				column = 40;
			if (row == 22 && column == 40)
				 page_full = TRUE;
		}

		move (row,column);

		addstr((char *)entries->symbol_text);
		addstr(entries->description);
		entries++;

		/*
		 * decide if we need print a continue type message
		 */
		if ( (*entries->description == 0) || page_full)
		{
			if (*entries->description == 0)
				mvaddstr (24,0,"--press space to continue--");
			else if (terse)
				mvaddstr (24,0,"--Space for more, Esc to continue--");
			else
				mvaddstr (24,0,"--Press space for more, Esc to continue--");
			do
				answer = read_game_key();
			while (answer != ' ' && answer != ESCAPE) ;
		}
		entry_index++;
	}
	restore_screen();
#endif //HELP
}

#ifndef UNIX

int
distance_squared(int y1, int x1, int y2, int x2)
{
	register int dx, dy;

	dx = (x1 - x2);
	dy = (y1 - y2);
	return dx * dx + dy * dy;
}

bool
compare_positions(Position *a, Position *b)
{
	return(a->x == b->x && a->y == b->y);
}

int
map_index(int y, int x)
{
#ifdef DEBUG
	if (outside_dungeon(y,x) && me())
		fatal("BAD INDEX");
#endif //DEBUG
	return((x * (dungeon_bottom_row-1)) + y - 1);
}

bool
outside_dungeon(int y, int x)
{
	return (y < 1 || y >= dungeon_bottom_row || x < 0 || x >= COLS) ;
}

byte
visible_entity_at(int y, int x)
{
	return(monster_at(y,x) != NULL ? monster_at(y,x)->actor_disguise : terrain_at(y,x));
}
#endif

/*
 * search:
 *	Player gropes about him to find hidden things.
 */
void
search(void)
{
	register int y, x;
	register byte *flags_cursor;
	register int ey, ex;

	if (has_actor_flag(player, ACTOR_BLIND))
		return;
	ey = player_position.y + 1;
	ex = player_position.x + 1;
	for (y = player_position.y - 1; y <= ey; y++)
		for (x = player_position.x - 1; x <= ex; x++)
		{
			if ((y == player_position.y && x == player_position.x) || outside_dungeon(y, x))
				continue;
			flags_cursor = &cell_flags_at(y, x);
			if (!(*flags_cursor & CELL_REVEALED))
				switch (terrain_at(y, x))
				{
					case VWALL:
					case HWALL:
					case ULWALL:
					case URWALL:
					case LLWALL:
					case LRWALL:
						if (random_below(5) != 0)
							break;
						terrain_at(y, x) = DOOR;
						*flags_cursor |= CELL_REVEALED;
						command_repeat_count = running = FALSE;
						break;
					case FLOOR:
						if (random_below(2) != 0)
							break;
						terrain_at(y, x) = TRAP;
						*flags_cursor |= CELL_REVEALED;
						command_repeat_count = running = FALSE;
						show_message("you found %s", trap_name(*flags_cursor & TRAP_TYPE_MASK));
						break;
				}
		}
}


/*
 * descend_stairs:
 *	He wants to go down a level
 */
void
descend_stairs(void)
{
	if (terrain_at(player_position.y, player_position.x) != STAIRS)
		show_message("I see no way down");
	else {
		dungeon_level++;
		generate_level();
	}
}

/*
 * ascend_stairs:
 *	He wants to go up a level
 */
void
ascend_stairs(void)
{
	if (terrain_at(player_position.y, player_position.x) == STAIRS)
		if (carrying_amulet) {
			dungeon_level--;
			if (dungeon_level == 0)
				show_victory_screen();
			generate_level();
			show_message("you feel a wrenching sensation in your gut");
		}
		else
			show_message("your way is magically blocked");
	else
		show_message("I see no way up");
}

/*
 * name_item_type:
 *	Allow a user to call a potion, scroll, or ring something
 */
void
name_item_type(void)
{
	register Entity *item;
	register char **labels, *current_label;
	register bool *identified_types;

	item = select_inventory_item("call", CALLABLE);
	/*
	 * Make certain that it is somethings that we want to wear
	 */
	if (item == NULL)
		return;
	switch (item->item_category)
	{
	case RING:
		labels = (char **)ring_labels;
		identified_types = ring_identified;
		current_label = (*labels[item->item_subtype] != '\0' ?
			labels[item->item_subtype] : ring_gemstones[item->item_subtype]);
		break;
	case POTION:
		labels = (char **)potion_labels;
		identified_types = potion_identified;
		current_label = (*labels[item->item_subtype] != '\0' ?
			labels[item->item_subtype] : potion_colors[item->item_subtype]);
		break;
	case SCROLL:
		labels = (char **)scroll_labels;
		identified_types = scroll_identified;
		current_label = (*labels[item->item_subtype] != '\0' ?
			labels[item->item_subtype] : scroll_titles[item->item_subtype].storage);
		break;
	case STICK:
		labels = (char **)wand_labels;
		identified_types = wand_identified;
		current_label = (*labels[item->item_subtype] != '\0' ?
			labels[item->item_subtype] : wand_materials[item->item_subtype]);
		break;
	default:
		show_message("you can't call that anything");
		return;
	}
	if (identified_types[item->item_subtype])
	{
		show_message("that has already been identified");
		return;
	}
	show_message("Was called \"%s\"", current_label);
	show_message("what do you want to call it? ");
	read_line(description_buffer,MAXNAME);
	if (*description_buffer && *description_buffer != ESCAPE)
		strcpy(labels[item->item_subtype], description_buffer);
	show_message("");
}

/*
 * prompt player for definition of macro
 */
void
edit_keyboard_macro(char *buffer, int capacity)
{
	register char *buffer_cursor = description_buffer;

	show_message("F9 was %s, enter new macro: ",buffer);
	if (read_line(description_buffer,capacity-1) != ESCAPE)
		do {
			if (*buffer_cursor != CTRL('F'))
				*buffer++ = *buffer_cursor;
		} while (*buffer_cursor++) ;
	show_message("");
	clear_macro_input();
}

#ifdef ME
bool
me(void)
{
	return is_me;
}
#endif //ME


#ifdef TEST
bool
istest(void)
{
	return (!strcmp("debug",favorite_fruit));
}
#endif //TEST
