/*
 * Code	for one	creature to chase another
 *
 * chase.c	1.32	(A.I. Design) 12/12/84
 */

#include "rogue.h"
#include "curses.h"

#define	DRAGONSHOT  5	/* one chance in DRAGONSHOT that a dragon will flame */

Position chase_next_position;			/* Where chasing takes	you */

/*
 * runners:
 *	Make all the running monsters move.
 */
void
move_monsters()
{
	register Entity *entity;
	register int distance;

	for	(entity = level_monsters; entity	!= NULL; entity = next(entity)) {
		if (!has_actor_flag(*entity, ACTOR_HELD) && has_actor_flag(*entity, ACTOR_CHASING)) {
			distance = distance_squared(player_position.y, player_position.x, entity->actor_position.y, entity->actor_position.x);
			if	(!(has_actor_flag(*entity, ACTOR_SLOWED) || (entity->actor_species == 'S' && distance > 3)) || entity->actor_move_this_turn)
				move_chasing_monster(entity);
			if (has_actor_flag(*entity, ACTOR_HASTED))
				move_chasing_monster(entity);
			distance = distance_squared(player_position.y, player_position.x, entity->actor_position.y, entity->actor_position.x);
			if (has_actor_flag(*entity, ACTOR_FLIES) && distance > 3)
				move_chasing_monster(entity);
			entity->actor_move_this_turn ^= TRUE;
		}
	}
}

/*
 * do_chase:
 *	Make one thing chase another.
 */
void
move_chasing_monster(monster)
Entity *monster;
{
	int	nearest_door_distance	= 32767, i, distance;
	bool standing_in_door;
	register Entity *item;
	struct room	*previous_room;
	register struct room	*chaser_room, *destination_room;	/* room of chaser, room of chasee */
	Position next_destination;				/* Temporary	destination for	chaser */

	chaser_room	= monster->actor_room;		/* Find room of chaser */
	if (has_actor_flag(*monster,	ACTOR_GREEDY) && chaser_room->gold_amount == 0)
		monster->actor_destination = &player_position;	/*	If gold	has been taken,	run after hero */
	destination_room	= player_room;
	if (monster->actor_destination != &player_position)	/*	Find room of chasee */
		destination_room = room_at(monster->actor_destination);
	if (destination_room == NULL)
		return;
	/*
	 * We don't	count doors as inside rooms for	this routine
	 */
	standing_in_door = (terrain_at(monster->actor_position.y, monster->actor_position.x) == DOOR);


	/*
	 * If the object of	our desire is in a different room,
	 * and we are not in a maze, run to	the door nearest to
	 * our goal.
	 */
over:
	if (chaser_room != destination_room && (chaser_room->flags & ROOM_MAZE) == 0)
	{
		for (i	= 0; i < chaser_room->exit_count;	i++) {	/*	loop through doors */
			distance = distance_squared(monster->actor_destination->y, monster->actor_destination->x,chaser_room->exits[i].y, chaser_room->exits[i].x);
			if	(distance <	nearest_door_distance) {
				next_destination = chaser_room->exits[i];
				nearest_door_distance = distance;
			}
		}
		if (standing_in_door) {
			chaser_room = &passages[cell_flags_at(monster->actor_position.y, monster->actor_position.x) & PASSAGE_NUMBER_MASK];
			standing_in_door = FALSE;
			goto over;
		}
	} else {
		next_destination =	*monster->actor_destination;
		/*
		 * For	monsters which can fire	bolts at the poor hero,	we check to
		 * see	if (a) the hero	in on a	straight line from it, and (b) that
		 * it is within shooting distance, but	outside	of striking range.
		 */
		if ((monster->actor_species == 'D' || monster->actor_species == 'I')
			&&	(monster->actor_position.y ==	player_position.y || monster->actor_position.x == player_position.x
			 || abs(monster->actor_position.y - player_position.y) == abs(monster->actor_position.x - player_position.x))
			&&	((distance=distance_squared(monster->actor_position.y, monster->actor_position.x, player_position.y, player_position.x)) > 2
			 && distance <= BOLT_LENGTH	* BOLT_LENGTH)
			&&	!has_actor_flag(*monster, ACTOR_CANCELLED) && random_below(DRAGONSHOT) == 0)
		{
			running = FALSE;
			action_direction.y = sign(player_position.y - monster->actor_position.y);
			action_direction.x = sign(player_position.x - monster->actor_position.x);
			fire_bolt(&monster->actor_position,&action_direction,monster->actor_species == 'D' ? "flame" : "frost");
			return;
		}
	}
	/*
	 * This now	contains what we want to run to	this time
	 * so we run to it.	 If we hit it we either	want to	fight it
	 * or stop running
	 */
	choose_chase_step(monster, &next_destination);
	if (positions_equal(chase_next_position, player_position)) {
		monster_attack(monster);
		return;
	} else if (positions_equal(chase_next_position,	*monster->actor_destination)) {
		for (item = level_items; item != NULL; item =	next(item))
			if	(monster->actor_destination == &item->item_position) {
				byte oldchar;

				detach(level_items, item);
				attach(monster->actor_inventory, item);
				oldchar = terrain_at(item->item_position.y, item->item_position.x) =
				(monster->actor_room->flags & ROOM_ABSENT) ? PASSAGE : FLOOR;
				if (player_can_see_position(item->item_position.y, item->item_position.x))
					mvaddch(item->item_position.y, item->item_position.x, oldchar);
				monster->actor_destination = choose_monster_destination(monster);
				break;
			}
	}
	if (monster->actor_species == 'F')
		return;
	/*
	 * If the chasing thing moved, update the screen
	 */
	if (monster->actor_previous_tile != '@') {
		if	(monster->actor_previous_tile ==	' ' && player_can_see_position(monster->actor_position.y, monster->actor_position.x)
			   && terrain_map[map_index(monster->actor_position.y,monster->actor_position.x)] == FLOOR)
			mvaddch(monster->actor_position.y, monster->actor_position.x, FLOOR);
		else if (monster->actor_previous_tile == FLOOR && !player_can_see_position(monster->actor_position.y, monster->actor_position.x)
				&& !has_actor_flag(player, ACTOR_DETECTS_MONSTERS))
			mvaddch(monster->actor_position.y, monster->actor_position.x, ' ');
		else
			mvaddch(monster->actor_position.y, monster->actor_position.x, monster->actor_previous_tile);
	}
	previous_room = monster->actor_room;
	if (!positions_equal(chase_next_position, monster->actor_position))
	{
		if ((monster->actor_room = room_at(&chase_next_position)) == NULL) {
			monster->actor_room	= previous_room;
			return;
		}
		if (previous_room != monster->actor_room)
			monster->actor_destination	= choose_monster_destination(monster);
		monster->actor_position = chase_next_position;
	}

	if (player_can_see_monster(monster)) {
		if (cell_flags_at(chase_next_position.y,chase_next_position.x) & CELL_PASSAGE)
			standout();
		monster->actor_previous_tile = mvinch(chase_next_position.y, chase_next_position.x);
		mvaddch(chase_next_position.y, chase_next_position.x, monster->actor_disguise);
	}
	else if (has_actor_flag(player,	ACTOR_DETECTS_MONSTERS))
	{
		standout();
		monster->actor_previous_tile = mvinch(chase_next_position.y, chase_next_position.x);
		mvaddch(chase_next_position.y, chase_next_position.x, monster->actor_species);
	}
	else
		monster->actor_previous_tile = '@';

	if (monster->actor_previous_tile == FLOOR && (previous_room->flags & ROOM_DARK))
		monster->actor_previous_tile = ' ';
	standend();
}

/*
 * see_monst:
 *	Return TRUE if the hero can see the monster
 */
bool
player_can_see_monster(monster)
register Entity *monster;
{
	if (has_actor_flag(player, ACTOR_BLIND))
		return	FALSE;
	if (has_actor_flag(*monster,	ACTOR_INVISIBLE) && !has_actor_flag(player,	ACTOR_SEES_INVISIBLE))
		return	FALSE;
	if (distance_squared(monster->actor_position.y, monster->actor_position.x, player_position.y, player_position.x) >= LAMPDIST &&
	  ((monster->actor_room != player_room || (monster->actor_room->flags & ROOM_DARK) ||
	  (monster->actor_room->flags & ROOM_MAZE))))
		return FALSE;
	/*
	 * If we are seeing	the enemy of a vorpally	enchanted weapon for the first
	 * time, give the player a hint as to what that weapon is good for.
	 */
	if (equipped_weapon != NULL && monster->actor_species == equipped_weapon->item_slays_species
	  && ((equipped_weapon->item_flags & ITEM_VORPAL_FLASHED) == 0))
	{
		equipped_weapon->item_flags |=	ITEM_VORPAL_FLASHED;
		show_message(vorpal_flash_message, weapon_names[equipped_weapon->item_subtype], terse	|| expert ? "" : vorpal_flash_intensity);
	}
	return TRUE;
}

/*
 * start_run:
 *	Set a monster running after something or stop it from running
 *	(for	when it	dies)
 */
void
start_monster_chase(runner)
register Position *runner;
{
	register Entity *entity;

	/*
	 * If we couldn't find him,	something is funny
	 */
	entity = monster_at(runner->y, runner->x);
	if (entity != NULL) {
		/*
		 *	Start the beastie running
		 */
		entity->actor_flags |= ACTOR_CHASING;
		entity->actor_flags &= ~ACTOR_HELD;
		entity->actor_destination	= choose_monster_destination(entity);
	}
#ifdef DEBUG
	else
		debug("start_run: moat == NULL ???");
#endif //DEBUG
}

/*
 * chase:
 *	Find	the spot for the chaser(er) to move closer to the
 *	chasee(ee).	Returns	TRUE if	we want	to keep	on chasing later
 *	FALSE if we reach the goal.
 *
 *	@@ Wrong documentation: function is actually a void, there is no return
 */
void
choose_chase_step(monster, destination)
Entity *monster;
Position *destination;
{
	register int	x, y;
	int	best_distance, candidate_distance;
	register Entity *item;
	Position *origin;
	byte character;
	int	equally_close_positions =	1;

	origin = &monster->actor_position;
	/*
	 * If the thing is confused, let it	move randomly. Phantoms
	 * are slightly confused all of the	time, and bats are
	 * quite confused all the time
	 */
	if ((has_actor_flag(*monster, ACTOR_CONFUSED)	&& random_below(5) != 0)	|| (monster->actor_species == 'P' && random_below(5)	== 0)
		|| (monster->actor_species	== 'B' && random_below(2) == 0))
	{
		/*
		 * get	a valid	random move
		 */
		random_move(monster,&chase_next_position);
		best_distance =	distance_squared(chase_next_position.y, chase_next_position.x, destination->y, destination->x);
		/*
		 * Small chance that it will become un-confused
		 */
		if (random_below(30) ==	17)
			monster->actor_flags &= ~ACTOR_CONFUSED;
	}
	/*
	 * Otherwise, find the empty spot next to the chaser that is
	 * closest to the chasee.
	 */
	else
	{
		register int ey, ex;
		/*
		 * This will eventually hold where we move to get closer
		 * If we can't	find an	empty spot, we stay where we are.
		 */
		best_distance =	distance_squared(origin->y,	origin->x, destination->y, destination->x);
		chase_next_position	= *origin;

		ey = origin->y + 1;
		ex = origin->x + 1;
		for (x	= origin->x	- 1; x <= ex; x++)
		{
			for (y = origin->y - 1; y <= ey; y++)
			{
				Position	candidate_position;

				candidate_position.x = x;
				candidate_position.y = y;
				if (outside_dungeon(y,	x) || !diagonal_move_allowed(origin, &candidate_position))
					continue;
				character = visible_entity_at(y,	x);
				if (is_walkable_symbol(character))
				{
					/*
					 * If it is a scroll, it might be	a scare	monster	scroll
					 * so we need to look it up to see what type it is.
					 */
					if (character ==	SCROLL)
					{
						for (item = level_items; item != NULL; item	= next(item))
						{
							if (y ==	item->item_position.y &&	x == item->item_position.x)
								break;
						}
						if (item != NULL && item->item_subtype == SCROLL_SCARE_MONSTER)
							continue;
					}
					/*
					 * If we didn't find any scrolls at this place or	it
					 * wasn't	a scare	scroll,	then this place	counts
					 */
					candidate_distance = distance_squared(y, x,	destination->y, destination->x);
					if (candidate_distance < best_distance)
					{
						equally_close_positions = 1;
						chase_next_position = candidate_position;
						best_distance	= candidate_distance;
					}
					else if (candidate_distance	== best_distance	&& random_below(++equally_close_positions)	== 0)
					{
						chase_next_position = candidate_position;
						best_distance	= candidate_distance;
					}
				}
			}
		}
	}
}

/*
 * roomin:
 *	Find	what room some coordinates are in. NULL	means they aren't
 *	in any room.
 */
struct room *
room_at(position)
register Position *position;
{
	register struct room *room;
	register byte *flags_cursor;

	for	(room = rooms; room	<= &rooms[MAXROOMS-1]; room++)
		if (position->x < room->origin.x + room->size.x && room->origin.x <= position->x
		 && position->y < room->origin.y + room->size.y && room->origin.y <= position->y)
			return room;
	flags_cursor = &cell_flags_at(position->y, position->x);
	if (*flags_cursor & CELL_PASSAGE)
		return	&passages[*flags_cursor &	PASSAGE_NUMBER_MASK];
#ifdef DEBUG
	debug("in some bizarre place (%d, %d)", position_yx(*position));
#endif //DEBUG
	pending_trapdoor_fall = TRUE;
	return NULL;
}

/*
 * diag_ok:
 *	Check to see	if the move is legal if	it is diagonal
 */
bool
diagonal_move_allowed(start_position, end_position)
register Position *start_position, *end_position;
{
	if (end_position->x == start_position->x || end_position->y	== start_position->y)
		return	TRUE;
	return (is_walkable_symbol(terrain_at(end_position->y,	start_position->x))	&& is_walkable_symbol(terrain_at(start_position->y, end_position->x)));
}

/*
 * cansee:
 *	Returns true	if the hero can	see a certain coordinate.
 */
bool
player_can_see_position(y, x)
register int y,	x;
{
	register struct room *room;
	Position target_position;

	if (has_actor_flag(player, ACTOR_BLIND))
		return	FALSE;
	if (distance_squared(y, x, player_position.y, player_position.x) < LAMPDIST)
		return	TRUE;
	/*
	 * We can only see if the hero in the same room as
	 * the coordinate and the room is lit or if	it is close.
	 */
	target_position.y = y;
	target_position.x = x;
	room	= room_at(&target_position);
	return (room	== player_room && !(room->flags & ROOM_DARK));
}

/*
 * find_dest:
 *	find	the proper destination for the monster
 */
Position *
choose_monster_destination(entity)
register Entity *entity;
{
	register Entity *item;
	register int probability;
	register struct room *room;

	if ((probability =	monster_definitions[entity->actor_species - 'A'].carry_probability) <= 0 || entity->actor_room == player_room
	|| player_can_see_monster(entity))
		return &player_position;
	room = entity->actor_room;
	for	(item = level_items;	item != NULL; item = next(item))
	{
	if (item->item_category == SCROLL && item->item_subtype == SCROLL_SCARE_MONSTER)
		continue;
	if (room_at(&item->item_position) == room && random_below(100) < probability)
	{
		for (entity = level_monsters; entity != NULL; entity = next(entity))
		if (entity->actor_destination == &item->item_position)
			break;
		if	(entity == NULL)
		return &item->item_position;
	}
	}
	return &player_position;
}
