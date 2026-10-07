/*
 * Hero movement commands
 *
 * move.c	1.4 (A.I. Design)	12/22/84
 */

#include "rogue.h"
#include "curses.h"

/*
 * Used to hold the new hero position
 */
static Position next_player_position;

static byte	trigger_trap(Position *trap_position);

/*
 * start_player_run:
 *	Start the hero running
 */
void
start_player_run(byte character)
{
	running = TRUE;
	turn_consumed = FALSE;
	run_direction = character;
}

/*
 * move_player:
 *	Check to see that a move is legal.  If it is handle the
 * consequences (fighting, picking up, etc.)
 */
void
move_player(dy, dx)
	int dy, dx;
{
	register byte character;
	register int destination_flags;

	first_run_step = FALSE;
	if (pending_trapdoor_fall) {
		pending_trapdoor_fall = FALSE;
		show_message("the crack widens ... ");
		fall_to_next_level("");
		return ;
	}
	if (immobile_turns) {
		immobile_turns--;
		show_message("you are still stuck in the bear trap");
		return;
	}
	/*
	 * Do a confused move (maybe)
	 */
	if (has_actor_flag(player, ACTOR_CONFUSED) && random_below(5) != 0)
		random_move(&player,&next_player_position);
	else {
over:
		next_player_position.y = player_position.y + dy;
		next_player_position.x = player_position.x + dx;
	}

	/*
	 * Check if he tried to move off the screen or make an illegal
	 * diagonal move, and stop him if he did.
	 * fudge it for 40/80 jll -- 2/7/84
	 */
	if (outside_dungeon(next_player_position.y, next_player_position.x))
		goto hit_bound;
	if (!diagonal_move_allowed(&player_position, &next_player_position)) {
		turn_consumed = FALSE;
		running = FALSE;
		return;
	}
	/*
	 * If you are running and the move does
	 * not get you anywhere stop running
	 */
	if (running && positions_equal(player_position, next_player_position))
		turn_consumed = running = FALSE;
	destination_flags = cell_flags_at(next_player_position.y, next_player_position.x);
	character = visible_entity_at(next_player_position.y, next_player_position.x);
	/*
	 * When the hero is on the door do not allow him
	 * to run until he enters the room all the way
	 */
	if ((terrain_at(player_position.y,player_position.x) == DOOR) && (character == FLOOR))
		running = FALSE;
	if (!(destination_flags & CELL_REVEALED) && character == FLOOR) {
		terrain_at(next_player_position.y, next_player_position.x) = character = TRAP;
		cell_flags_at(next_player_position.y, next_player_position.x) |= CELL_REVEALED;
	}
	else if (has_actor_flag(player, ACTOR_HELD) && character != 'F') {
		show_message("you are being held");
		return;
	}
	switch (character) {
	case ' ':
	case VWALL:
	case HWALL:
	case ULWALL:
	case URWALL:
	case LLWALL:
	case LRWALL:
hit_bound:
		if (running && is_passage_room(player_room) && !has_actor_flag(player, ACTOR_BLIND)) {
			register bool	can_turn_negative, can_turn_positive;

			switch (run_direction)
			{
			case 'h':
			case 'l':
				can_turn_negative = (player_position.y > 1 &&
					((cell_flags_at(player_position.y - 1, player_position.x) & CELL_PASSAGE) ||
					  terrain_at(player_position.y - 1, player_position.x) == DOOR));
				can_turn_positive = (player_position.y < dungeon_bottom_row - 1 &&
					((cell_flags_at(player_position.y + 1, player_position.x) & CELL_PASSAGE) ||
					  terrain_at(player_position.y + 1, player_position.x) == DOOR));
				if (!(can_turn_negative ^ can_turn_positive))
					break;
				if (can_turn_negative) {
					run_direction = 'k';
					dy = -1;
				} else {
					run_direction = 'j';
					dy = 1;
				}
				dx = 0;
				goto over;
			case 'j':
			case 'k':
				can_turn_negative = (player_position.x > 1 &&
					((cell_flags_at(player_position.y, player_position.x - 1) & CELL_PASSAGE)
					|| terrain_at(player_position.y, player_position.x - 1) == DOOR));
				can_turn_positive = (player_position.x < COLS-2 &&
					((cell_flags_at(player_position.y, player_position.x + 1) & CELL_PASSAGE)
					|| terrain_at(player_position.y, player_position.x + 1) == DOOR));
				if (!(can_turn_negative ^ can_turn_positive))
					break;
				if (can_turn_negative) {
					run_direction = 'h';
					dx = -1;
				} else {
					run_direction = 'l';
					dx = 1;
				}
				dy = 0;
				goto over;
			}
		}
		turn_consumed = running = FALSE;
		break;
	case DOOR:
		running = FALSE;
		if (cell_flags_at(player_position.y, player_position.x) & CELL_PASSAGE)
			enter_room(&next_player_position);
		goto move_stuff;
	case TRAP:
		character = trigger_trap(&next_player_position);
		if (character == TRAP_TRAPDOOR || character == TRAP_TELEPORT)
			return;
		/* fallthrough */
	case PASSAGE:
		goto move_stuff;
	case FLOOR:
		if (!(destination_flags & CELL_REVEALED))
			trigger_trap(&player_position);
		goto move_stuff;
	default:
		running = FALSE;
		if (is_monster_symbol(character) || monster_at(next_player_position.y, next_player_position.x))
			player_attack(&next_player_position, character, equipped_weapon, FALSE);
		else {
			running = FALSE;
			if (character != STAIRS)
				pickup_symbol = character;
move_stuff:
			mvaddch(player_position.y, player_position.x, terrain_at(player_position.y, player_position.x));
			if ((destination_flags & CELL_PASSAGE) && (terrain_at(previous_player_position.y, previous_player_position.x) == DOOR
					|| (cell_flags_at(previous_player_position.y, previous_player_position.x) & CELL_MAZE)))
				leave_room(&next_player_position);
			if ((destination_flags & CELL_MAZE) && (cell_flags_at(previous_player_position.y, previous_player_position.x) & CELL_MAZE) == 0)
				enter_room(&next_player_position);
			copy_value(player_position,next_player_position);
		}
		break;
	}
}

/*
		 * wake_room_monsters:
		 *	Called to illuminate a room.  If it is dark, remove anything
		 *	that might move.
		 */
void
wake_room_monsters(room)
	struct room *room;
{
	register int y, x;
	register byte character;
	register Entity *monster;

	if (!(room->flags & ROOM_ABSENT) && !has_actor_flag(player, ACTOR_BLIND))
		for (y = room->origin.y; y < room->origin.y + room->size.y; y++)
			for (x = room->origin.x; x < room->origin.x + room->size.x; x++) {
				character = visible_entity_at(y, x);
				if (is_monster_symbol(character)) {
					monster = wake_monster(y, x);
					//@ this sanity check was not in original
					if (monster == NULL)
					{
						continue;
					}
					if (monster->actor_previous_tile == ' ' && !(room->flags & ROOM_DARK)
						&& !has_actor_flag(player, ACTOR_BLIND))
							monster->actor_previous_tile = terrain_at(y, x);
				}
			}
}

/*
 * trigger_trap:
 *	The guy stepped on a trap.... Make him pay.
 */
static
byte
trigger_trap(Position *trap_position)
{
	register byte trap_type;
	register int index;

	command_repeat_count = running = FALSE;
	index = map_index(trap_position->y, trap_position->x);
	terrain_map[index] = TRAP;
	trap_type = cell_flags[index] & TRAP_TYPE_MASK;
	trap_display_state = TRUE;
	switch (trap_type) {
	when TRAP_TRAPDOOR:
		fall_to_next_level("you fell into a trap!");
	when TRAP_BEAR:
		immobile_turns += BEARTIME;
		show_message("you are caught in a bear trap");
	when TRAP_SLEEP_GAS:
		incapacitated_turns += SLEEPTIME;
		player.actor_flags &= ~ACTOR_CHASING;
		show_message("a %smist envelops you and you fall asleep",
			verbose_text("strange white "));
	when TRAP_ARROW:
		if (attack_hits(player_stats.experience_level-1, player_stats.armor_class, 1)) {
			player_stats.hit_points -= roll_dice(1, 6);
			if (player_stats.hit_points <= 0) {
				show_message("an arrow killed you");
				show_death_screen('a');
			} else
				show_message("oh no! An arrow shot you");
		}
		else {
			Entity *arrow;

			if ((arrow = allocate_entity()) != NULL) {
				arrow->item_category = WEAPON;
				arrow->item_subtype = ARROW;
				init_weapon(arrow, ARROW);
				arrow->item_quantity = 1;
				copy_value(arrow->item_position,player_position);
				drop_projectile(arrow, FALSE);
			}
			show_message("an arrow shoots past you");
		}
	when TRAP_TELEPORT:
		teleport();
		mvaddch(trap_position->y, trap_position->x, TRAP); /* since the hero's leaving, update_player_view()
						won't put it on for us */
		/*
		 * trigger_trap initialized trap_display_state to 1. Incrementing it marks a teleport
		 * trap with state 2, which update_player_view tests with > TRUE before clearing it.
		 */
		trap_display_state++;
	when TRAP_DART:
		if (attack_hits(player_stats.experience_level+1, player_stats.armor_class, 1)) {
			player_stats.hit_points -= roll_dice(1, 4);
			if (player_stats.hit_points <= 0) {
				show_message("a poisoned dart killed you");
				show_death_screen('d');
			}
			if (!wearing_ring(RING_SUSTAIN_STRENGTH) && !player_saving_throw(VS_POISON))
				change_player_strength(-1);
			show_message("a dart just hit you in the shoulder");
		} else
			show_message("a dart whizzes by your ear and vanishes");
		break;
	}
	clear_macro_input();
	return trap_type;
}

void
fall_to_next_level(message_text)
	char *message_text;
{
	dungeon_level++;
	if (*message_text == 0)
		show_message(" ");
	generate_level();
	show_message("");
	show_message(message_text);
	if (!player_saving_throw(VS_LUCK)) {
		show_message("you are damaged by the fall");
		if ((player_stats.hit_points -= roll_dice(1,8)) <= 0)
			show_death_screen('f');
	}
}

/*
 * random_move:
 *	Move in a random direction if the monster/person is confused
 */
void
random_move(actor,destination)
	Entity *actor;
	Position *destination;
{
	register int x, y;
	register byte character;
	register Entity *item;

	y = destination->y = actor->actor_position.y + random_below(3) - 1;
	x = destination->x = actor->actor_position.x + random_below(3) - 1;
	/*
	 * Now check to see if that's a legal move.  If not, don't move.
	 * (I.e., bump into the wall or whatever)
	 */
	if (y == actor->actor_position.y && x == actor->actor_position.x)
		return;
	if ((y < 1 || y >= dungeon_bottom_row) || (x < 0 || x >= COLS))
		goto bad;
	else if (!diagonal_move_allowed(&actor->actor_position, destination))
		goto bad;
	else {
		character = visible_entity_at(y, x);
		if (!is_walkable_symbol(character))
			goto bad;
		if (character == SCROLL) {
			for (item = level_items; item != NULL; item = next(item))
				if (y == item->item_position.y && x == item->item_position.x)
					break;
			if (item != NULL && item->item_subtype == SCROLL_SCARE_MONSTER)
				goto bad;
		}
	}
	return;

bad:
	copy_value((*destination),actor->actor_position);
	return;
}
