/*
 * new_level:
 *	Dig and draw a new level
 *
 * new_level.c	1.4 (A.I. Design) 12/13/84
 */

#include "rogue.h"
#include "curses.h"

#define TREAS_ROOM 20	/* one chance in TREAS_ROOM for a treasure room */
#define MAXTREAS 10	/* maximum number of treasures in a treasure room */
#define MINTREAS 2	/* minimum number of treasures in a treasure room */

static void	populate_treasure_room(void);

void
generate_level(void)
{
	register int room_index, i;
	Entity *monster;
	byte *cell_flag_ptr;
	int index;
	Position stairs;

	player.actor_flags &= ~ACTOR_HELD;	/* unhold when you go down just in case */
	/*
	 * Monsters only get displayed when you move
	 * so start a level by having the poor guy rest
	 * God forbid he lands next to a monster!
	 */
	if (dungeon_level > deepest_level)
		deepest_level = dungeon_level;
#ifdef ENABLE_COPY_PROTECTION_CHECKS
	protection_tick();  //@ make sure the clock is ticking?
	if (dungeon_level > 1 && code_checksum() != expected_code_checksum)
		halt_game();
#endif
	/*
	 * Clean things off from last level
	 */
	fill_bytes(terrain_map, ((MAXLINES-3)*MAXCOLS),' ');
	fill_bytes(cell_flags, (MAXLINES-3)*MAXCOLS, CELL_REVEALED);
	/*
	 * Free up the monsters on the last level
	 */
	for (monster = level_monsters; monster != NULL; monster = next(monster))
		free_list(monster->actor_inventory);
	free_list(level_monsters);
	/*
	 * just in case we left some flytraps behind
	 */
	reset_flytrap_damage();
	/*
	 * Throw away stuff left on the previous level (if anything)
	 */
	free_list(level_items);
	generate_rooms();				/* Draw rooms */
#ifdef ROGUE_DOS_CURSES
	if (deepest_level == 1) {
		status_layout_dirty = TRUE;
		if (svwin_ds == -1) {
			move(dungeon_bottom_row, 0);
			clrtoeol();
		} else
			clear();
	}
	animate_level_transition();
#else
	if (deepest_level > 1)
	{
		animate_level_transition();
	}
#endif
	update_status_line();
	generate_passages();			/* Draw passages */
	levels_without_food++;
	populate_level_items();			/* Place objects (if any) */
	/*
	 * Place the staircase down.
	 */
	i = 0;
	do {
		room_index = random_room_index();
	random_room_position(&rooms[room_index], &stairs);
	index = map_index(stairs.y, stairs.x);
	if (i++ > 100)
	{
		i = 0;
		random_state = random_seed_from_clock();
	}
	} while (!is_floor_tile(terrain_map[index]));
	terrain_map[index] = STAIRS;
	/*
	 * Place the traps
	 */
	if (random_below(10) < dungeon_level) {
		trap_count = random_below(dungeon_level / 4) + 1;
		if (trap_count > MAXTRAPS)
			trap_count = MAXTRAPS;
		i = trap_count;
		while (i--) {
			do {
				room_index = random_room_index();
				random_room_position(&rooms[room_index], &stairs);
				index = map_index(stairs.y, stairs.x);
			} while (!is_floor_tile(terrain_map[index]));
			cell_flag_ptr = &cell_flags[index];
			*cell_flag_ptr &= ~CELL_REVEALED;
			*cell_flag_ptr |= random_below(NTRAPS);
		}
	}
	do {
		room_index = random_room_index();
		random_room_position(&rooms[room_index], &player_position);
		index = map_index(player_position.y, player_position.x);
	} while (!(is_floor_tile(terrain_map[index]) && (cell_flags[index] & CELL_REVEALED)
				&& monster_at(player_position.y, player_position.x) == NULL));

	message_column = 0;
	enter_room(&player_position);
	mvaddch(player_position.y, player_position.x, PLAYER);
	copy_value(previous_player_position,player_position);
	previous_player_room = player_room;
	if (has_actor_flag(player, ACTOR_DETECTS_MONSTERS))
		set_monster_detection(FALSE);
}

/*
 * rnd_room:
 *	Pick a room that is really there
 */
int
random_room_index(void)
{
	register int room_index;

	do
	room_index = random_below(MAXROOMS);
	while (!((rooms[room_index].flags & ROOM_ABSENT)==0||(rooms[room_index].flags & ROOM_MAZE)));
	return room_index;
}

/*
 * put_things:
 *	Put potions and scrolls on this level
 */
void
populate_level_items(void)
{
	register int i = 0;
	register Entity *item;
	register int room_index;
	Position placement;

	/*
	 * Once you have found the amulet, the only way to get new stuff is
	 * go down into the dungeon.
	 * This is real unfair - I'm going to allow one thing, that way
	 * the poor guy will get some food.
	 */
	if (saw_amulet && dungeon_level < deepest_level)
		i = MAXOBJ - 1;
	else {
		/*
		 * If he is really deep in the dungeon and he hasn't found the
		 * amulet yet, put it somewhere on the ground
		 * Check this first so if we are out of memory the guy has a
		 * hope of getting the amulet
		 */
		if (dungeon_level >= AMULETLEVEL && !saw_amulet) {
			if ((item = allocate_entity()) != NULL) {
				attach(level_items, item);
				item->item_hit_bonus = item->item_damage_bonus = 0;
				item->item_melee_damage = item->item_thrown_damage = "0d0";
				item->item_modifier = 11;
				item->item_category = AMULET;
				/*
				 * Put it somewhere
				 */
				do {
					room_index = random_room_index();
					random_room_position(&rooms[room_index], &placement);
				} while (!is_floor_tile(visible_entity_at(placement.y, placement.x)));
				terrain_at(placement.y, placement.x) = AMULET;
				copy_value(item->item_position,placement);
			}
		}
		/*
		 * check for treasure rooms, and if so, put it in.
		 */
		if (random_below(TREAS_ROOM) == 0)
			populate_treasure_room();
	}
	/*
	 * Do MAXOBJ attempts to put things on a level
	 */
	for (;i < MAXOBJ; i++)
		if (allocated_entity_count < MAXITEMS && random_below(100) < 35) {
			/*
			 * Pick a new object and link it in the list
			 */
			item = generate_item();
			attach(level_items, item);
			/*
			 * Put it somewhere
			 */
			do {
				room_index = random_room_index();
				random_room_position(&rooms[room_index], &placement);
			} while (!is_floor_tile(terrain_at(placement.y, placement.x)));
			terrain_at(placement.y, placement.x) = item->item_category;
			copy_value(item->item_position,placement);
		}
}

/*
 * treas_room:
 *	Add a treasure room
 */
#define MAXTRIES 10	/* max number of tries to put down a monster */

static
void
populate_treasure_room(void)
{
	int item_count, index;
	register Entity *monster;
	register struct room *room;
	int spots, num_monst;
	Position spawn_position;

	room = &rooms[random_room_index()];
	spots = (room->size.y - 2) * (room->size.x - 2) - MINTREAS;
	if (spots > (MAXTREAS - MINTREAS))
		spots = (MAXTREAS - MINTREAS);
	num_monst = item_count = random_below(spots) + MINTREAS;
	while (item_count-- && allocated_entity_count < MAXITEMS)
	{
		do
		{
			random_room_position(room, &spawn_position);
			index = map_index(spawn_position.y, spawn_position.x);
		} while (!is_floor_tile(terrain_map[index]));
		monster = generate_item();
		copy_value(monster->item_position,spawn_position);
		attach(level_items, monster);
		terrain_map[index] = monster->item_category;
	}

	/*
	 * fill up room with monsters from the next level down
	 */

	if ((item_count = random_below(spots) + MINTREAS) < num_monst + 2)
		item_count = num_monst + 2;
	spots = (room->size.y - 2) * (room->size.x - 2);
	if (item_count > spots)
		item_count = spots;
	dungeon_level++;
	while (item_count--)
	{
		for (spots = 0; spots < MAXTRIES; spots++)
		{
			random_room_position(room, &spawn_position);
			index = map_index(spawn_position.y, spawn_position.x);
			if (is_floor_tile(terrain_map[index]) && monster_at(spawn_position.y, spawn_position.x) == NULL)
				break;
		}
		if (spots != MAXTRIES)
		{
			if ((monster = allocate_entity()) != NULL)
			{
				new_monster(monster, random_monster_species(FALSE), &spawn_position);
	#ifdef TEST
				if (pending_trapdoor_fall && me())
					show_message("treasure rm bailout");
	#endif //TEST
				monster->actor_flags |= ACTOR_AGGRESSIVE;	/* no sloughers in THIS room */
				give_monster_item(monster);
			}
		}
	}
	dungeon_level--;
}
