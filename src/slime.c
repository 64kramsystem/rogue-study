/*
 * Code for handling the various special properties of the slime
 *
 * slime.c	1.0		(A.I. Design 1.42)	1/17/85
 */

#include	"rogue.h"
#include	"curses.h"

/*
 * Slime_split:
 *	Called when it has been decided that A slime should divide itself
 */

static Position slime_spawn_position;

static bool	new_slime(Entity *slime);

void
slime_split(slime)
	Entity *slime;
{
	register Entity *offspring;

	if (!new_slime(slime) || (offspring = allocate_entity()) == NULL)
		return;
	show_message("The slime divides.  Ick!");
	new_monster(offspring, 'S', &slime_spawn_position);
	if (player_can_see_position(slime_spawn_position.y, slime_spawn_position.x)) {
		offspring->actor_previous_tile = terrain_at(slime_spawn_position.y, slime_spawn_position.x);
		mvaddch(slime_spawn_position.y, slime_spawn_position.x, 'S');
	}
	start_monster_chase(&slime_spawn_position);
}

static
bool
new_slime(slime)
	Entity *slime;
{
	register int y, x, origin_y, origin_x;
	register bool found_space;
	Entity *adjacent_monster;
	Position spawn_position;

	found_space = FALSE;
	slime->actor_flags |= ACTOR_FLIES;
	if (!find_monster_spawn_position((origin_y = slime->actor_position.y), (origin_x = slime->actor_position.x), &spawn_position)) {
		/*
		 * There were no open spaces next to this slime, look for other
		 * slimes that might have open spaces next to them.
		 */
		for (y = origin_y -1; y <= origin_y+1; y++)
			for (x = origin_x-1; x <= origin_x+1; x++)
				if (visible_entity_at(y, x) == 'S' && (adjacent_monster = monster_at(y, x))) {
					if (adjacent_monster->actor_flags & ACTOR_FLIES)
						continue;				/* Already done this one */
					if (new_slime(adjacent_monster)) {
						y = origin_y+2;
						x = origin_x +2;
					}
				}
	} else {
		found_space = TRUE;
		slime_spawn_position = spawn_position;
	}
	slime->actor_flags &= ~ACTOR_FLIES;
	return found_space;
}

/*@
 * Pick an appropriate spot around a central spot for a new monster to spawn
 * (r, c): row, col of central spot
 * cp: pointer to coordinate for the new monster, if any
 * Return FALSE if no suitable spot around (r, c) is found
 *
 * Original return value was somewhat an abuse of the bool convention,
 * used both as TRUE/FALSE and as an integer for calculating odds.
 * To avoid that, 'inv_odds' was created for the random_below() call,
 * and 'appear' is now "strictly" boolean
 */
bool
find_monster_spawn_position(origin_y, origin_x, spawn_position)
	int origin_y, origin_x;
	Position *spawn_position;
{
	register int y, x, inv_odds = 0;
	bool appear = FALSE;
	byte character;

	for (y = origin_y-1; y <= origin_y+1; y++)
		for (x = origin_x-1; x <= origin_x+1; x++) {
			/*
			 * Don't put a monster in top of the player.
			 */
			if ((y == player_position.y && x == player_position.x) || outside_dungeon(y,x))
				continue;
			/*
			 * Or anything else nasty
			 */
			if (is_walkable_symbol(character = visible_entity_at(y, x))) {
				if (character == SCROLL && item_at(y, x)->item_subtype == SCROLL_SCARE_MONSTER)
					continue;
				/*@
				 * Get first available spot with 100% chance,
				 * then randomly change to next available spot, if any,
				 * with decreasing 1-to-n odds (50%, 33%, 25%, 20%,...)
				 */
				appear = TRUE;
				if (random_below(++inv_odds) == 0) {
					spawn_position->y = y;
					spawn_position->x = x;
				}
			}
		}
	return appear;
}
