/*
 * Create the layout for the new level
 *
 * rooms.c	1.4 (A.I. Design)	12/16/84
 */

#include "rogue.h"
#include "screen.h"

#define GOLDGRP 1

static void	draw_vertical_room_wall( struct room *room, int startx);
static void	draw_horizontal_room_wall(struct room *room, int starty);

/*
 * generate_rooms:
 *	Create rooms and corridors with a connectivity graph
 */
void
generate_rooms(void)
{
	register int i, room_index;
	struct room *room;
	register Entity *monster;
	int left_out;
	Position top;
	Position grid_cell_size;
	Position monster_position;
	int room_bottom;

	room_bottom = dungeon_bottom_row + 1;

	/*
	 * bsze is the maximum room size
	 */
	grid_cell_size.x = COLS/3;
	grid_cell_size.y = room_bottom/3;
	/*
	 * Clear things for a new level
	 */
	for (room = rooms; room < &rooms[MAXROOMS]; room++)
		room->gold_amount = room->exit_count = room->flags = 0;
	/*
	 * Put the gone rooms, if any, on the level
	 */
	left_out = random_below(4);
	for (i = 0; i < left_out; i++) {
		do
			room = &rooms[(room_index = random_room_index())];
		while (room->flags & ROOM_MAZE);
		room->flags |= ROOM_ABSENT;
#ifdef TEST
		if (room_index > 2 && ((dungeon_level > 10 && random_below(20) < dungeon_level - 9) || istest()))
#else //TEST
		if (room_index > 2 && dungeon_level > 10 && random_below(20) < dungeon_level - 9)
#endif //TEST
			room->flags |= ROOM_MAZE;
	}
	/*
	 * dig and populate all the rooms on the level
	 */
	for (i = 0, room = rooms; i < MAXROOMS; room++, i++) {
		/*
		 * Find upper left corner of box that this room goes in
		 */
		top.x = (i%3)*grid_cell_size.x + 1;
		top.y = i/3*grid_cell_size.y;
		if (room->flags & ROOM_ABSENT) {
			/*
			 * If the gone room is a maze room, draw the maze and set the
			 * size equal to the maximum possible.
			 */
			if (room->flags&ROOM_MAZE) {
				room->origin.x = top.x;
				room->origin.y = top.y;
				draw_maze(room);
			} else {
				/*
				 * Place a gone room.  Make certain that there is a blank line
				 * for passage drawing.
				 */
				do {
					room->origin.x = top.x + random_below(grid_cell_size.x-2) + 1;
					room->origin.y = top.y + random_below(grid_cell_size.y-2) + 1;
					room->size.x = -COLS;
					room->size.x = -room_bottom;
				} while (!(room->origin.y > 0 && room->origin.y < room_bottom-1));
			}
			continue;
		}
		if (random_below(10) < (dungeon_level - 1))
			room->flags |= ROOM_DARK;
		/*
		 * Find a place and size for a random room
		 */
		do {
			room->size.x = random_below(grid_cell_size.x - 4) + 4;
			room->size.y = random_below(grid_cell_size.y - 4) + 4;
			room->origin.x = top.x + random_below(grid_cell_size.x - room->size.x);
			room->origin.y = top.y + random_below(grid_cell_size.y - room->size.y);
		} while (room->origin.y == 0);
		draw_room(room);
		/*
		 * Put the gold in
		 */
		if ((random_below(2) == 0) && (!saw_amulet || (dungeon_level >= deepest_level))) {
			Entity *gold;

			if ((gold = allocate_entity()) != NULL) {
				gold->item_gold_amount = room->gold_amount = GOLDCALC;
				while (1) {
					byte gold_background;

					random_room_position(room, &room->gold_position);
					gold_background =  terrain_at(room->gold_position.y, room->gold_position.x);
					if (is_floor_tile(gold_background))
						break;
				}
				copy_value(gold->item_position,room->gold_position);
				gold->item_flags = ITEM_STACKABLE;
				gold->item_stack_group = GOLDGRP;
				gold->item_category = GOLD;
				attach(level_items, gold);
				terrain_at(room->gold_position.y, room->gold_position.x) = GOLD;
			}
		}
		/*
		 * Put the monster in
		 */
		if (random_below(100) < (room->gold_amount > 0 ? 80 : 25)) {
			if ((monster = allocate_entity()) != NULL) {
				byte monster_symbol;

				do {
					random_room_position(room, &monster_position);
					monster_symbol = visible_entity_at(monster_position.y, monster_position.x);
				} while (!is_floor_tile(monster_symbol));
				new_monster(monster, random_monster_species(FALSE), &monster_position);
				give_monster_item(monster);
			}
		}
	}
}

/*
 * draw_room:
 *	Draw a box around a room and lay down the floor
 */
void
draw_room(struct room *room)
{
	register int y, x;

	/*
	 * Here we draw normal rooms, one side at a time
	 */
	draw_vertical_room_wall(room, room->origin.x);			/* Draw left side */
	draw_vertical_room_wall(room, room->origin.x + room->size.x - 1);	/* Draw right side */
	draw_horizontal_room_wall(room, room->origin.y);			/* Draw top */
	draw_horizontal_room_wall(room, room->origin.y + room->size.y - 1);	/* Draw bottom */
	terrain_at(room->origin.y,room->origin.x) = ULWALL;
	terrain_at(room->origin.y,room->origin.x+room->size.x - 1) = URWALL;
	terrain_at(room->origin.y+room->size.y-1,room->origin.x) = LLWALL;
	terrain_at(room->origin.y+room->size.y-1,room->origin.x+room->size.x - 1) = LRWALL;
	/*
	 * Put the floor down
	 */
	for (y = room->origin.y + 1; y < room->origin.y + room->size.y - 1; y++)
		for (x = room->origin.x + 1; x < room->origin.x + room->size.x - 1; x++)
			terrain_at(y, x) = FLOOR;
}

/*
 * draw_vertical_room_wall:
 *	Draw a vertical line
 */
static
void
draw_vertical_room_wall(struct room *room, int startx)
{
	register int y;

	for (y = room->origin.y + 1; y <= room->size.y + room->origin.y - 1; y++)
		terrain_at(y, startx) = VWALL;
}

/*
 * draw_horizontal_room_wall:
 *	Draw a horizontal line
 */
static
void
draw_horizontal_room_wall(struct room *room, int starty)
{
	register int x;

	for (x = room->origin.x; x <= room->origin.x + room->size.x - 1; x++)
		terrain_at(starty, x) = HWALL;
}

/*
 * random_room_position:
 *	Pick a random spot in a room
 */
void
random_room_position(struct room *room, Position *position)
{
	position->x = room->origin.x + random_below(room->size.x - 2) + 1;
	position->y = room->origin.y + random_below(room->size.y - 2) + 1;
}

/*
 * enter_room:
 *	Code that is executed whenver you appear in a room
 */
void
enter_room(Position *position)
{
	register struct room *room;
	register int y, x;
	register Entity *entity;

	room = player_room = room_at(position);
	if (pending_trapdoor_fall || ((room->flags & ROOM_ABSENT) && (room->flags & ROOM_MAZE) == 0)) {
#ifdef DEBUG
		show_message("in a gone room");
#endif //DEBUG
		return;
	}
	wake_room_monsters(room);
	if (!(room->flags&ROOM_DARK) && !has_actor_flag(player,ACTOR_BLIND) && !(room->flags&ROOM_MAZE))
		for (y = room->origin.y; y < room->size.y + room->origin.y; y++) {
			move(y, room->origin.x);
			for (x = room->origin.x; x < room->size.x + room->origin.x; x++) {
				/*
				 * Displaying monsters is all handled in the
				 * chase code now
				 */
				entity = monster_at(y, x);
				if (entity == NULL || !player_can_see_monster(entity))
					addch(terrain_at(y, x));
				else {
					entity->actor_previous_tile = terrain_at(y,x);
					addch(entity->actor_disguise);
				}
			}
		}
}

/*
 * leave_room:
 *	Code for when we exit a room
 */
void
leave_room(Position *position)
{
	register int y, x;
	register struct room *room;
	register byte floor;
	register byte character;

	room = player_room;
	player_room = &passages[cell_flags_at(position->y, position->x) & PASSAGE_NUMBER_MASK];
	floor = ((room->flags & ROOM_DARK) && !has_actor_flag(player, ACTOR_BLIND)) ? ' ' : FLOOR;
	if (room->flags & ROOM_MAZE)
		floor = PASSAGE;
	for (y = room->origin.y + 1; y < room->size.y + room->origin.y - 1; y++)
		for (x = room->origin.x + 1; x < room->size.x + room->origin.x - 1; x++)
			switch (character = mvinch(y, x)) {
			case ' ':
			case PASSAGE:
			case TRAP:
			case STAIRS:
				break;
			case FLOOR:
				if (floor == ' ')
					addch(' ');
				break;
			default:
				/*
				 * to check for monster, we have to strip out
				 * standout bit
				 * @ No we don't, inch() took care of that already
				 * @ originally tested for isupper(toascii(ch))
				 */
				if (is_monster_symbol(character))
				{
					if (has_actor_flag(player, ACTOR_DETECTS_MONSTERS)) {
						standout();
						addch(character);
						standend();
						break;
					} else
						monster_at(y, x)->actor_previous_tile = '@';
				}
				addch(floor);
				break;
			}
	wake_room_monsters(room);
}
